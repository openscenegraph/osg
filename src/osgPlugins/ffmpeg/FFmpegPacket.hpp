
#ifndef HEADER_GUARD_OSGFFMPEG_FFMPEG_PACKET_H
#define HEADER_GUARD_OSGFFMPEG_FFMPEG_PACKET_H

#include "FFmpegHeaders.hpp"



namespace osgFFmpeg
{

    struct FFmpegPacket
    {

        enum Type
        {
            PACKET_DATA,
            PACKET_END_OF_STREAM,
            PACKET_FLUSH
        };


        FFmpegPacket() :
            type(PACKET_DATA)
        {
            packet.data = 0;
        }

        explicit FFmpegPacket(const Type t) :
            type(t)
        {
            packet.data = 0;
        }

        explicit FFmpegPacket(const AVPacket & p) :
            packet(p),
            type(PACKET_DATA)
        {

        }

        void clear()
        {
            if (packet.data != 0)
                av_packet_unref(&packet);

            release();
        }

        void release()
        {
            packet.data = 0;
            type = PACKET_DATA;
        }

        bool valid() const
        {
            return (type != PACKET_DATA) ^ (packet.data != 0);
        }

        bool operator ! () const
        {
            return ! valid();
        }

        AVPacket    packet;
        Type        type;
    };

    struct FFmpegPacketClear
    {
        void operator () (FFmpegPacket & packet) const
        {
            packet.clear();
        }
    };

}



#endif // HEADER_GUARD_OSGFFMPEG_FFMPEG_PACKET_H
