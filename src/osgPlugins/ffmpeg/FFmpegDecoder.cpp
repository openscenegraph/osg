#include "FFmpegDecoder.hpp"
#include "FFmpegParameters.hpp"

#include <osg/Notify>
#include <osgDB/FileNameUtils>

#include <cassert>
#include <limits>
#include <stdexcept>
#include <string.h>
#include <iostream>

// Changes for FFMpeg version greater than 0.6
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(52, 64, 0)
#define CODEC_TYPE_AUDIO AVMEDIA_TYPE_AUDIO
#define CODEC_TYPE_VIDEO AVMEDIA_TYPE_VIDEO
#endif

#ifdef AVERROR
#define AVERROR_IO AVERROR(EIO)
#define AVERROR_NUMEXPECTED AVERROR(EDOM)
#define AVERROR_NOMEM AVERROR(ENOMEM)
#define AVERROR_NOFMT AVERROR(EILSEQ)
#define AVERROR_NOTSUPP AVERROR(ENOSYS)
#define AVERROR_NOENT AVERROR(ENOENT)
#endif

namespace osgFFmpeg {

static std::string AvStrError(int errnum)
{
    char buf[128];
    av_strerror(errnum, buf, sizeof(buf));
    return std::string(buf);
}

FFmpegDecoder::FFmpegDecoder() :
    m_audio_stream(nullptr),
    m_video_stream(nullptr),
    m_audio_index(-1),
    m_video_index(-1),
    m_audio_queue(100),
    m_video_queue(100),
    m_audio_decoder(m_audio_queue, m_clocks),
    m_video_decoder(m_video_queue, m_clocks),
    m_state(NORMAL),
    m_loop(false)
{

}



FFmpegDecoder::~FFmpegDecoder()
{
    close(true);
}


bool FFmpegDecoder::open(const std::string & filename, FFmpegParameters* parameters)
{
    try
    {
        // Open video file
        AVFormatContext * p_format_context = nullptr;
        AVInputFormat *iformat = nullptr;

        if (filename.compare(0, 5, "/dev/") == 0)
        {
#ifdef ANDROID
            throw std::runtime_error("Device not supported on Android");
#else
            avdevice_register_all();

            if (parameters)
            {
                av_dict_set(parameters->getOptions(), "video_size", "640x480", 0);
                av_dict_set(parameters->getOptions(), "framerate", "1:30", 0);
            }

            std::string format = "video4linux2";
            iformat = const_cast<AVInputFormat*>(av_find_input_format(format.c_str()));

            if (iformat)
            {
                OSG_INFO << "Found input format: " << format << std::endl;
            }
            else
            {
                OSG_INFO << "Failed to find input format: " << format << std::endl;
            }

#endif
        }
        else
        {
            iformat = parameters ? const_cast<AVInputFormat*>(parameters->getFormat()) : nullptr;
            AVIOContext* context = parameters ? parameters->getContext() : nullptr;
            if (context != nullptr)
            {
                p_format_context = avformat_alloc_context();
                p_format_context->pb = context;
            }
        }

        int error = avformat_open_input(&p_format_context, filename.c_str(), iformat, parameters->getOptions());
        if (error != 0)
        {
            throw std::runtime_error("avformat_open_input() failed: " + AvStrError(error));
        }

        m_format_context.reset(p_format_context);

        // Retrieve stream info
        // Only buffer up to one and a half seconds by default
        float max_analyze_duration = 1.5;
        AVDictionaryEntry *mad = av_dict_get(*parameters->getOptions(), "mad", NULL, 0);
        if (mad) {
            max_analyze_duration = atof(mad->value);
        }
        p_format_context->max_analyze_duration = AV_TIME_BASE * max_analyze_duration;
//        p_format_context->probesize = 100000;

        if (avformat_find_stream_info(p_format_context, NULL) < 0)
            throw std::runtime_error("avformat_find_stream_info() failed");

        m_duration = double(m_format_context->duration) / AV_TIME_BASE;
        if (m_format_context->start_time != static_cast<int64_t>(AV_NOPTS_VALUE))
            m_start = double(m_format_context->start_time) / AV_TIME_BASE;
        else
            m_start = 0;

        // TODO move this elsewhere
        m_clocks.reset(m_start);

        // Dump info to stderr
        av_dump_format(p_format_context, 0, filename.c_str(), false);

        // Find and open the first video and audio streams (note that audio stream is optional and only opened if possible)
        if ((m_video_index = av_find_best_stream(m_format_context.get(), AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0)) < 0)
            throw std::runtime_error("Could not open video stream");
        m_video_stream = m_format_context->streams[m_video_index];

        if ((m_audio_index = av_find_best_stream(m_format_context.get(), AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0)) >= 0)
            m_audio_stream = m_format_context->streams[m_audio_index];
        else
        {
            m_audio_stream = nullptr;
            m_audio_index = std::numeric_limits<unsigned int>::max();
        }

        m_video_decoder.open(m_video_stream);

        try
        {
            m_audio_decoder.open(m_audio_stream, parameters);
        }
        catch (const std::runtime_error & rerror)
        {
            OSG_WARN << "FFmpegImageStream::open audio failed, audio stream will be disabled: " << rerror.what() << std::endl;
        }
    }

    catch (const std::runtime_error & error)
    {
        OSG_WARN << "FFmpegImageStream::open : " << error.what() << std::endl;
        return false;
    }

    return true;
}



void FFmpegDecoder::close(bool waitForThreadToExit)
{
    flushAudioQueue();
    flushVideoQueue();

    m_audio_decoder.close(waitForThreadToExit);
    m_video_decoder.close(waitForThreadToExit);
}



bool FFmpegDecoder::readNextPacket()
{
    switch (m_state)
    {
    case NORMAL:
        return readNextPacketNormal();

    case PAUSE:
        return false;

    case END_OF_STREAM:
        return readNextPacketEndOfStream();

    case REWINDING:
        return readNextPacketRewinding();

    case SEEKING:
        return readNextPacketSeeking();

    default:
        OSG_FATAL << "unknown decoder state " << m_state << std::endl;
        assert(false);
        return false;
    }
}



void FFmpegDecoder::rewind()
{
    m_pending_packet.clear();

    flushAudioQueue();
    flushVideoQueue();
    rewindButDontFlushQueues();
}

void FFmpegDecoder::seek(double time)
{
    m_pending_packet.clear();

    flushAudioQueue();
    flushVideoQueue();
    seekButDontFlushQueues(time);
}

void FFmpegDecoder::pause()
{
    m_pending_packet.clear();

    flushAudioQueue();
    flushVideoQueue();
    m_state = PAUSE;
}


inline void FFmpegDecoder::flushAudioQueue()
{
    FFmpegPacketClear pc;
    m_audio_queue.flush(pc);
}



inline void FFmpegDecoder::flushVideoQueue()
{
    FFmpegPacketClear pc;
    m_video_queue.flush(pc);
}

bool FFmpegDecoder::readNextPacketNormal()
{
    AVPacket packet;

    if (!m_pending_packet)
    {
        bool end_of_stream = false;

        // Read the next frame packet
        int error = av_read_frame(m_format_context.get(), &packet);
        if (error < 0)
        {
            if (error == static_cast<int>(AVERROR_EOF) || m_format_context.get()->pb->eof_reached)
                end_of_stream = true;
            else
            {
                OSG_FATAL << "av_read_frame() returned " << AvStrError(error) << std::endl;
                throw std::runtime_error("av_read_frame() failed");
            }
        }

        if (end_of_stream)
        {
            // If we reach the end of the stream, change the decoder state
            if (loop())
            {
                m_clocks.reset(m_start);
                rewindButDontFlushQueues();
            }
            else
                m_state = END_OF_STREAM;

            return false;
        }
        else
        {
            m_pending_packet = FFmpegPacket(packet);
        }
    }

    // Send data packet
    if (m_pending_packet.type == FFmpegPacket::PACKET_DATA)
    {
        if (m_pending_packet.packet.stream_index == m_audio_index)
        {
            if (m_audio_queue.timedPush(m_pending_packet, 10)) {
                m_pending_packet.release();
                return true;
            }
        }
        else if (m_pending_packet.packet.stream_index == m_video_index)
        {
            if (m_video_queue.timedPush(m_pending_packet, 10)) {
                m_pending_packet.release();
                return true;
            }
        }
        else
        {
            m_pending_packet.clear();
            return true;
        }
    }

    return false;
}

bool FFmpegDecoder::readNextPacketEndOfStream()
{
    const FFmpegPacket packet(FFmpegPacket::PACKET_END_OF_STREAM);

    m_audio_queue.timedPush(packet, 10);
    m_video_queue.timedPush(packet, 10);

    return false;
}

bool FFmpegDecoder::readNextPacketRewinding()
{
    const FFmpegPacket packet(FFmpegPacket::PACKET_FLUSH);

    if (m_audio_queue.timedPush(packet, 10) && m_video_queue.timedPush(packet, 10))
        m_state = NORMAL;

    return false;
}

void FFmpegDecoder::rewindButDontFlushQueues()
{
    const AVRational AvTimeBaseQ = { 1, AV_TIME_BASE }; // = AV_TIME_BASE_Q

    const int64_t pos = int64_t(m_clocks.getStartTime() * double(AV_TIME_BASE));
    const int64_t seek_target = av_rescale_q(pos, AvTimeBaseQ, m_video_stream->time_base);

    int error = 0;
    if ((error = av_seek_frame(m_format_context.get(), m_video_index, seek_target, 0)) < 0)
    {
        OSG_FATAL << "av_seek_frame returned " << AvStrError(error) << std::endl;
        throw std::runtime_error("av_seek_frame failed()");
    }

    m_clocks.rewind();
    m_state = REWINDING;
}

bool FFmpegDecoder::readNextPacketSeeking()
{
    const FFmpegPacket packet(FFmpegPacket::PACKET_FLUSH);

    if (m_audio_queue.timedPush(packet, 10) && m_video_queue.timedPush(packet, 10))
        m_state = NORMAL;

    return false;
}

void FFmpegDecoder::seekButDontFlushQueues(double time)
{
    const AVRational AvTimeBaseQ = { 1, AV_TIME_BASE }; // = AV_TIME_BASE_Q

    const int64_t pos = int64_t(m_clocks.getStartTime() + time * double(AV_TIME_BASE));
    const int64_t seek_target = av_rescale_q(pos, AvTimeBaseQ, m_video_stream->time_base);

    m_clocks.setSeekTime(time);

    int error = 0;
    if ((error = av_seek_frame(m_format_context.get(), m_video_index, seek_target, 0)) < 0)
    {
        OSG_FATAL << "av_seek_frame() returned " << AvStrError(error) << std::endl;
        throw std::runtime_error("av_seek_frame failed()");
    }

    m_clocks.seek(time);
    m_state = SEEKING;
}



} // namespace osgFFmpeg
