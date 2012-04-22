/* avbin_dump.c
 * Copyright 2012 AVbin Team
 *
 * This file is part of AVbin.
 *
 * AVbin is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 3 of
 * the License, or (at your option) any later version.
 *
 * AVbin is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

/* Example use of AVbin.
 *
 * Prints out stream details, then exits.
 *
 * TODO: Optionally print verbose decoding information.
 * TODO: Clean up, comment.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include <avbin.h>

int main(int argc, char** argv)
{
    if (avbin_init()) 
        exit(-1);

    printf("AVbin %d, built with Libav %s on commit %s.\n", avbin_get_version(), avbin_get_libav_version(), avbin_get_libav_commit());

    if (argc < 2) 
        exit(-1);

    AVbinFile* file = avbin_open_filename(argv[1]);
    if (!file) 
        exit(-1);

    AVbinFileInfo fileinfo;
    fileinfo.structure_size = sizeof(fileinfo);

    if (avbin_file_info(file, &fileinfo)) 
        exit(-2);

    printf("#streams %d\n",fileinfo.n_streams);
    printf("duration %lldus (%lld:%02lld:%02lld)\n",
        fileinfo.duration,
        fileinfo.duration / (1000000L * 60 * 60),
        (fileinfo.duration / (1000000L * 60)) % 60,
        (fileinfo.duration / 1000000L) % 60);

    printf("Title: %s\n", fileinfo.title);
    printf("Author: %s\n", fileinfo.author);
    printf("Copyright: %s\n", fileinfo.copyright);
    printf("Comment: %s\n", fileinfo.comment);
    printf("Album: %s\n", fileinfo.album);
    printf("Track: %d\n", fileinfo.track);
    printf("Year: %d\n", fileinfo.year);
    printf("Genre: %s\n", fileinfo.genre);

    AVbinStream* video_stream = NULL;
    AVbinStream* audio_stream = NULL;
    int video_stream_index = -1;
    int audio_stream_index = -1;
    int width, height;
    int have_frame_rate = avbin_have_feature("frame_rate");

    int stream_index;
    for (stream_index=0; stream_index<fileinfo.n_streams; stream_index++)
    {
        AVbinStreamInfo8 streaminfo;
        streaminfo.structure_size = sizeof(streaminfo);

        avbin_stream_info(file, stream_index, (AVbinStreamInfo *) &streaminfo);

        if (streaminfo.type == AVBIN_STREAM_TYPE_VIDEO)
        {
            printf("video stream at %d, height %d, width %d\n",stream_index,streaminfo.video.height,streaminfo.video.width);
            if (have_frame_rate)
                printf("frame rate %d / %d (approximately %.2f)\n", 
                       streaminfo.video.frame_rate_num, 
                       streaminfo.video.frame_rate_den,
                       streaminfo.video.frame_rate_num / (float)
                            streaminfo.video.frame_rate_den);
            width = streaminfo.video.width;
            height = streaminfo.video.height;
            video_stream_index = stream_index;
            video_stream = avbin_open_stream(file, stream_index);
        }
        if (streaminfo.type == AVBIN_STREAM_TYPE_AUDIO)
        {
            printf("audio stream at %d, rate %d, bits %d, chan %d\n",stream_index,streaminfo.audio.sample_rate,streaminfo.audio.sample_bits,streaminfo.audio.channels);
            audio_stream_index = stream_index;
            audio_stream = avbin_open_stream(file, stream_index);
        }
    }

    exit(0);

    /* TODO enable the following optionally */

    AVbinPacket packet;
    packet.structure_size = sizeof(packet);

    while (!avbin_read(file, &packet))
    {
        if (packet.stream_index == video_stream_index)
        {
            uint8_t* video_buffer = (uint8_t*) malloc(width*height*3);
            if (avbin_decode_video(video_stream, packet.data, packet.size,video_buffer)<=0) printf("could not read video packet\n");
            else printf("read video frame\n");

            // do something with video_buffer

            free(video_buffer);
        }
        if (packet.stream_index == audio_stream_index)
        {
            uint8_t audio_buffer[1024*1024];
            int bytesleft = sizeof(audio_buffer);
            int bytesout = bytesleft;
            int bytesread;
            uint8_t* audio_data = audio_buffer;
            while ((bytesread = avbin_decode_audio(audio_stream, packet.data, packet.size, audio_data, &bytesout)) > 0)
            {
                packet.data += bytesread;
                packet.size -= bytesread;
                audio_data += bytesout;
                bytesleft -= bytesout;
                bytesout = bytesleft;
            }

            int nrBytes = audio_data-audio_buffer;

            printf("read audio packet of size %d bytes\n",nrBytes);

            // do something with audio_buffer ... but don't free it since it is a local array
        }
    }

    if (video_stream) avbin_close_stream(video_stream);
    if (audio_stream) avbin_close_stream(audio_stream);

    avbin_close_file(file);
}
