#include <iostream>

#include "log.hpp"

extern "C"
{
#include <libavdevice/avdevice.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/time.h>
#include <libavutil/error.h>
}

// decode packets into frames
static int decode_packet(AVPacket *pPacket, AVCodecContext *pCodecContext, AVFrame *pFrame);
// save a frame into a .pgm file
static void save_gray_frame(unsigned char *buf, int wrap, int xsize, int ysize, char *filename);

int main()
{
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
    av_dict_set(&options, "framerate", "10", 0);         // 设置帧率
    av_dict_set(&options, "video_size", "1920x1080", 0); // 设置视频尺寸

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

    // record the encode and decode for the stream
    AVCodec *pCodec = NULL;
    // describe the properties of a codec of the stream i
    AVCodecParameters *pCodecParameters = NULL;
    int video_stream_index = -1;

    // loop through all streams and print its man infomation
    for (int i = 0; i < input_ctx->nb_streams; i++)
    {
        AVCodecParameters *pLocalCodecParameters = input_ctx->streams[i]->codecpar;
        logging("AVStream->time_base before open coded %d%d", input_ctx->streams[i]->time_base.num, input_ctx->streams[i]->time_base.den); // num:den->4:3, 16:9, 分子 分母
        logging("AVStream->r_frame_rate before open coded %d%d", input_ctx->streams[i]->r_frame_rate.num, input_ctx->streams[i]->r_frame_rate.den);
        logging("AVStream->start_time %" PRId64, input_ctx->streams[i]->start_time);
        logging("AVStream->duration %d" PRId64, input_ctx->streams[i]->duration);

        logging("find the proper decoder (CODEC)");
        AVCodec *pLocalCodec = avcodec_find_decoder(pLocalCodecParameters->codec_id);
        if (!pLocalCodec)
        {
            logging("ERROR: unsupported codec");
            continue; // skip
        }

        if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            if (video_stream_index == -1)
            {
                video_stream_index = i;
                pCodec = pLocalCodec;
                pCodecParameters = pLocalCodecParameters;
            }
            logging("Video Codec: resolution %d x %d", pLocalCodecParameters->width, pLocalCodecParameters->height);
        }
        else if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            logging("Audio Codec: %d channels, sample rate %d", pLocalCodecParameters->channels, pLocalCodecParameters->sample_rate);
        }

        // log its name, id and bitrate
        logging("\tCodec %s ID %d bit_rate %lld", pLocalCodec->name, pLocalCodec->id, pLocalCodecParameters->bit_rate);
    }

    if (video_stream_index == -1)
    {
        logging("ERROR: input file does not contain a video stream");
        return -1;
    }

    AVCodecContext *pCodecContex = avcodec_alloc_context3(pCodec);
    if (!pCodecContex)
    {
        logging("ERROR: failed to allocated memory for AVCodecContext");
        return -1;
    }

    // fill the codec context based on the values from the supplied codec parameters
    if (avcodec_parameters_to_context(pCodecContex, pCodecParameters) < 0)
    {
        logging("ERROR: failed to copy codec params to codec context");
        return -1;
    }

    // initialize the AVCodecContext to use the given AVCodec
    if (avcodec_open2(pCodecContex, pCodec, NULL) < 0)
    {
        logging("ERROR: failed to open codec through avcodec_open2");
        return -1;
    }

    AVFrame *pFrame = av_frame_alloc();
    if (!pFrame)
    {
        logging("ERROR: failed to allocate memory for AVFrame");
        return -1;
    }

    AVPacket *pPacket = av_packet_alloc();
    if (!pPacket)
    {
        logging("ERROR: failed to allocate memory for AVPacket");
        return -1;
    }

    ret = 0;
    int how_many_packets_to_process = 30;

    // fill the Packet with data from the Stream
    while (av_read_frame(input_ctx, pPacket) >= 0)
    {
        if (pPacket->stream_index == video_stream_index)
        {
            logging("AVPacket->pts %" PRId64, pPacket->pts);
            ret = decode_packet(pPacket, pCodecContex, pFrame);
            if (ret < 0)
                break;
            // stop it, otherwise we'll be saving hundreds of frames
            if (--how_many_packets_to_process <= 0)
                break;
        }
        av_packet_unref(pPacket);
    }

    // // 打开输出流
    // AVFormatContext *output_ctx = avformat_alloc_context();
    // AVOutputFormat *output_format = NULL;
    // avformat_alloc_output_context2(&output_ctx, NULL, "flv", "rtmp://your_streaming_server/your_stream_key");
    // if (!output_ctx)
    // {
    //     fprintf(stderr, "Could not allocate output context\n");
    //     return -1;
    // }

    // // 打开视频编码器
    // AVCodec *codec = avcodec_find_encoder_by_name("libx264");
    // if (!codec)
    // {
    //     fprintf(stderr, "Codec not found\n");
    //     return -1;
    // }

    // AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
    // // 配置编码器参数（例如：bitrate、帧率、分辨率等）

    // // 添加视频流到输出上下文
    // AVStream *video_stream = avformat_new_stream(output_ctx, codec);
    // if (!video_stream)
    // {
    //     fprintf(stderr, "Failed to create video stream\n");
    //     return -1;
    // }

    // // 打开编码器
    // if (avcodec_open2(codec_ctx, codec, NULL) < 0)
    // {
    //     fprintf(stderr, "Failed to open codec\n");
    //     return -1;
    // }

    // // 初始化输出上下文
    // if (avio_open(&output_ctx->pb, output_ctx->filename, AVIO_FLAG_WRITE) < 0)
    // {
    //     fprintf(stderr, "Failed to open output\n");
    //     return -1;
    // }

    // // 写文件头
    // if (avformat_write_header(output_ctx, NULL) < 0)
    // {
    //     fprintf(stderr, "Error writing header\n");
    //     return -1;
    // }

    // // 开始捕获和推流
    // AVPacket packet;
    // while (1)
    // {
    //     // 捕获屏幕内容并将其存储在 packet 中
    //     // 你需要实现捕获屏幕内容的逻辑

    //     // 编码并发送数据到输出流
    //     av_init_packet(&packet);
    //     if (avcodec_receive_packet(codec_ctx, &packet) == 0)
    //     {
    //         // 发送 packet 到输出上下文
    //         av_interleaved_write_frame(output_ctx, &packet);
    //         av_packet_unref(&packet);
    //     }
    // }

    // 关闭流和释放资源
    logging("releasing all the resources");
    avformat_close_input(&input_ctx);
    av_packet_free(&pPacket);
    av_frame_free(&pFrame);
    avcodec_free_context(&pCodecContex);

    // av_write_trailer(output_ctx);
    // avformat_close_input(&input_ctx);
    // avformat_free_context(output_ctx);

    return 0;
}

static int decode_packet(AVPacket *pPacket, AVCodecContext *pCodecContext, AVFrame *pFrame)
{
    // supply raw packet data as input to a decoder
    int ret = avcodec_send_packet(pCodecContext, pPacket);
    char err_buf[25];

    if (ret < 0)
    {
        av_strerror(ret, err_buf, 25);
        logging("ERROR: sending a packet to the decoder error: %s", err_buf);
        return ret;
    }

    while (ret >= 0)
    {
        // return decodec output data (into a frame) from a decoder
        ret = avcodec_receive_frame(pCodecContext, pFrame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
        {
            break;
        }
        else if (ret < 0)
        {
            av_strerror(ret, err_buf, 25);
            logging("ERROR: receiving as frame from the decoder: %s", err_buf);
            return ret;
        }

        if (ret >= 0)
        {
            logging(
                "Frame %d (type = %c, size = %d bytes, format = %d) pts %d key_frame %d [DTS %d]",
                pCodecContext->frame_number,
                av_get_picture_type_char(pFrame->pict_type),
                pFrame->pkt_size,
                pFrame->format,
                pFrame->pts,
                pFrame->key_frame,
                pFrame->coded_picture_number);

            char frame_filename[1024];
            snprintf(frame_filename, sizeof(frame_filename), "%s-%d.pgm", "frame", pCodecContext->frame_number);
            /*
               check if the frame is a planar YUV 4:2:0, 12bpp
               That is the format of the providec .mp4 file
               RGB formats will definitely not give a gray image
               Other YUV image may do so, but untested, so give a warning
            */
            if (pFrame->format != AV_PIX_FMT_YUV420P)
            {
                logging("WARNING: the generated file may not be a gray scale image, but could e.g. be just the R component if the video format is RGB");
            }
            save_gray_frame(pFrame->data[0], pFrame->linesize[0], pFrame->width, pFrame->height, frame_filename);
        }
    }

    return 0;
}

static void save_gray_frame(unsigned char *buf, int wrap, int xsize, int ysize, char *filename)
{
    FILE *f = NULL;
    int i = 0;
    f = fopen(filename, "w");
    // writing the minimal required header for a pgm file format
    // portable graymap format -> https://en.wikipedia.org/wiki/Netpbm_format#PGM_example
    fprintf(f, "P5\n%d %d\n%d\n", xsize, ysize, 255);

    // writing line by line
    for (i = 0; i < ysize; i++)
        fwrite(buf + i * wrap, 1, xsize, f);
    fclose(f);
}