#include "FFmpegDecoderVideo.hpp"

#include <osg/Notify>
#include <osg/Timer>

#include <stdexcept>
#include <string.h>

extern "C" {
#include <libavutil/imgutils.h>
}

namespace osgFFmpeg {

FFmpegDecoderVideo::FFmpegDecoderVideo(PacketQueue & packets, FFmpegClocks & clocks) :
    m_packets(packets),
    m_clocks(clocks),
    m_stream(nullptr),
    m_context(nullptr),
    m_codec(nullptr),
    m_packet_data(nullptr),
    m_bytes_remaining(0),
    m_packet_pts(AV_NOPTS_VALUE),
    m_writeBuffer(0),
    m_user_data(nullptr),
    m_publish_func(nullptr),
    m_paused(true),
    m_exit(false)
#ifdef USE_SWSCALE
    ,m_swscale_ctx(nullptr)
#endif
{
}

FFmpegDecoderVideo::~FFmpegDecoderVideo()
{
    OSG_INFO << "Destructing FFmpegDecoderVideo..." << std::endl;

    this->close(true);

#ifdef USE_SWSCALE
    if (m_swscale_ctx)
    {
        sws_freeContext(m_swscale_ctx);
        m_swscale_ctx = nullptr;
    }
#endif

    if (m_context)
    {
        avcodec_free_context(&m_context);
    }

    OSG_INFO << "Destructed FFmpegDecoderVideo" << std::endl;
}

void FFmpegDecoderVideo::open(AVStream * const stream)
{
    m_stream = stream;
    m_context = avcodec_alloc_context3(nullptr);
    avcodec_parameters_to_context(m_context, stream->codecpar);

    // Trust the video size given at this point
    // (avcodec_open seems to sometimes return a 0x0 size)
    m_width = m_context->width;
    m_height = m_context->height;
    findAspectRatio();

    // Find out whether we support Alpha channel
    m_alpha_channel = (m_context->pix_fmt == AV_PIX_FMT_YUVA420P);

    // Find out the framerate
    m_frame_rate = av_q2d(stream->avg_frame_rate);

    // Find the decoder for the video stream
    m_codec = avcodec_find_decoder(m_context->codec_id);

    if (m_codec == nullptr)
        throw std::runtime_error("avcodec_find_decoder() failed");

    // Open codec
    if (avcodec_open2(m_context, m_codec, nullptr) < 0)
        throw std::runtime_error("avcodec_open2() failed");

    // Allocate video frame
    m_frame.reset(av_frame_alloc());

    // Allocate converted RGB frame
    m_frame_rgba.reset(av_frame_alloc());
    m_buffer_rgba[0].resize(av_image_get_buffer_size(AV_PIX_FMT_RGB24, width(), height(), 1));
    m_buffer_rgba[1].resize(m_buffer_rgba[0].size());

    // Assign appropriate parts of the buffer to image planes in m_frame_rgba
    av_image_fill_arrays(m_frame_rgba->data, m_frame_rgba->linesize, &(m_buffer_rgba[0])[0], AV_PIX_FMT_RGB24, width(), height(), 1);

    // Override get_buffer2() from codec context in order to retrieve the PTS of each frame.
    m_context->opaque = this;
    m_context->get_buffer2 = getBuffer;
}

void FFmpegDecoderVideo::close(bool waitForThreadToExit)
{
    if (isRunning())
    {
        m_exit = true;
        if (waitForThreadToExit)
            join();
    }
}

void FFmpegDecoderVideo::pause(bool pause)
{
    m_paused = pause;
}

void FFmpegDecoderVideo::run()
{
    try
    {
        decodeLoop();
    }
    catch (const std::exception &error)
    {
        OSG_WARN << "FFmpegDecoderVideo::run : " << error.what() << std::endl;
    }
    catch (...)
    {
        OSG_WARN << "FFmpegDecoderVideo::run : unhandled exception" << std::endl;
    }
}

void FFmpegDecoderVideo::decodeLoop()
{
    FFmpegPacket packet;
    double pts;

    while (!m_exit)
    {
        // Work on the current packet until we have decoded all of it
        while (m_bytes_remaining > 0)
        {
            // Save global PTS to be stored in m_frame via getBuffer()
            m_packet_pts = packet.packet.pts;

            // Decode video frame
            int frame_finished = 0;
            const int bytes_decoded = avcodec_receive_frame(m_context, m_frame.get());

            if (bytes_decoded == 0)
            {
                frame_finished = 1;
                m_bytes_remaining -= bytes_decoded;
                m_packet_data += bytes_decoded;
            }
            else if (bytes_decoded == AVERROR(EAGAIN))
            {
                break;
            }
            else if (bytes_decoded < 0)
            {
                throw std::runtime_error("avcodec_receive_frame() failed");
            }

            // Publish the frame if we have decoded a complete frame
            if (frame_finished)
            {
                if (m_frame->pts != AV_NOPTS_VALUE)
                {
                    pts = av_q2d(m_stream->time_base) * m_frame->pts;
                }
                else
                {
                    pts = 0;
                }

                const double synched_pts = m_clocks.videoSynchClock(m_frame.get(), av_q2d(av_inv_q(m_context->framerate)), pts);
                const double frame_delay = m_clocks.videoRefreshSchedule(synched_pts);

                publishFrame(frame_delay, m_clocks.audioDisabled());
            }
        }

        while (m_paused && !m_exit)
        {
            OpenThreads::Thread::microSleep(10000);
        }

        // Get the next packet
        pts = 0;

        if (packet.valid())
            packet.clear();

        bool is_empty = true;
        packet = m_packets.timedPop(is_empty, 10);

        if (!is_empty)
        {
            if (packet.type == FFmpegPacket::PACKET_DATA)
            {
                m_bytes_remaining = packet.packet.size;
                m_packet_data = packet.packet.data;
                avcodec_send_packet(m_context, &(packet.packet));
            }
            else if (packet.type == FFmpegPacket::PACKET_FLUSH)
            {
                avcodec_flush_buffers(m_context);
            }
        }
    }
}

void FFmpegDecoderVideo::findAspectRatio()
{
    float ratio = 0.0f;

    if (m_context->sample_aspect_ratio.num != 0)
        ratio = float(av_q2d(m_context->sample_aspect_ratio));

    if (ratio <= 0.0f)
        ratio = 1.0f;

    m_pixel_aspect_ratio = ratio;
}

int FFmpegDecoderVideo::convert(AVFrame *dst, int dst_pix_fmt, AVFrame *src,
                                int src_pix_fmt, int src_width, int src_height)
{
    osg::Timer_t startTick = osg::Timer::instance()->tick();
#ifdef USE_SWSCALE
    if (m_swscale_ctx == nullptr)
    {
        m_swscale_ctx = sws_getContext(src_width, src_height, (AVPixelFormat)src_pix_fmt,
                                       src_width, src_height, (AVPixelFormat)dst_pix_fmt,
                                       SWS_BICUBIC, nullptr, nullptr, nullptr);
    }

    OSG_DEBUG << "Using sws_scale ";

    int result = sws_scale(m_swscale_ctx,
                           src->data, src->linesize, 0, src_height,
                           dst->data, dst->linesize);
#else

    OSG_DEBUG << "Using img_convert ";

    int result = av_image_copy_to_buffer(dst->data, dst_pix_fmt, src->data, src_pix_fmt, src_width, src_height);
#endif
    osg::Timer_t endTick = osg::Timer::instance()->tick();
    OSG_DEBUG << " time = " << osg::Timer::instance()->delta_m(startTick, endTick) << "ms" << std::endl;

    return result;
}

void FFmpegDecoderVideo::publishFrame(const double delay, bool audio_disabled)
{
    // If no publishing function, just ignore the frame
    if (m_publish_func == nullptr)
        return;

    // If the display delay is too small, we better skip the frame.
    if (!audio_disabled && delay < -0.010)
        return;

    AVFrame *src = m_frame.get();
    AVFrame *dst = m_frame_rgba.get();

    // Assign appropriate parts of the buffer to image planes in m_frame_rgba
    av_image_fill_arrays(dst->data, dst->linesize, &(m_buffer_rgba[m_writeBuffer])[0], AV_PIX_FMT_RGB24, width(), height(), 1);

    // Convert YUVA420p (i.e. YUV420p plus alpha channel) using our own routine
    if (m_context->pix_fmt == AV_PIX_FMT_YUVA420P)
        yuva420pToRgba(dst, src, width(), height());
    else
        convert(dst, AV_PIX_FMT_RGB24, src, m_context->pix_fmt, width(), height());

    // Wait 'delay' seconds before publishing the picture.
    int i_delay = static_cast<int>(delay * 1000000 + 0.5);

    while (i_delay > 1000)
    {
        // Avoid infinite/very long loops
        if (m_exit)
            return;

        const int micro_delay = (std::min)(1000000, i_delay);

        OpenThreads::Thread::microSleep(micro_delay);

        i_delay -= micro_delay;
    }

    m_writeBuffer = 1 - m_writeBuffer;

    m_publish_func(*this, m_user_data);
}

void FFmpegDecoderVideo::yuva420pToRgba(AVFrame * const dst, AVFrame * const src, int width, int height)
{
    convert(dst, AV_PIX_FMT_RGB24, src, m_context->pix_fmt, width, height);

    const size_t bpp = 4;

    uint8_t *a_dst = dst->data[0] + 3;

    for (int h = 0; h < height; ++h)
    {
        const uint8_t *a_src = src->data[3] + h * src->linesize[3];

        for (int w = 0; w < width; ++w)
        {
            *a_dst = *a_src;
            a_dst += bpp;
            a_src += 1;
        }
    }
}

int FFmpegDecoderVideo::getBuffer(AVCodecContext * const context, AVFrame * const picture, int flags)
{
    AVBufferRef *ref;
    const FFmpegDecoderVideo * const this_ = reinterpret_cast<const FFmpegDecoderVideo*>(context->opaque);

    const int result = avcodec_default_get_buffer2(context, picture, flags);
    int64_t *p_pts = reinterpret_cast<int64_t*>(av_malloc(sizeof(int64_t)));

    *p_pts = this_->m_packet_pts;
    picture->opaque = p_pts;

    ref = av_buffer_create((uint8_t *)picture->opaque, sizeof(int64_t), FFmpegDecoderVideo::freeBuffer, picture->buf[0], flags);
    picture->buf[0] = ref;

    return result;
}

void FFmpegDecoderVideo::freeBuffer(void *opaque, uint8_t *data)
{
    AVBufferRef *ref = (AVBufferRef *)opaque;
    av_buffer_unref(&ref);
    av_free(data);
}

} // namespace osgFFmpeg

