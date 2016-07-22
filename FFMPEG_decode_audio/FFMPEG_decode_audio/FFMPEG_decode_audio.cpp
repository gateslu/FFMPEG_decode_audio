//ʹ��FFMPEG��������Ƶ�ļ�
//


#include "stdafx.h"
#include <stdio.h>
#define __STD_CONSTANT_MACROS

extern "C"
{
#include "libavcodec/avcodec.h"     //������������Ϣ
#include "libavformat/avformat.h"	//������װ��Ϣ
#include "libswresample/swresample.h"
}
#define MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio

int _tmain(int argc, _TCHAR* argv[])
{
	AVFormatContext *pFormatCtx;
	//char *filepath = "WavinFlag.aac";
	char *filepath = "skycity1.mp3";
	FILE *pFile = fopen("output.pcm", "wb");

	av_register_all();    //ע���������
	avformat_network_init();

	pFormatCtx = avformat_alloc_context();   //�����ڴ�
	if (avformat_open_input(&pFormatCtx, filepath, NULL, NULL) < 0) //��������Ƶ�ļ�
	{
		printf("Can't open the input stream.\n");
		return -1;
	}
	if (avformat_find_stream_info(pFormatCtx, NULL)<0)     //�ж��ļ�������Ƶ��������Ƶ��
	{
		printf("Can't find the stream information!\n");
		return -1;
	}

	int i, index_audio = -1;
	for (i = 0; i < pFormatCtx->nb_streams; i++)
	{
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)      //�������Ƶ�������¼�洢��
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

	AVCodec *pCodec = avcodec_find_decoder(pCodecCtx->codec_id);     //���ҽ�����
	if (pCodec == NULL)
	{
		printf("Can't find a decoder!\n");
		return -1;
	}
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)   //�򿪱�����
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
	
	//�ض���Ƶ��ʽ�µĻ�������С
	int out_buffer_size = av_samples_get_buffer_size(NULL, out_channels, out_nb_samples, out_sample_fmt, 1);

	//��Ϊ���������������4λ�����������ŵ���16λ��Ҫ�ز���
	struct SwrContext *aud_convert_ctx = swr_alloc_set_opts(NULL, out_channel_layout, out_sample_fmt, out_sample_rate,
		av_get_default_channel_layout(pCodecCtx->channels), pCodecCtx->sample_fmt, pCodecCtx->sample_rate, 0, NULL);
	swr_init(aud_convert_ctx);   //��ʼ��

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

