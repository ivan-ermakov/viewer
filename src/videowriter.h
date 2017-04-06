// Transparent images

#ifndef VIDEOWRITER_H
#define VIDEOWRITER_H

#include <string>

extern "C"
{
#include <libavformat/avformat.h>
#include "libswscale/swscale.h"
}

#include "debug/Stable.h"

#include <QImage>

class VideoWriter
{
public:
    VideoWriter(int w = 352, int h = 288, int fps_ = 25, int bitRate_ = 400000, AVCodecID codecId_ = AV_CODEC_ID_NONE, AVPixelFormat pixelFormat_ = AV_PIX_FMT_YUV420P);
	~VideoWriter();

	static void initAv(); // Init FFmpeg

	bool open(std::string);
	void close();

	bool isOpen();
	int getFps();
    int getBitRate();

    bool setBitRate(int);

	bool writeVideoFrame(std::string, int64_t msec);
    bool writeVideoFrame(const QImage&, int64_t msec);

private:
	// Video Stream
	AVStream* openVideo(AVCodecID); // add a video output stream
	void closeVideo(AVStream*);

	// Frame
	AVFrame* allocateFrame(int w, int h, AVPixelFormat);
	AVFrame* allocateFrame(AVCodecContext*);
	void freeFrame(AVFrame*);
	
	int writeVideoFrame(AVStream* st, AVFrame* frame);
    bool writeVideoFrames(AVStream* st, AVFrame* frame, int frames);
    bool writeVideoFrame(AVStream* st, AVFrame* frame, int64_t time);
	bool writeBufferedFrames(AVStream*);

    bool convertVideoFrame(AVCodecContext*, AVFrame*, AVFrame*);
	
	bool frameReadImage(AVFrame*, const QImage&);
	AVFrame* loadFrame(const QImage&);
	AVFrame* loadFrame(const std::string); // Load with ffmpeg

	void fillYuvImage(AVFrame *pict, int frame_index); // prepare a dummy image


    int m_width;
    int m_height;
    int m_fps;
    int m_bitRate;
    int m_frameCount;
    AVPixelFormat m_pixelFormat;
    AVCodecID m_codecId;

    AVFormatContext* m_formatContext;
    AVOutputFormat* m_outputFormat;
    AVStream* m_videoStream;
	
    AVFrame* m_videoFrame; // Target format video frame

    AVFrame* m_imgFrame; // Temporary conversion frame
    SwsContext* m_imgConversionContext; // Conversion context
    int m_swsFlags;
};

#endif // VIDEOWRITER_H
