//使用FFMPEG来解码音频文件
//


#include "stdafx.h"
#include <stdio.h>
#define __STD_CONSTANT_MACROS

extern "C"
{
#include "libavcodec/avcodec.h"     //包含解码器信息
#include "libavformat/avformat.h"	//包含封装信息
#include "libswresample/swresample.h"
}
#define MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio

int _tmain(int argc, _TCHAR* argv[])
{
	AVFormatContext *pFormatCtx;
	//char *filepath = "WavinFlag.aac";
	char *filepath = "skycity1.mp3";
	FILE *pFile = fopen("output.pcm", "wb");

	av_register_all();    //注册所有组件
	avformat_network_init();

	pFormatCtx = avformat_alloc_context();   //开辟内存
	if (avformat_open_input(&pFormatCtx, filepath, NULL, NULL) < 0) //打开输入视频文件
	{
		printf("Can't open the input stream.\n");
		return -1;
	}
	if (avformat_find_stream_info(pFormatCtx, NULL)<0)     //判断文件流，视频流还是音频流
	{
		printf("Can't find the stream information!\n");
		return -1;
	}

	int i, index_audio = -1;
	for (i = 0; i < pFormatCtx->nb_streams; i++)
	{
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)      //如果是音频流，则记录存储。
		{
			index_audio = i;
			break;
		}
	}
	if (index_audio == -1)
	{
		printf("Can't find a video stream;\n");
		return -1;
	}

	AVCodecContext *pCodecCtx = pFormatCtx->streams[index_audio]->codec;

	AVCodec *pCodec = avcodec_find_decoder(pCodecCtx->codec_id);     //查找解码器
	if (pCodec == NULL)
	{
		printf("Can't find a decoder!\n");
		return -1;
	}
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)   //打开编码器
	{
		printf("Can't open the decoder!\n");
		return -1;
	}
	//Audio out parameter:

	//nb_samples: AAC-1024 MP3-1152
	int out_nb_samples = pCodecCtx->frame_size;
	int64_t out_channel_layout = AV_CH_LAYOUT_STEREO;    //double channels
	AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;    //Audio sample formats
	int out_sample_rate = 44100;                          //sample rates

	int out_channels = av_get_channel_layout_nb_channels(out_channel_layout);  //channel numbers
	uint8_t *out_buffer = (uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE * 2);
	
	//特定音频格式下的缓冲区大小
	int out_buffer_size = av_samples_get_buffer_size(NULL, out_channels, out_nb_samples, out_sample_fmt, 1);

	//因为解码的数据类型是4位，播放器播放的是16位需要重采样
	struct SwrContext *aud_convert_ctx = swr_alloc_set_opts(NULL, out_channel_layout, out_sample_fmt, out_sample_rate,
		av_get_default_channel_layout(pCodecCtx->channels), pCodecCtx->sample_fmt, pCodecCtx->sample_rate, 0, NULL);
	swr_init(aud_convert_ctx);   //初始化

	AVFrame *pFrame = av_frame_alloc();  //this only allocates the avframe itself, not the data buffers
	AVPacket *pkt = (AVPacket *)av_malloc(sizeof(AVPacket));
	av_init_packet(pkt);

	int frame_cnt = 0;
	int get_frame;

	while (av_read_frame(pFormatCtx, pkt) >= 0)
	{
		if (pkt->stream_index == index_audio)
		{

			if (avcodec_decode_audio4(pCodecCtx, pFrame, &get_frame, pkt) < 0)
			{
				printf("Decode Error!\n");
				return -1;
			}
			if (get_frame)
			{
				printf("Decoded frame index: %d\n", frame_cnt);
				swr_convert(aud_convert_ctx, &out_buffer, MAX_AUDIO_FRAME_SIZE, (const uint8_t **)pFrame->data, pFrame->nb_samples);
				fwrite(out_buffer, 1, out_buffer_size, pFile);
				frame_cnt++;
			}

		}
		av_free_packet(pkt);
	}


	//free
	fclose(pFile);

	swr_free(&aud_convert_ctx);

	av_frame_free(&pFrame);
	av_free(out_buffer);

	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);
	avformat_free_context(pFormatCtx);

	return 0;
}

