#ifndef HEADER_GUARD_OSGFFMPEG_FFMPEG_DECODER_AUDIO_H
#define HEADER_GUARD_OSGFFMPEG_FFMPEG_DECODER_AUDIO_H

extern "C" {
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
}

#include <OpenThreads/Thread>
#include <osg/Timer>
#include <osg/AudioStream>
#include "FFmpegClocks.hpp"
#include "FFmpegPacket.hpp"
#include "FFmpegParameters.hpp"
#include "BoundedMessageQueue.hpp"

namespace osgFFmpeg {



class FFmpegDecoderAudio : public OpenThreads::Thread
{
public:

    typedef BoundedMessageQueue<FFmpegPacket> PacketQueue;
    typedef void (*PublishFunc)(const FFmpegDecoderAudio &decoder, void *user_data);

    FFmpegDecoderAudio(PacketQueue &packets, FFmpegClocks &clocks);
    ~FFmpegDecoderAudio();

    void open(AVStream *stream, FFmpegParameters *parameters);
    void pause(bool pause);
    void close(bool waitForThreadToExit);

    void setVolume(float volume);
    float getVolume() const;

    virtual void run();

    void setAudioSink(osg::ref_ptr<osg::AudioSink> audio_sink);
    void fillBuffer(void *buffer, size_t size);

    bool validContext() const;
    int frequency() const;
    int nbChannels() const;
    osg::AudioStream::SampleFormat sampleFormat() const;

private:

    typedef osg::ref_ptr<osg::AudioSink> SinkPtr;
    typedef std::vector<uint8_t> Buffer;

    void decodeLoop();
    void adjustBufferEndPts(size_t buffer_size);
    size_t decodeFrame(void *buffer, size_t size);


    PacketQueue &m_packets;
    FFmpegClocks &m_clocks;
    AVStream *m_stream;
    AVCodecContext *m_context;
    FFmpegPacket m_packet;
    const uint8_t *m_packet_data;
    int m_bytes_remaining;

    Buffer m_audio_buffer;
    size_t m_audio_buf_size;
    size_t m_audio_buf_index;

    int m_in_sample_rate;
    int m_in_nb_channels;
    AVSampleFormat m_in_sample_format;
    int m_out_sample_rate;
    int m_out_nb_channels;
    AVSampleFormat m_out_sample_format;

    SinkPtr m_audio_sink;

    osg::Timer m_pause_timer;

    bool m_end_of_stream;
    bool m_paused;
    volatile bool m_exit;

    SwrContext *m_swr_context; // Sw resampling context
};





inline bool FFmpegDecoderAudio::validContext() const
{
    return m_context != nullptr;
}


inline int FFmpegDecoderAudio::frequency() const
{
    return m_out_sample_rate;
}


inline int FFmpegDecoderAudio::nbChannels() const
{
    return m_out_nb_channels;
}

} // namespace osgFFmpeg


#endif // HEADER_GUARD_OSGFFMPEG_FFMPEG_DECODER_AUDIO_H

