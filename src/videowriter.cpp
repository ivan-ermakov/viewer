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

#include "debug/Stable.h"

#include <QDebug>
#include <QtDebug>
#include <QString>

#include "videowriter.h"

VideoWriter::VideoWriter(int w, int h, int m_fps_, int bitRate_, AVCodecID codecId_, AVPixelFormat pixelFormat_) :
    m_width(w),
    m_height(h),
    m_fps(m_fps_),
    m_pixelFormat(pixelFormat_),
    m_bitRate(bitRate_),
    m_codecId(codecId_),
    m_swsFlags(SWS_BICUBIC),
    m_formatContext(nullptr),
    m_outputFormat(nullptr),
    m_videoStream(nullptr),
    m_frameCount(0),
    m_videoFrame(nullptr),
    m_imgFrame(nullptr),
    m_imgConversionContext(nullptr)
{}

VideoWriter::~VideoWriter()
{
	close();
}

void VideoWriter::initAv()
{
	avcodec_register_all();
	av_register_all();
}

bool VideoWriter::open(std::string fileName)
{
    m_outputFormat = av_guess_format(nullptr, fileName.c_str(), nullptr);

    if (!m_outputFormat)
	{
        qDebug("Could not deduce output format from file extension: using AVI.\n");
        m_outputFormat = av_guess_format("avi", nullptr, nullptr);
		fileName += ".avi";
    }

    if (!m_outputFormat)
	{
        qDebug("Could not find suitable output format\n");
		return false;
	}

    // allocate the output media context
    m_formatContext = avformat_alloc_context();
    if (!m_formatContext)
	{
        qDebug("Memory error\n");
		return false;
	}

    m_formatContext->oformat = m_outputFormat;
    _snprintf(m_formatContext->filename, sizeof(m_formatContext->filename), "%s", fileName.c_str());

    // add the audio and video streams using the default format codecs and initialize the codecs

    if (m_codecId != AV_CODEC_ID_NONE)
        m_videoStream = openVideo(m_codecId);
    else if (m_outputFormat->video_codec != AV_CODEC_ID_NONE)
	{
        m_videoStream = openVideo(m_outputFormat->video_codec);
        m_codecId = m_outputFormat->video_codec;
	}

    // set the output parameters (must be done even if no parameters).

    av_dump_format(m_formatContext, 0, fileName.c_str(), 1);

    // now that all the parameters are set, we can open the audio and
    // video codecs and allocate the necessary encode buffers
    if (!m_videoStream)
		return false;

    // open the output file, if needed
    if (!(m_outputFormat->flags & AVFMT_NOFILE))
	{
        if (avio_open(&m_formatContext->pb, fileName.c_str(), AVIO_FLAG_WRITE) < 0)
		{
            qDebug() << "Could not open '" << fileName.c_str() << "'\n";
			return false;
		}
	}

    // write the stream header, if any
    avformat_write_header(m_formatContext, nullptr);

	return true;
}

void VideoWriter::close()
{
    if (!m_formatContext)
		return;

	/* write the trailer, if any.  the trailer must be written
	* before you close the CodecContexts open when you wrote the
	* header; otherwise write_trailer may try to use memory that
	* was freed on av_codec_close() */
    av_write_trailer(m_formatContext);

    // close each codec
    if (m_videoStream)
	{
        closeVideo(m_videoStream);
        m_videoStream = nullptr;
	}

    // free the streams
    for (int i = 0; (unsigned)i < m_formatContext->nb_streams; i++)
	{
        av_freep(&m_formatContext->streams[i]->codec);
        av_freep(&m_formatContext->streams[i]);
	}

    if (!(m_outputFormat->flags & AVFMT_NOFILE))
	{
        // close the output file
        avio_close(m_formatContext->pb);
	}

    m_outputFormat = nullptr;

    // free the stream
    av_free(m_formatContext);
    m_formatContext = nullptr;
}

bool VideoWriter::isOpen()
{
    return m_formatContext && m_outputFormat && m_videoStream;
}

int VideoWriter::getFps()
{
    return m_fps;
}

int VideoWriter::getBitRate()
{
    return m_bitRate;
}

bool VideoWriter::setBitRate(int bitRate_)
{
    if (isOpen() || bitRate_ <= 0)
        return false;

    m_bitRate = bitRate_;
    return true;
}

bool VideoWriter::writeVideoFrame(std::string fileName, int64_t time)
{
	return writeVideoFrame(QImage(QString::fromStdString(fileName)), time);
}

bool VideoWriter::writeVideoFrame(const QImage& img, int64_t time)
{
    qint64 start = 1000 * av_stream_get_end_pts(m_videoStream) * m_videoStream->time_base.num / m_videoStream->time_base.den;
    frameReadImage(m_videoFrame, img);
    writeVideoFrame(m_videoStream, m_videoFrame, time);
    writeBufferedFrames(m_videoStream);
    qDebug() << "VW pts\t" << 1000 * av_stream_get_end_pts(m_videoStream) * m_videoStream->time_base.num / m_videoStream->time_base.den - start << " ms";

	return true;
}

AVStream* VideoWriter::openVideo(AVCodecID codec_id)
{
	AVStream *st;

    st = avformat_new_stream(m_formatContext, 0);
	if (!st)
	{
        qDebug() << "Could not alloc stream\n";
		return nullptr;
	}

    // TODO: avcodec_get_context_defaults3(st->codec, codec);
	st->codecpar->codec_id = codec_id;
	st->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;

	// put sample parameters
    st->codecpar->bit_rate = m_bitRate;

	// resolution must be a multiple of two
    st->codecpar->width = m_width;
    st->codecpar->height = m_height;

	/* time base: this is the fundamental unit of time (in seconds) in terms
    of which frame timestamps are represented. for fixed-m_fps content,
	timebase should be 1/framerate and timestamp increments should be
	identically 1. */
	st->time_base.num = 1;
    st->time_base.den = m_fps;
    st->codecpar->format = m_pixelFormat;

	// some formats want stream headers to be separate
    if (m_formatContext->oformat->flags & AVFMT_GLOBALHEADER)
		st->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;

	AVCodec* codec;
    AVCodecContext* c;

    // find the video encoder
	codec = avcodec_find_encoder(st->codecpar->codec_id);
	if (!codec)
	{
        qDebug("openVideo: avcodec_find_encoder: codec not found\n");
		return nullptr;
	}

	c = avcodec_alloc_context3(codec);
	if (!c)
	{
        qDebug("Could not allocate video codec context\n");
		return nullptr;
	}

	st->codec = c;

	avcodec_parameters_to_context(c, st->codecpar);

	// frames per second
	c->time_base.num = st->time_base.num; // 1
    c->time_base.den = st->time_base.den; // m_fps

	/* emit one intra frame every ten frames
	* check frame pict_type before passing frame
	* to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
	* then gop_size is ignored and the output of encoder
	* will always be I frame irrespective to gop_size
	*/
    c->gop_size = 12;

	// open the codec
	if (avcodec_open2(c, codec, nullptr) < 0)
	{
        qDebug("could not open codec\n");
		return nullptr;
	}

	// allocate the encoded raw picture
    m_videoFrame = allocateFrame(c);
    if (!m_videoFrame)
	{
        qDebug() << "Could not allocate picture\n";
		return nullptr;
	}

	/* if the output format is not YUV420P, then a temporary YUV420P
	picture is needed too. It is then converted to the required
	output format */

    m_imgFrame = allocateFrame(c->width, c->height, AV_PIX_FMT_RGB32);
    if (!m_imgFrame)
	{
        qDebug() << "Could not allocate temporary picture\n";
		return nullptr;
	}

    m_imgConversionContext = sws_getContext(m_imgFrame->width, m_imgFrame->height,
        (AVPixelFormat)m_imgFrame->format,
		c->width, c->height,
		c->pix_fmt,
        m_swsFlags, nullptr, nullptr, nullptr);

	return st;
}

void VideoWriter::closeVideo(AVStream *st)
{
	avcodec_close(st->codec);

    if (m_videoFrame)
	{
        freeFrame(m_videoFrame);
        m_videoFrame = nullptr;
	}

    if (m_imgFrame)
	{
        freeFrame(m_imgFrame);
        m_imgFrame = nullptr;
	}

    if (m_imgConversionContext)
	{
        sws_freeContext(m_imgConversionContext);
        m_imgConversionContext = nullptr;
    }
}

AVFrame* VideoWriter::allocateFrame(int w, int h, AVPixelFormat pixFmt)
{
	AVFrame* frame = av_frame_alloc();

	if (!frame)
		return nullptr;

	frame->format = pixFmt;
	frame->width = w;
	frame->height = h;

	// the image can be allocated by any means and av_image_alloc() is
	// just the most convenient way if av_malloc() is to be used

    int ret = av_image_alloc(frame->data, frame->linesize, w, h, pixFmt, 32);
    if (ret < 0)
	{
        qDebug() << "av_image_alloc " << ret << ": Could not allocate raw picture buffer\n";
		return nullptr;
	}

	return frame;
}

AVFrame* VideoWriter::allocateFrame(AVCodecContext* c)
{
	return allocateFrame(c->width, c->height, c->pix_fmt);
}

void VideoWriter::freeFrame(AVFrame* f)
{
	if (!f)
		return;

	av_free(f->data[0]);
	av_free(f);
}

int VideoWriter::writeVideoFrame(AVStream* st, AVFrame* frame)
{
	int outSize;
	int ret;

	if (!frame || st->codec->pix_fmt != (AVPixelFormat)frame->format)
		return -1; // Fail

	AVPacket pkt;
	av_init_packet(&pkt);

    if (m_formatContext->oformat->flags & AVFMT_RAWPICTURE)
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

        ret = av_interleaved_write_frame(m_formatContext, &pkt);
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
        if (gotPacket)
		{
            if (pkt.pts != AV_NOPTS_VALUE)
                pkt.pts = av_rescale_q(pkt.pts, st->codec->time_base, st->time_base);

            if (pkt.dts != AV_NOPTS_VALUE)
                pkt.dts = av_rescale_q(pkt.dts, st->codec->time_base, st->time_base);

			if (st->codec->coded_frame->key_frame)
				pkt.flags |= AV_PKT_FLAG_KEY;

            pkt.stream_index = st->index;

			// write the compressed frame in the media file
            ret = av_interleaved_write_frame(m_formatContext, &pkt);
		}
        else
            ret = 1;
	}

	if (ret < 0)
	{
        qDebug() << "Error " << ret << " while writing video frame\n";
		return ret;
	}

	if (ret == 0)
        m_frameCount++;

	return ret;
}

bool VideoWriter::writeVideoFrames(AVStream* st, AVFrame* frame, int frames)
{
	for (int i = 0; i < frames; ++i)
	{
		if (writeVideoFrame(st, frame) < 0)
			return false;
	}

	return true;
}

bool VideoWriter::writeVideoFrame(AVStream* st, AVFrame* frame, int64_t time)
{
    return writeVideoFrames(st, frame, time * m_fps / 1000);
}

bool VideoWriter::writeBufferedFrames(AVStream* st)
{
	// get the delayed frames
	while (writeVideoFrame(st, nullptr) == 0);

	return true;
}

bool VideoWriter::convertVideoFrame(AVCodecContext* c, AVFrame* srcFrame, AVFrame* dstFrame)
{
	SwsContext* conversionContext = sws_getContext(srcFrame->width, srcFrame->height,
		(AVPixelFormat)srcFrame->format,
		c->width, c->height,
		c->pix_fmt,
        m_swsFlags, nullptr, nullptr, nullptr);

	if (conversionContext == nullptr)
	{
        qDebug() << "Cannot initialize the conversion context\n";
		return false;
	}

	int ret = sws_scale(conversionContext, srcFrame->data, srcFrame->linesize, 0, srcFrame->height, dstFrame->data, dstFrame->linesize);

	sws_freeContext(conversionContext);

	return ret > 0;
}

bool VideoWriter::frameReadImage(AVFrame* frame, const QImage& img)
{
    if (!m_imgFrame || m_imgFrame->width != img.width() || m_imgFrame->height != img.height() || !m_imgConversionContext)
	{
        freeFrame(m_imgFrame);
        sws_freeContext(m_imgConversionContext);

        m_imgFrame = allocateFrame(img.width(), img.height(), AV_PIX_FMT_RGB32);

        if (!m_imgFrame)
		{
            qDebug() << "frameReadImage: Failed to allocate frame\n";
            m_imgConversionContext = nullptr;
			return false;
		}

        m_imgConversionContext = sws_getContext(m_imgFrame->width, m_imgFrame->height, (AVPixelFormat)m_imgFrame->format,
			frame->width, frame->height, (AVPixelFormat)frame->format,
            m_swsFlags, nullptr, nullptr, nullptr);

        if (!m_imgConversionContext)
			return false;
	}

    //if (!img.hasAlphaChannel())
        memcpy(m_imgFrame->data[0], img.constBits(), img.width() * img.height() * 4);
    //else
    /*{
        QImage im = img.scaled(QSize(frame->width, frame->height));
        int x, y;
        QColor clr;

        for (y = 0; y < im.height(); y++)
        {
            for (x = 0; x < im.width(); x++)
            {
                clr = im.pixelColor(QPoint(x, y));
                frame->data[0][y * frame->linesize[0] + x * 3] = (uint8_t)clr.red() * clr.alpha() / 255;
                frame->data[0][y * frame->linesize[0] + x * 3 + 1] = (uint8_t)clr.green() * clr.alpha() / 255;
                frame->data[0][y * frame->linesize[0] + x * 3 + 2] = (uint8_t)clr.blue() * clr.alpha() / 255;
            }
        }
    }*/

    if (sws_scale(m_imgConversionContext, m_imgFrame->data, m_imgFrame->linesize, 0, m_imgFrame->height, frame->data, frame->linesize) <= 0)
		return false;

	return true;
}

AVFrame* VideoWriter::loadFrame(const QImage& img)
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
        qDebug() << "Could not allocate raw picture buffer\n";
		return nullptr;
	}

	int x, y;
	QColor clr;

    for (y = 0; y < img.height(); y++)
	{
		for (x = 0; x < img.width(); x++)
		{
			clr = img.pixelColor(QPoint(x, y));
			frame->data[0][y * frame->linesize[0] + x * 3] = (uint8_t)clr.red() * clr.alpha() / 255;
			frame->data[0][y * frame->linesize[0] + x * 3 + 1] = (uint8_t)clr.green() * clr.alpha() / 255;
			frame->data[0][y * frame->linesize[0] + x * 3 + 2] = (uint8_t)clr.blue() * clr.alpha() / 255;
		}
	}

	return frame;
}

AVFrame* VideoWriter::loadFrame(const std::string imageFileName)
{
    AVFormatContext* formatCtx = nullptr;
	AVInputFormat* inputFormat = av_find_input_format(imageFileName.c_str());

	if (!inputFormat)
        qDebug() << "Cant find input format!\n";

	if (avformat_open_input(&formatCtx, imageFileName.c_str(), inputFormat, nullptr) != 0)
	{
        qDebug() << "Can't open image file '" << imageFileName.c_str() << "'\n";
		return nullptr;
	}

	av_dump_format(formatCtx, 0, imageFileName.c_str(), false);

	AVCodecContext* codecCtx;

	codecCtx = formatCtx->streams[0]->codec;
    codecCtx->width = m_videoStream->codec->width;
    codecCtx->height = m_videoStream->codec->height;
    codecCtx->pix_fmt = m_videoStream->codec->pix_fmt; // m_pixelFormat

    // some formats want stream headers to be separate
    if (m_formatContext->oformat->flags & AVFMT_GLOBALHEADER)
		codecCtx->flags |= CODEC_FLAG_GLOBAL_HEADER;

	// Find the decoder for the video stream
	AVCodec* pCodec = avcodec_find_decoder(codecCtx->codec_id);
	if (!pCodec)
	{
        qDebug("Codec not found\n");
		return nullptr;
	}

	// Open codec
	if (avcodec_open2(codecCtx, pCodec, nullptr)<0)
	{
        qDebug("Could not open codec\n");
		return nullptr;
    }

	// No allocation of image for frame
    AVFrame* pFrame = allocateFrame(codecCtx);

	if (!pFrame)
	{
        qDebug("Can't allocate memory for AVFrame\n");
		return nullptr;
	}

	AVPacket packet;
	av_init_packet(&packet);

	if (av_read_frame(formatCtx, &packet) < 0)
	{
        qDebug() << "Failed to read frame!\n";
		return nullptr;
	}

	int frameFinished;

	if (avcodec_decode_video2(codecCtx, pFrame, &frameFinished, &packet) < 0 || !frameFinished)
	{
        qDebug() << "Failed to decode frame!\n";
		return nullptr;
    }

	return pFrame;
}

// prepare a dummy image
void VideoWriter::fillYuvImage(AVFrame *pict, int frame_index)
{
	int x, y, i;

	i = frame_index;

	// Y
	for (y = 0; y < pict->height; y++) {
		for (x = 0; x < pict->width; x++) {
			pict->data[0][y * pict->linesize[0] + x] = x + y + i * 3;
		}
	}

	// Cb and Cr
	for (y = 0; y < pict->height / 2; y++) {
		for (x = 0; x < pict->width / 2; x++) {
			pict->data[1][y * pict->linesize[1] + x] = 128 + y + i * 2;
			pict->data[2][y * pict->linesize[2] + x] = 64 + x + i * 5;
		}
	}
}
