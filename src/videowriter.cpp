#include <cstdio>
#include <iostream>

extern "C"
{
#include "libavutil/imgutils.h"
#include "libavutil/opt.h"
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include "libavutil/mathematics.h"
#include "libavutil/samplefmt.h"
#include "libswscale/swscale.h"
}

#include <QString>

#include "videowriter.h"

VideoWriter::VideoWriter(int w, int h, int fps_, AVCodecID codecId_, AVPixelFormat pixelFormat_, int bitRate_) :
	width(w),
	height(h),
	fps(fps_),
	pixelFormat(pixelFormat_),
	bitRate(bitRate_),
	codecId(codecId_),
	swsFlags(SWS_BICUBIC),
	formatContext(nullptr),
	outputFormat(nullptr),
	videoStream(nullptr),
	frameCount(0),
	picture(nullptr),
	tmpPicture(nullptr)
{}

VideoWriter::~VideoWriter()
{
	close();
}

void VideoWriter::initAv()
{
	//avdevice_register_all();
	avcodec_register_all();
	av_register_all();
}

bool VideoWriter::open(std::string fileName)
{
	outputFormat = av_guess_format(nullptr, fileName.c_str(), nullptr);

	if (!outputFormat)
	{
		printf("Could not deduce output format from file extension: using AVI.\n");
		outputFormat = av_guess_format("avi", nullptr, nullptr);
		fileName += ".avi";
	}

	/*if (!outputFormat)
	{
		// MPEG - black frames appended to end
		printf("Could not deduce output format from file extension: using MPEG.\n");
		outputFormat = av_guess_format("mpeg", nullptr, nullptr);
		fileName += ".mpeg";
	}*/

	if (!outputFormat)
	{
		fprintf(stderr, "Could not find suitable output format\n");
		return false;
	}

	/* allocate the output media context */
	formatContext = avformat_alloc_context();
	if (!formatContext)
	{
		fprintf(stderr, "Memory error\n");
		return false;
	}

	formatContext->oformat = outputFormat;
	snprintf(formatContext->filename, sizeof(formatContext->filename), "%s", fileName.c_str());

	/* add the audio and video streams using the default format codecs
	and initialize the codecs */

	if (codecId != AV_CODEC_ID_NONE)
		videoStream = openVideo(codecId);
	else if (outputFormat->video_codec != AV_CODEC_ID_NONE)
	{
		videoStream = openVideo(outputFormat->video_codec);
		codecId = outputFormat->video_codec;
	}

	/* set the output parameters (must be done even if no
	parameters). */
	// !!!!!
	/*if (av_set_parameters(formatContext, nullptr) < 0) {
	fprintf(stderr, "Invalid output format parameters\n");
	exit(1);
	}*/

	av_dump_format(formatContext, 0, fileName.c_str(), 1);

	/* now that all the parameters are set, we can open the audio and
	video codecs and allocate the necessary encode buffers */
	if (!videoStream)
		return false;

	/* open the output file, if needed */
	if (!(outputFormat->flags & AVFMT_NOFILE))
	{
		if (avio_open(&formatContext->pb, fileName.c_str(), AVIO_FLAG_WRITE) < 0)
		{
			std::cerr << "Could not open '" << fileName << "'\n";
			return false;
		}
	}

	/* write the stream header, if any */
	avformat_write_header(formatContext, nullptr);

	return true;
}

void VideoWriter::close()
{
	if (!formatContext)
		return;

	/* write the trailer, if any.  the trailer must be written
	* before you close the CodecContexts open when you wrote the
	* header; otherwise write_trailer may try to use memory that
	* was freed on av_codec_close() */
	av_write_trailer(formatContext);

	/* close each codec */
	if (videoStream)
	{
		closeVideo(videoStream);
		videoStream = nullptr;
	}

	/* free the streams */
	for (int i = 0; (unsigned)i < formatContext->nb_streams; i++)
	{
		av_freep(&formatContext->streams[i]->codec);
		av_freep(&formatContext->streams[i]);
	}

	if (!(outputFormat->flags & AVFMT_NOFILE))
	{
		/* close the output file */
		avio_close(formatContext->pb);
	}

	/* free the stream */
	av_free(formatContext);
	formatContext = nullptr;

	//std::cout << "Written " << frameCount << " frames\n";
}

bool VideoWriter::isOpen()
{
	return formatContext && videoStream;
}

int VideoWriter::getFps()
{
	return fps;
}

bool VideoWriter::writeVideoFrame(std::string fileName, int frames)
{
	return writeVideoFrame(QImage(QString::fromStdString(fileName)), frames);
}

bool VideoWriter::writeVideoFrame(QImage img, int frames)
{
	AVFrame* frame = loadFrame(img);
	if (!frame)
		return false;

	// write interleaved audio and video frames
	writeVideoFrames(videoStream, frame, frames);

	writeBufferedFrames(videoStream);

	freeFrame(frame);

	return true;
}

bool VideoWriter::writeVideoFrame(std::string fileName, double time)
{
	return writeVideoFrame(QImage(QString::fromStdString(fileName)), time);
}

bool VideoWriter::writeVideoFrame(QImage img, double time)
{
	AVFrame* frame = loadFrame(img);
	if (!frame)
		return false;

	writeVideoFrames(videoStream, frame, time);

	writeBufferedFrames(videoStream);

	freeFrame(frame);

	return true;
}

AVStream* VideoWriter::openVideo(AVCodecID codec_id)
{
	AVStream *st;

	st = avformat_new_stream(formatContext, 0);
	if (!st)
	{
		std::cerr << "Could not alloc stream\n";
		return nullptr;
	}

	st->codecpar->codec_id = codec_id;
	st->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;

	// put sample parameters
	st->codecpar->bit_rate = bitRate;

	// resolution must be a multiple of two
	st->codecpar->width = width;
	st->codecpar->height = height;

	/* time base: this is the fundamental unit of time (in seconds) in terms
	of which frame timestamps are represented. for fixed-fps content,
	timebase should be 1/framerate and timestamp increments should be
	identically 1. */
	st->time_base.num = 1;
	st->time_base.den = fps;
	st->codecpar->format = pixelFormat;

	/*if (st->codecpar->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
	// Needed to avoid using macroblocks in which some coeffs overflow.
	// This does not happen with normal video, it just happens here as
	// the motion of the chroma plane does not match the luma plane.
	st->codec->mb_decision = 2;
	}*/

	// some formats want stream headers to be separate
	if (formatContext->oformat->flags & AVFMT_GLOBALHEADER)
		st->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;

	AVCodec* codec;
	AVCodecContext* c;

	//c = st->codec;

	/* find the video encoder */
	codec = avcodec_find_encoder(st->codecpar->codec_id);
	if (!codec)
	{
		fprintf(stderr, "openVideo: avcodec_find_encoder: codec not found\n");
		return nullptr;
	}

	c = avcodec_alloc_context3(codec);
	if (!c)
	{
		fprintf(stderr, "Could not allocate video codec context\n");
		return nullptr;
	}

	st->codec = c;

	avcodec_parameters_to_context(c, st->codecpar);

	/*c->codec_type = st->codecpar->codec_type;
	c->codec_id = st->codecpar->codec_id;

	// put sample parameters
	c->bit_rate = st->codecpar->bit_rate;

	// resolution must be a multiple of two
	c->width = st->codecpar->width;
	c->height = st->codecpar->height;*/

	// frames per second
	c->time_base.num = st->time_base.num; // 1
	c->time_base.den = st->time_base.den; // fps

	/* emit one intra frame every ten frames
	* check frame pict_type before passing frame
	* to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
	* then gop_size is ignored and the output of encoder
	* will always be I frame irrespective to gop_size
	*/
	c->gop_size = 12;
	//c->max_b_frames = 1;
	//c->pix_fmt = (AVPixelFormat) st->codecpar->format;

	// open the codec
	if (avcodec_open2(c, codec, nullptr) < 0)
	{
		fprintf(stderr, "could not open codec\n");
		return nullptr;
	}

	// allocate the encoded raw picture
	picture = allocateFrame(c);
	if (!picture)
	{
		std::cerr << "Could not allocate picture\n";
		return nullptr;
	}

	/* if the output format is not YUV420P, then a temporary YUV420P
	picture is needed too. It is then converted to the required
	output format */

	tmpPicture = allocateFrame(c);
	if (!tmpPicture)
	{
		std::cerr << "Could not allocate temporary picture\n";
		return nullptr;
	}

	return st;
}

void VideoWriter::closeVideo(AVStream *st)
{
	avcodec_close(st->codec);

	if (picture)
	{
		freeFrame(picture);
		picture = nullptr;
	}

	if (tmpPicture)
	{
		freeFrame(tmpPicture);
		tmpPicture = nullptr;
	}

	//av_free(video_outbuf);
}

AVFrame* VideoWriter::allocateFrame(AVCodecContext* c)
{
	AVFrame* frame;

	frame = av_frame_alloc();

	if (!frame)
		return nullptr;

	frame->format = c->pix_fmt;
	frame->width = c->width;
	frame->height = c->height;

	// the image can be allocated by any means and av_image_alloc() is
	// just the most convenient way if av_malloc() is to be used

	if (av_image_alloc(frame->data, frame->linesize, c->width, c->height, c->pix_fmt, 32) < 0)
	{
		std::cerr << "Could not allocate raw picture buffer\n";
		return nullptr;
	}

	return frame;
}

void VideoWriter::freeFrame(AVFrame* f)
{
	av_free(f->data[0]);
	av_free(f);
}

int VideoWriter::writeVideoFrame(AVStream* st, AVFrame* frame)
{
	int outSize;
	int ret;

	if (frame && st->codec->pix_fmt != (AVPixelFormat)frame->format)
	{
		// as we only generate a YUV420P picture, we must convert it to the codec pixel format if needed
		if (!convertVideoFrame(st->codec, frame, tmpPicture))
			return -1; // Fail

		frame = tmpPicture;
	}

	AVPacket pkt;
	av_init_packet(&pkt);

	if (formatContext->oformat->flags & AVFMT_RAWPICTURE)
	{
		std::cout << "Raw frame\n";
		// raw video case. The API will change slightly in the near future for that

		pkt.flags |= AV_PKT_FLAG_KEY;
		pkt.stream_index = st->index;
		pkt.data = (uint8_t*) frame;
		pkt.size = sizeof(AVPicture);

		if (pkt.pts != AV_NOPTS_VALUE)
			pkt.pts = av_rescale_q(pkt.pts, st->codec->time_base, st->time_base);

		if (pkt.dts != AV_NOPTS_VALUE)
			pkt.dts = av_rescale_q(pkt.dts, st->codec->time_base, st->time_base);

		ret = av_interleaved_write_frame(formatContext, &pkt);
	}
	else
	{
		// encode the image
		int gotPacket;

		pkt.data = nullptr;
		pkt.size = 0;

		outSize = avcodec_encode_video2(st->codec, &pkt, frame, &gotPacket);

		if (outSize < 0)
		{
			std::cerr << "Error " << outSize << " encoding frame\n";
			return -1;
		}

		// if zero size, it means the image was buffered
		if (gotPacket) // outSize?
		{
			if (pkt.pts != AV_NOPTS_VALUE)
			{
				pkt.pts = av_rescale_q(pkt.pts, st->codec->time_base, st->time_base);
				//pkt.pts = av_rescale_q(pkt.pts, st->time_base, st->codec->time_base);
				//st->pts.val = pkt.pts;
			}

			if (pkt.dts != AV_NOPTS_VALUE)
			{
				pkt.dts = av_rescale_q(pkt.dts, st->codec->time_base, st->time_base);
				//pkt.dts = av_rescale_q(pkt.dts, st->time_base, st->codec->time_base);
			}

			if (st->codec->coded_frame->key_frame)
				pkt.flags |= AV_PKT_FLAG_KEY;

			pkt.stream_index = st->index;

			//pkt.data= video_outbuf;
			//pkt.size= outSize;

			// write the compressed frame in the media file
			ret = av_interleaved_write_frame(formatContext, &pkt);

			//if (ret != 0)
				//std::cerr << "av_interleaved_write_frame -> " << ret << "\n";

			//std::cout << "Written frame" << outSize << "\n";
		}
		else
		{
			ret = 1;
			//std::cout << "Delayed frame\n"; // or no more delayed frames
		}
	}

	if (ret < 0)
	{
		std::cerr << "Error " << ret << " while writing video frame\n";
		return ret;
	}

	if (ret == 0)
		frameCount++;

	return ret;
}

bool VideoWriter::writeVideoFrames(AVStream* st, AVFrame* frame, int frames)
{
	if (st->codec->pix_fmt != (AVPixelFormat)frame->format)
	{
		// as we only generate a YUV420P picture, we must convert it to the codec pixel format if needed
		if (!convertVideoFrame(st->codec, frame, tmpPicture))
			return false;

		frame = tmpPicture;
	}

	for (int i = 0; i < frames; ++i)
	{
		if (writeVideoFrame(st, frame) < 0)
		{
			return false;
		}
	}

	return true;
}

bool VideoWriter::writeVideoFrames(AVStream* st, AVFrame* frame, double time)
{
	if (st->codec->pix_fmt != (AVPixelFormat)frame->format)
	{
		// as we only generate a YUV420P picture, we must convert it to the codec pixel format if needed
		if (!convertVideoFrame(st->codec, frame, tmpPicture))
			return false;

		frame = tmpPicture;
	}

	time += (double)av_stream_get_end_pts(videoStream) * videoStream->time_base.num / videoStream->time_base.den;

	for (; (double)av_stream_get_end_pts(videoStream) * videoStream->time_base.num / videoStream->time_base.den < time;)
	{
		// write interleaved audio and video frames
		if (writeVideoFrame(videoStream, frame) < 0)
			return false;
	}

	return true;
}

bool VideoWriter::writeBufferedFrames(AVStream* st)
{
	// get the delayed frames
	while (writeVideoFrame(st, nullptr) == 0);

	return true;
}

bool VideoWriter::convertVideoFrame(AVCodecContext* c, AVFrame* frame, AVFrame* frame_to)
{
	SwsContext* convertContext = sws_getContext(frame->width, frame->height,
		(AVPixelFormat)frame->format,
		c->width, c->height,
		c->pix_fmt,
		swsFlags, nullptr, nullptr, nullptr);

	if (convertContext == nullptr)
	{
		std::cerr << "Cannot initialize the conversion context\n";
		return false;
	}

	int ret = sws_scale(convertContext, frame->data, frame->linesize, 0, frame->height, frame_to->data, frame_to->linesize);

	//if (ret != 0)
		//std::cerr << "sws_scale -> " << ret << "\n";

	sws_freeContext(convertContext);

	return ret > 0;
}

AVFrame* VideoWriter::loadFrame(QImage img)
{
	AVFrame* frame = av_frame_alloc();

	if (!frame)
		return nullptr;

	frame->format = AV_PIX_FMT_RGB24;
	frame->width = img.width();
	frame->height = img.height();

	// the image can be allocated by any means and av_image_alloc() is
	// just the most convenient way if av_malloc() is to be used

	if (av_image_alloc(frame->data, frame->linesize, frame->width, frame->height, (AVPixelFormat)frame->format, 32) < 0)
	{
		std::cerr << "Could not allocate raw picture buffer\n";
		return nullptr;
	}

	int x, y;
	QColor clr;

	for (y = 0; y < frame->height; y++)
	{
		for (x = 0; x < frame->width; x++)
		{
			clr = img.pixelColor(QPoint(x, y));
			frame->data[0][y * frame->linesize[0] + x * 3] = (uint8_t)clr.red() * clr.alpha() / 255;
			frame->data[0][y * frame->linesize[0] + x * 3 + 1] = (uint8_t)clr.green() * clr.alpha() / 255;
			frame->data[0][y * frame->linesize[0] + x * 3 + 2] = (uint8_t)clr.blue() * clr.alpha() / 255;
		}
	}

	return frame;
}

AVFrame* VideoWriter::readFrameImage(const std::string imageFileName)
{
	AVFormatContext* formatCtx = nullptr;
	AVInputFormat* inputFormat = av_find_input_format(imageFileName.c_str());

	if (!inputFormat)
	{
		std::cerr << "Cant find input format!\n";
		//return nullptr;
	}

	if (avformat_open_input(&formatCtx, imageFileName.c_str(), inputFormat, nullptr) != 0)
	{
		std::cerr << "Can't open image file '" << imageFileName << "'\n";
		return nullptr;
	}

	av_dump_format(formatCtx, 0, imageFileName.c_str(), false);

	AVCodecContext* codecCtx;

	/*codecCtx = avcodec_alloc_context3(pCodec);
	if (!codecCtx)
	{
	std::cerr << "Could not allocate video codec context\n";
	return nullptr;
	}*/

	codecCtx = formatCtx->streams[0]->codec;
	codecCtx->width = videoStream->codec->width;
	codecCtx->height = videoStream->codec->height;
	codecCtx->pix_fmt = videoStream->codec->pix_fmt; //pixelFormat;

													 // some formats want stream headers to be separate
	if (formatContext->oformat->flags & AVFMT_GLOBALHEADER)
		codecCtx->flags |= CODEC_FLAG_GLOBAL_HEADER;

	// Find the decoder for the video stream
	AVCodec* pCodec = avcodec_find_decoder(codecCtx->codec_id);
	if (!pCodec)
	{
		printf("Codec not found\n");
		return nullptr;
	}

	// Open codec
	if (avcodec_open2(codecCtx, pCodec, nullptr)<0)
	{
		printf("Could not open codec\n");
		return nullptr;
	}

	//avcodec_align_dimensions(codecCtx, &codecCtx->width, &codecCtx->height); // , frame->linesize

	// No allocation of image for frame
	AVFrame* pFrame = allocateFrame(codecCtx); // 

	if (!pFrame)
	{
		printf("Can't allocate memory for AVFrame\n");
		return nullptr;
	}

	AVPacket packet;
	av_init_packet(&packet);

	if (av_read_frame(formatCtx, &packet) < 0)
	{
		std::cerr << "Failed to read frame!\n";
		return nullptr;
	}

	int frameFinished;

	if (avcodec_decode_video2(codecCtx, pFrame, &frameFinished, &packet) < 0 || !frameFinished)
	{
		std::cerr << "Failed to decode frame!\n";
		return nullptr;
	}

	//pFrame->data[0];

	return pFrame;
}

// prepare a dummy image
void VideoWriter::fillYuvImage(AVFrame *pict, int frame_index, int width, int height)
{
	int x, y, i;

	i = frame_index;

	// Y
	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			pict->data[0][y * pict->linesize[0] + x] = x + y + i * 3;
		}
	}

	// Cb and Cr
	for (y = 0; y < height / 2; y++) {
		for (x = 0; x < width / 2; x++) {
			pict->data[1][y * pict->linesize[1] + x] = 128 + y + i * 2;
			pict->data[2][y * pict->linesize[2] + x] = 64 + x + i * 5;
		}
	}
}