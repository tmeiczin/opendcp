/*
    OpenDCP: Builds Digital Cinema Packages
    Copyright (c) 2010-2014 Terrence Meiczinger, All Rights Reserved

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <opendcp.h>
#include <opendcp_image.h>
#include <opendcp_encoder.h>

#include <stdio.h>


typedef struct {
    AVFormatContext   *p_format_ctx;
    AVCodecContext    *p_codec_ctx ;
    AVCodec           *pCodec;
    AVDictionary      *options;
    struct SwsContext *sws_ctx;
    int                v_stream;
} av_context_t;

void video_decoder_delete(av_context_t *av);
int  video_decoder_create(av_context_t *av, const char *file);

static int copy_frame(const AVFrame *frame, opendcp_image_t *image) {
    int i, x, y;
    int index = 0;
    uint8_t *data  = (uint8_t *)frame->data[0];

    for (y = 0; y < image->h; y++) {
        for (x = 0; x < frame->linesize[0]; x += 3) {
            i = frame->linesize[0] * y + x;

            /* rounded to 12 bits */
            image->component[0].data[index] = data[i + 0] << 4; // R
            image->component[1].data[index] = data[i + 1] << 4; // G
            image->component[2].data[index] = data[i + 2] << 4; // B
            index++;
        }
    }

    return 0;
}

static int copy_frame_16(const AVFrame *frame, opendcp_image_t *image) {
    int i, x, y;
    int index = 0;
    uint8_t *data  = (uint8_t *)frame->data[0];

    for (y = 0; y < image->h; y++) {
        for (x = 0; x < frame->linesize[0]; x += 6) {
            i = frame->linesize[0] * y + x;

            /* rounded to 12 bits */
            image->component[0].data[index] = ((data[i + 1] << 8) | data[i + 0]) >> 4; // R
            image->component[1].data[index] = ((data[i + 3] << 8) | data[i + 2]) >> 4; // G
            image->component[2].data[index] = ((data[i + 5] << 8) | data[i + 4]) >> 4; // B
            index++;
        }
    }

    return 0;
}

int video_decoder_find(const char *file) {
    av_context_t    *av;

    av = malloc(sizeof(av_context_t));

    if (video_decoder_create(av, file) != OPENDCP_NO_ERROR) {
        return 0;
    }

    video_decoder_delete(av);

    return 1;
}

void video_decoder_delete(av_context_t *av) {
    /* close the codec */
    if (av->p_codec_ctx) {
        avcodec_close(av->p_codec_ctx);
    }

    /* close the video file */
    if (av->p_format_ctx) {
        avformat_close_input(&av->p_format_ctx);
    }

    if (av) {
        free(av);
    }
}

int video_decoder_create(av_context_t *av, const char *file) {
    unsigned int    i;

    av->p_format_ctx = NULL;
    av->p_codec_ctx = NULL;
    av->pCodec = NULL;
    av->options = NULL;
    av->sws_ctx = NULL;

    av->v_stream = -1;

    /* register all formats and codecs */
    av_register_all();

    /* open file */
    if (avformat_open_input(&av->p_format_ctx, file, NULL, NULL) != 0) {
        OPENDCP_LOG(LOG_ERROR, "unable to open input file %s", file);
        return OPENDCP_ERROR;
    }

    /* get stream information */
    if(avformat_find_stream_info(av->p_format_ctx, NULL) < 0) {
        OPENDCP_LOG(LOG_ERROR, "could not get stream information %s", file);
        return OPENDCP_ERROR;
    }

    /* locate video stream */
    for (i = 0; i < av->p_format_ctx->nb_streams; i++) {
        if (av->p_format_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            av->v_stream = i;
            break;
        }
    }

    /* check if stream found */
    if (av->v_stream == -1) {
        OPENDCP_LOG(LOG_ERROR, "no video stream found %s", file);
        return OPENDCP_ERROR;
    }

    /* get a pointer to the codec context for the video stream */
    av->p_codec_ctx = av->p_format_ctx->streams[av->v_stream]->codec;

    /* find the decoder for the video stream */
    av->pCodec = avcodec_find_decoder(av->p_codec_ctx->codec_id);

    if(av->pCodec == NULL) {
        OPENDCP_LOG(LOG_ERROR, "unsupported codec");
        return OPENDCP_ERROR;
    }

    printf("time: %lld\n", av->p_format_ctx->duration);
    printf("frames: %lld\n", av->p_format_ctx->streams[i]->nb_frames);

    return OPENDCP_NO_ERROR;
}

void save_frame(opendcp_t *opendcp, AVCodecContext *av_ctx, const AVFrame *frame, int frame_number) {
    char dfile[255];
    opendcp_image_t *image = 00;

    OPENDCP_LOG(LOG_DEBUG, "allocating opendcp image");
    image = opendcp_image_create(3, av_ctx->width, av_ctx->height);

    OPENDCP_LOG(LOG_DEBUG, "copying frame from video");
    copy_frame(frame, image);

    OPENDCP_LOG(LOG_DEBUG, "encoding image");
    sprintf(dfile, "frame_%08d.j2c", frame_number);
    opendcp_encode_ragnarok(opendcp, image, dfile);
    opendcp_image_free(image);
}

int decode_video(opendcp_t *opendcp,  const char *file) {
    av_context_t      *av;
    AVFrame           *p_frame = NULL;
    AVFrame           *p_frame_rgb = NULL;
    AVPacket          packet;
    unsigned int      i;
    int               frame_done;
    int               n_bytes;
    uint8_t           *buffer = NULL;

    OPENDCP_LOG(LOG_INFO, "creating video decoder");
    av = malloc(sizeof(av_context_t));

    if (video_decoder_create(av, file) != OPENDCP_NO_ERROR) {
        OPENDCP_LOG(LOG_ERROR, "could not create video decoder: %s", file);
        video_decoder_delete(av);

        return OPENDCP_ERROR;
    }

    /* start codec */
    if (avcodec_open2(av->p_codec_ctx, av->pCodec, &av->options) < 0) {
        OPENDCP_LOG(LOG_ERROR, "could not open codec");
        video_decoder_delete(av);
        return OPENDCP_ERROR;
    }

    /* determine required buffer size and allocate buffer */
    n_bytes = avpicture_get_size(PIX_FMT_RGB24, av->p_codec_ctx->width, av->p_codec_ctx->height);
    buffer = (uint8_t *)av_malloc(n_bytes * sizeof(uint8_t));

    av->sws_ctx =
        sws_getContext
        (
            av->p_codec_ctx->width,
            av->p_codec_ctx->height,
            av->p_codec_ctx->pix_fmt,
            av->p_codec_ctx->width,
            av->p_codec_ctx->height,
            PIX_FMT_RGB24,
            SWS_BILINEAR,
            NULL,
            NULL,
            NULL
        );

    /* allocate video frame */
    p_frame = av_frame_alloc();

    /* allocate an AVFrame structure */
    p_frame_rgb = av_frame_alloc();

    if (p_frame_rgb == NULL) {
        return OPENDCP_ERROR;
    }

    /* Assign appropriate parts of buffer to image planes in p_frame_rgb */
    avpicture_fill((AVPicture *)p_frame_rgb, buffer, PIX_FMT_RGB24, av->p_codec_ctx->width, av->p_codec_ctx->height);

    /* Read frames and re-encode */
    i = 1;
    while(av_read_frame(av->p_format_ctx, &packet) >= 0) {
        /* is this part of the stream */
        if (packet.stream_index != av->v_stream) {
            continue;
        }

        /* Decode video frame */
        avcodec_decode_video2(av->p_codec_ctx, p_frame, &frame_done, &packet);

        /* check if valid frame */
        if (frame_done) {
            /* convert the image from its native format to RGB */
            sws_scale
            (
                av->sws_ctx,
                (uint8_t const * const *)p_frame->data,
                p_frame->linesize,
                0,
                av->p_codec_ctx->height,
                p_frame_rgb->data,
                p_frame_rgb->linesize
            );

            /* save the frame to disk */
            save_frame(opendcp, av->p_codec_ctx, p_frame_rgb, i++);

            /* emit callback, if set */
            if (opendcp->j2k.frame_done.callback) {
                opendcp->j2k.frame_done.callback(opendcp->j2k.frame_done.argument);
            }
        }

        /* free the packet that was allocated by av_read_frame */
        av_free_packet(&packet);
    }

    /* free the RGB image */
    av_free(buffer);
    av_free(p_frame_rgb);

    /* free the original frame */
    av_free(p_frame);

    /* delete decoder */;
    video_decoder_delete(av);

    return OPENDCP_NO_ERROR;
}
