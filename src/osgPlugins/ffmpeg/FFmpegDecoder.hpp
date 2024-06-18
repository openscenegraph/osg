#ifndef HEADER_GUARD_OSGFFMPEG_FFMPEG_DECODER_H
#define HEADER_GUARD_OSGFFMPEG_FFMPEG_DECODER_H

#include "FFmpegDecoderAudio.hpp"
#include "FFmpegDecoderVideo.hpp"

#include <osg/Notify>

namespace osgFFmpeg {

class FFmpegParameters;

class FormatContextPtr
{
public:
    typedef AVFormatContext T;

    explicit FormatContextPtr() : _ptr(nullptr) {}
    explicit FormatContextPtr(T* ptr) : _ptr(ptr) {}

    ~FormatContextPtr()
    {
        cleanup();
    }

    T* get() const { return _ptr; }
    T** getPtr() { return &_ptr; }

    operator T*() const { return _ptr; }

    FormatContextPtr& operator=(T* ptr)
    {
        reset(ptr);
        return *this;
    }

    bool operator==(std::nullptr_t) const { return _ptr == nullptr; }
    bool operator!=(std::nullptr_t) const { return _ptr != nullptr; }

    T* operator->() const // never throws
    {
        return _ptr;
    }

    void reset(T* ptr)
    {
        if (ptr == _ptr) return;
        cleanup();
        _ptr = ptr;
    }

    void cleanup()
    {
        if (_ptr)
        {
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(53, 17, 0)
            OSG_NOTICE << "Calling avformat_close_input(" << &_ptr << ")" << std::endl;
            avformat_close_input(&_ptr);
#else
            OSG_NOTICE << "Calling av_close_input_file(" << _ptr << ")" << std::endl;
            av_close_input_file(_ptr);
#endif
        }
        _ptr = nullptr;
    }

protected:
    T* _ptr;
};

class FFmpegDecoder : public osg::Referenced
{
public:
    FFmpegDecoder();
    ~FFmpegDecoder();

    bool open(const std::string& filename, FFmpegParameters* parameters);
    void close(bool waitForThreadToExit = true);

    bool readNextPacket();
    void rewind();
    void seek(double time);
    void pause();

    void loop(bool loop);
    bool loop() const;

    double creation_time() const;
    double duration() const;
    double reference();

    FFmpegDecoderAudio& audio_decoder();
    FFmpegDecoderVideo& video_decoder();
    FFmpegDecoderAudio const& audio_decoder() const;
    FFmpegDecoderVideo const& video_decoder() const;

protected:
    enum State
    {
        NORMAL,
        PAUSE,
        END_OF_STREAM,
        REWINDING,
        SEEKING
    };

    typedef BoundedMessageQueue<FFmpegPacket> PacketQueue;

    void flushAudioQueue();
    void flushVideoQueue();
    bool readNextPacketNormal();
    bool readNextPacketEndOfStream();
    bool readNextPacketRewinding();
    bool readNextPacketSeeking();
    bool readNextPacketPause();
    void rewindButDontFlushQueues();
    void seekButDontFlushQueues(double time);

    FormatContextPtr m_format_context;
    AVStream* m_audio_stream;
    AVStream* m_video_stream;

    int m_audio_index;
    int m_video_index;

    FFmpegClocks m_clocks;
    FFmpegPacket m_pending_packet;
    PacketQueue m_audio_queue;
    PacketQueue m_video_queue;

    FFmpegDecoderAudio m_audio_decoder;
    FFmpegDecoderVideo m_video_decoder;

    double m_duration;
    double m_start;

    State m_state;
    bool m_loop;
};

inline void FFmpegDecoder::loop(const bool loop)
{
    m_loop = loop;
}

inline bool FFmpegDecoder::loop() const
{
    return m_loop;
}

inline double FFmpegDecoder::creation_time() const
{
    if (m_format_context) return m_format_context->start_time;
    else return HUGE_VAL;
}

inline double FFmpegDecoder::duration() const
{
    return double(m_format_context->duration) / AV_TIME_BASE;
}

inline double FFmpegDecoder::reference()
{
    return m_clocks.getCurrentTime();
}

inline FFmpegDecoderAudio& FFmpegDecoder::audio_decoder()
{
    return m_audio_decoder;
}

inline FFmpegDecoderVideo& FFmpegDecoder::video_decoder()
{
    return m_video_decoder;
}

inline FFmpegDecoderAudio const& FFmpegDecoder::audio_decoder() const
{
    return m_audio_decoder;
}

inline FFmpegDecoderVideo const& FFmpegDecoder::video_decoder() const
{
    return m_video_decoder;
}

} // namespace osgFFmpeg

#endif // HEADER_GUARD_OSGFFMPEG_FFMPEG_DECODER_H

