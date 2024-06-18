#ifndef HEADER_GUARD_OSGFFMPEG_FFMPEG_DECODER_VIDEO_H
#define HEADER_GUARD_OSGFFMPEG_FFMPEG_DECODER_VIDEO_H


#include "FFmpegHeaders.hpp"
#include "BoundedMessageQueue.hpp"
#include "FFmpegClocks.hpp"
#include "FFmpegPacket.hpp"

#include <OpenThreads/Thread>
#include <vector>

namespace osgFFmpeg {

class FramePtr
{
    public:  
    
        typedef AVFrame T;
    
        explicit FramePtr() : _ptr(nullptr) {}
        explicit FramePtr(T* ptr) : _ptr(ptr) {}
        
        ~FramePtr()
        {
            cleanup();
        }
        
        T* get() { return _ptr; }

        T * operator-> () const // never throws
        {
            return _ptr;
        }

        void reset(T* ptr) 
        {
            if (ptr==_ptr) return;
            cleanup();
            _ptr = ptr;
        }

        void cleanup()
        {
            if (_ptr) av_frame_free(&_ptr);
            _ptr = nullptr;
        }
        
    protected:
        T* _ptr;
};

class FFmpegDecoderVideo : public OpenThreads::Thread
{
public:

    typedef BoundedMessageQueue<FFmpegPacket> PacketQueue;
    typedef void (* PublishFunc) (const FFmpegDecoderVideo & decoder, void * user_data);

    FFmpegDecoderVideo(PacketQueue & packets, FFmpegClocks & clocks);
    ~FFmpegDecoderVideo();

    void open(AVStream * stream);
    void pause(bool pause);
    void close(bool waitForThreadToExit);

    virtual void run();

    void setUserData(void * user_data);
    void setPublishCallback(PublishFunc function);

    int width() const;
    int height() const;
    float pixelAspectRatio() const;
    bool alphaChannel() const;
    double frameRate() const;
    const uint8_t * image() const;

private:

    typedef std::vector<uint8_t> Buffer;

    void decodeLoop();
    void findAspectRatio();
    void publishFrame(double delay, bool audio_disabled);
    double synchronizeVideo(double pts);
    void yuva420pToRgba(AVFrame* dst, AVFrame* src, int width, int height);

    int convert(AVFrame* dst, int dst_pix_fmt, AVFrame* src,
                int src_pix_fmt, int src_width, int src_height);

    static int getBuffer(AVCodecContext * context, AVFrame * picture, int flags);
    static void freeBuffer(void * opaque, uint8_t *data);

    PacketQueue &           m_packets;
    FFmpegClocks &          m_clocks;
    AVStream *              m_stream;
    AVCodecContext *        m_context;
    const AVCodec*          m_codec;
    const uint8_t *         m_packet_data;
    int                     m_bytes_remaining;
    int64_t                 m_packet_pts;
    
    FramePtr                m_frame;
    FramePtr                m_frame_rgba;
    Buffer                  m_buffer_rgba[2];
    int                     m_writeBuffer;

    void *                  m_user_data;
    PublishFunc             m_publish_func;

    double                  m_frame_rate;
    float                   m_pixel_aspect_ratio;
    int                     m_width;
    int                     m_height;
    size_t                  m_next_frame_index;
    bool                    m_alpha_channel;

    bool                    m_paused;
    volatile bool           m_exit;
    
#ifdef USE_SWSCALE    
    struct SwsContext *     m_swscale_ctx;
#endif
};





inline void FFmpegDecoderVideo::setUserData(void * const user_data)
{
    m_user_data = user_data;
}


inline void FFmpegDecoderVideo::setPublishCallback(const PublishFunc function)
{
    m_publish_func = function;
}


inline int FFmpegDecoderVideo::width() const
{
    return m_width;
}


inline int FFmpegDecoderVideo::height() const
{
    return m_height;
}


inline float FFmpegDecoderVideo::pixelAspectRatio() const
{
    return m_pixel_aspect_ratio;
}


inline bool FFmpegDecoderVideo::alphaChannel() const
{
    return m_alpha_channel;
}


inline double FFmpegDecoderVideo::frameRate() const
{
    return m_frame_rate;
}


inline const uint8_t * FFmpegDecoderVideo::image() const
{
    return &((m_buffer_rgba[1-m_writeBuffer])[0]);
}



} // namespace osgFFmpeg



#endif // HEADER_GUARD_OSGFFMPEG_FFMPEG_DECODER_VIDEO_H
