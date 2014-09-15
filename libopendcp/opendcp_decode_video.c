#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <opendcp.h>
#include <opendcp_image.h>
#include <opendcp_encoder.h>

#include <stdio.h>


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

void save_frame(opendcp_t *opendcp, AVCodecContext *av_ctx, const AVFrame *frame, int frame_number) {
    char dfile[255];
    opendcp_image_t *image = 00;

    OPENDCP_LOG(LOG_DEBUG, "allocating opendcp image");
    image = opendcp_image_create(3, av_ctx->width, av_ctx->height);

    OPENDCP_LOG(LOG_DEBUG, "copying frame from video");
    copy_frame(frame, image);

    OPENDCP_LOG(LOG_DEBUG, "encoding eimage");
    sprintf(dfile, "frame%08d.j2c", frame_number);
    opendcp_encode_ragnarok(opendcp, image, dfile);
    opendcp_image_free(image);
}

int decode_video(opendcp_t *opendcp,  char *sfile) {
    AVFormatContext   *p_format_ctx = NULL;
    AVCodecContext    *p_codec_ctx = NULL;
    AVCodec           *pCodec = NULL;
    AVFrame           *p_frame = NULL;
    AVFrame           *p_frame_rgb = NULL;
    AVPacket          packet;
    unsigned int      i;
    int               v_stream, frame_done;
    int               n_bytes;
    uint8_t           *buffer = NULL;

    AVDictionary      *options = NULL;
    struct SwsContext *sws_ctx = NULL;

    /* register all formats and codecs */
    av_register_all();

    /* open file */
    if (avformat_open_input(&p_format_ctx, sfile, NULL, NULL) != 0) {
        OPENDCP_LOG(LOG_ERROR, "unable to open input file %s", sfile);
        return OPENDCP_ERROR;
    }

    /* get stream information */
    if(avformat_find_stream_info(p_format_ctx, NULL) < 0) {
        OPENDCP_LOG(LOG_ERROR, "could not get stream information %s", sfile);
        return OPENDCP_ERROR;
    }

    /* print information about file onto standard error */
    // av_dump_format(p_format_ctx, 0, sfile, 0);

    /* locate video stream */
    v_stream = -1;

    for (i = 0; i < p_format_ctx->nb_streams; i++) {
        if (p_format_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            v_stream = i;
            break;
        }
    }

    /* check if stream found */
    if (v_stream == -1) {
        OPENDCP_LOG(LOG_ERROR, "no video stream found %s", sfile);
        return OPENDCP_ERROR;
    }

    /* get a pointer to the codec context for the video stream */
    p_codec_ctx = p_format_ctx->streams[v_stream]->codec;

    /* find the decoder for the video stream */
    pCodec = avcodec_find_decoder(p_codec_ctx->codec_id);

    if(pCodec == NULL) {
        OPENDCP_LOG(LOG_ERROR, "unsupported codec");
        return OPENDCP_ERROR;
    }

    /* start codec */
    if (avcodec_open2(p_codec_ctx, pCodec, &options) < 0) {
        OPENDCP_LOG(LOG_ERROR, "could not open codec");
        return OPENDCP_ERROR;
    }

    /* allocate video frame */
    p_frame = av_frame_alloc();

    /* allocate an AVFrame structure */
    p_frame_rgb = av_frame_alloc();

    if (p_frame_rgb == NULL) {
        return OPENDCP_ERROR;
    }

    /* determine required buffer size and allocate buffer */
    n_bytes = avpicture_get_size(PIX_FMT_RGB24, p_codec_ctx->width, p_codec_ctx->height);
    buffer = (uint8_t *)av_malloc(n_bytes * sizeof(uint8_t));

    sws_ctx =
        sws_getContext
        (
            p_codec_ctx->width,
            p_codec_ctx->height,
            p_codec_ctx->pix_fmt,
            p_codec_ctx->width,
            p_codec_ctx->height,
            PIX_FMT_RGB24,
            SWS_BILINEAR,
            NULL,
            NULL,
            NULL
        );

    /* Assign appropriate parts of buffer to image planes in p_frame_rgb */
    avpicture_fill((AVPicture *)p_frame_rgb, buffer, PIX_FMT_RGB24, p_codec_ctx->width, p_codec_ctx->height);

    /* Read frames and save first five frames to disk */
    i = 1;

    while(av_read_frame(p_format_ctx, &packet) >= 0) {
        /* is this part of the stream */
        if (packet.stream_index != v_stream) {
            continue;
        }

        /* Decode video frame */
        avcodec_decode_video2(p_codec_ctx, p_frame, &frame_done, &packet);

        /* check if valid frame */
        if (frame_done) {
            /* convert the image from its native format to RGB */
            sws_scale
            (
                sws_ctx,
                (uint8_t const * const *)p_frame->data,
                p_frame->linesize,
                0,
                p_codec_ctx->height,
                p_frame_rgb->data,
                p_frame_rgb->linesize
            );

            /* save the frame to disk */
            save_frame(opendcp, p_codec_ctx, p_frame_rgb, i++);
 
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

    /* close the codec */
    avcodec_close(p_codec_ctx);

    /* close the video file */
    avformat_close_input(&p_format_ctx);

    return OPENDCP_NO_ERROR;
}
