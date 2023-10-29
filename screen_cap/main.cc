#include <iostream>
#include <csignal>

#include "log.hpp"

extern "C"
{
#include <libavdevice/avdevice.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/error.h>
#include <libswscale/swscale.h>
}

int quit;
void signal_handler(int sig)
{
    quit = 1;
}

int main()
{
    // 初始化停止信号
    std::signal(SIGINT, signal_handler);

    // 初始化 FFmpeg
    avdevice_register_all();
    avformat_network_init();
    int ret = 0;
    char err_buf[25] = "\0";

    // 打开输入设备（屏幕捕获）
    AVFormatContext *input_ctx = avformat_alloc_context();
    AVInputFormat *input_format = av_find_input_format("x11grab");
    if (!input_format)
    {
        logging("x11grab not found");
        return -1;
    }
    AVDictionary *options = NULL;
    av_dict_set(&options, "framerate", "60", 0);         // 设置帧率
    av_dict_set(&options, "video_size", "1920x1080", 0); // 设置视频尺寸
    av_dict_set(&options, "probesize", "100M", 0);       // 估计帧率

    ret = avformat_open_input(&input_ctx, ":0.0", input_format, &options);
    if (ret != 0)
    {
        av_strerror(ret, err_buf, 25);
        logging("ERROR: open input device error: %s", err_buf);

        return -1;
    }

    ret = avformat_find_stream_info(input_ctx, NULL);
    if (ret < 0)
    {
        av_strerror(ret, err_buf, 25);
        logging("ERROR: find stream info error: %s", err_buf);

        return -1;
    }

    // 打开视频流
    int video_stream_index = av_find_best_stream(input_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (video_stream_index == -1)
    {
        logging("no video stream");
        return -1;
    }
    AVStream *in_stream = input_ctx->streams[video_stream_index];

    av_dump_format(input_ctx, video_stream_index, ":0.0", 0);

    // 设置解码器
    AVCodec *decoder = avcodec_find_decoder(in_stream->codecpar->codec_id);
    AVCodecContext *decoder_ctx = avcodec_alloc_context3(decoder);

    ret = avcodec_parameters_to_context(decoder_ctx, in_stream->codecpar);
    if (ret < 0)
    {
        av_strerror(ret, err_buf, 25);
        logging("copy decoder context error: %s", err_buf);

        return -1;
    }

    // decoder_ctx->height = 1920;
    // decoder_ctx->width = 1080;
    decoder_ctx->pix_fmt = AV_PIX_FMT_YUV420P;

    ret = avcodec_open2(decoder_ctx, decoder, NULL);
    if (ret < 0)
    {
        av_strerror(ret, err_buf, 25);
        logging("open decoder error: %s", err_buf);

        return -1;
    }

    // 打开输出文件
    const char *filename = "./desk.mp4";
    AVFormatContext *output_ctx = avformat_alloc_context();

    ret = avformat_alloc_output_context2(&output_ctx, NULL, NULL, filename);
    if (ret < 0)
    {
        av_strerror(ret, err_buf, 25);
        logging("alloc output context error: %s", err_buf);

        return -1;
    }

    // 设置编码器
    AVCodec *encoder = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!encoder)
    {
        logging("can not find encoder");
        return -1;
    }
    AVCodecContext *encoder_ctx = avcodec_alloc_context3(encoder);

    // 添加输出流
    AVStream *out_stream = avformat_new_stream(output_ctx, encoder);
    if (!out_stream)
    {
        logging("add out stream failed");
        return -1;
    }

    encoder_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    encoder_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
    encoder_ctx->bit_rate = 850000;
    encoder_ctx->height = decoder_ctx->height / 3;
    encoder_ctx->width = decoder_ctx->width / 3;
    encoder_ctx->framerate = av_make_q(1, 24);
    encoder_ctx->time_base = av_make_q(24, 1);

    ret = avcodec_parameters_from_context(out_stream->codecpar, encoder_ctx);
    // ret = avcodec_parameters_to_context(encoder_ctx, output_ctx->streams[0]->codecpar);
    if (ret < 0)
    {
        av_strerror(ret, err_buf, 25);
        logging("copy encoder context error: %s", err_buf);

        return -1;
    }

    ret = avcodec_open2(encoder_ctx, encoder, NULL);
    if (ret < 0)
    {
        av_strerror(ret, err_buf, 25);
        logging("open encoder error: %s", err_buf);

        return -1;
    }

    av_dump_format(output_ctx, 0, "./desk.mp4", 1);

    ret = avio_open(&(output_ctx->pb), filename, AVIO_FLAG_WRITE);
    if (ret < 0)
    {
        av_strerror(ret, err_buf, 25);
        logging("can not open output file: %s", err_buf);

        return -1;
    }

    // 设置缩放上下文
    SwsContext *sws_ctx = sws_getContext(decoder_ctx->width, decoder_ctx->height, decoder_ctx->pix_fmt, encoder_ctx->width, encoder_ctx->height, encoder_ctx->pix_fmt, SWS_BICUBIC, NULL, NULL, NULL);
    if (!sws_ctx)
    {
        logging("set sws context error");
        return -1;
    }

    // 开始录制
    AVPacket *pkt = av_packet_alloc();
    if (!pkt)
    {
        logging("packet alloc error");
        return -1;
    }

    AVFrame *in_frame = av_frame_alloc();
    if (!in_frame)
    {
        logging("in_frame alloc error");
        return -1;
    }
    in_frame->height = decoder_ctx->height;
    in_frame->width = decoder_ctx->width;
    in_frame->format = decoder_ctx->pix_fmt;
    ret = av_frame_get_buffer(in_frame, 0);
    if (ret < 0)
    {
        logging("input frame get buffer error");
        return -1;
    }
    logging("input frame is writable: %d", av_frame_is_writable(in_frame));

    AVFrame *out_frame = av_frame_alloc();
    if (!out_frame)
    {
        logging("out_frame alloc error");
        return -1;
    }
    out_frame->height = encoder_ctx->height;
    out_frame->width = encoder_ctx->width;
    out_frame->format = encoder_ctx->pix_fmt;
    ret = av_frame_get_buffer(out_frame, 0);
    if (ret < 0)
    {
        logging("output frame get buffer error");
        return -1;
    }
    logging("output frame is writable: %d", av_frame_is_writable(out_frame));

    ret = avformat_write_header(output_ctx, NULL);
    if (ret < 0)
    {
        av_strerror(ret, err_buf, 25);
        logging("write file header error: %s", err_buf);

        return -1;
    }

    logging("write head ok");

    while (av_read_frame(input_ctx, pkt) >= 0)
    {
        if (quit)
        {
            break;
        }

        if (pkt->stream_index == video_stream_index)
        {
            ret = avcodec_send_packet(decoder_ctx, pkt);
            if (ret < 0)
            {
                logging("send pkt error");
                return -1;
            }

            ret = avcodec_receive_frame(decoder_ctx, in_frame);
            if (ret < 0)
            {
                logging("receive in_frame error");
                return -1;
            }

            /* 处理 in_frame */
            sws_scale(sws_ctx, in_frame->data, in_frame->linesize, 0, decoder_ctx->height, out_frame->data, out_frame->linesize);

            ret = avcodec_send_frame(encoder_ctx, out_frame);
            if (ret < 0)
            {
                logging("send out_frame error");
                return -1;
            }

            // 一个frame可能编码为多个packet
            while (avcodec_receive_packet(encoder_ctx, pkt) >= 0)
            {
                ret = av_interleaved_write_frame(output_ctx, pkt);
                if (ret < 0)
                {
                    logging("write file error");
                    return -1;
                }
                av_packet_unref(pkt);
            }

            // av_frame_unref(in_frame);
            // av_frame_unref(out_frame);
        }
    }
    logging("write file ok");

    // av_packet_unref(pkt);
    av_frame_unref(in_frame);
    av_frame_unref(out_frame);

    ret = av_write_trailer(output_ctx);
    if (ret < 0)
    {
        av_strerror(ret, err_buf, 25);
        logging("write file trailer error: %s", err_buf);

        return -1;
    }

    logging("write trailer ok");

    // 释放所有资源
    avformat_close_input(&input_ctx);
    avformat_free_context(input_ctx);
    avcodec_free_context(&decoder_ctx);

    avio_close(output_ctx->pb);
    avformat_free_context(output_ctx);
    avcodec_free_context(&encoder_ctx);

    av_packet_free(&pkt);
    av_frame_free(&in_frame);
    av_frame_free(&out_frame);

    return 0;
}
