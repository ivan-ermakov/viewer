#ifndef VIDEOWRITER_H
#define VIDEOWRITER_H

#include <string>

extern "C"
{
#include <libavformat/avformat.h>
//include <libavcodec/avcodec.h>
}

#include <QImage>

class VideoWriter
{
public:
    VideoWriter(int w = 352, int h = 288, int fps_ = 25, AVCodecID codecId_ = AV_CODEC_ID_NONE, AVPixelFormat pixelFormat_ = AV_PIX_FMT_YUV420P, int bitRate_ = 400000);
	~VideoWriter();

	static void initAv();

	bool open(std::string);
	void close();

	bool isOpen();
	int getFps();

	//bool writeVideoFrame(std::string, int frames = 1);
	//bool writeVideoFrame(const QImage&, int frames = 1);
	bool writeVideoFrame(std::string, int64_t msec);
	bool writeVideoFrame(const QImage&, int64_t msec);

private:
	AVStream* openVideo(AVCodecID); // add a video output stream
	void closeVideo(AVStream*);

	AVFrame* allocateFrame(int w, int h, AVPixelFormat);
	AVFrame* allocateFrame(AVCodecContext*);
	void freeFrame(AVFrame*);
	int writeVideoFrame(AVStream* st, AVFrame* frame);
	//bool writeVideoFrames(AVStream* st, AVFrame* frame, int frames);
	bool writeVideoFrames(AVStream* st, AVFrame* frame, int64_t time);
	bool writeBufferedFrames(AVStream*);
	bool convertVideoFrame(AVCodecContext*, AVFrame*, AVFrame*);
	bool frameReadImage(AVFrame*, const QImage&);
	AVFrame* loadFrame(const QImage&);
	AVFrame* readFrameImage(const std::string); // Load with ffmpeg
	void fillYuvImage(AVFrame *pict, int frame_index, int width, int height); // prepare a dummy image

	int width;
	int height;
	int fps;
	int bitRate;
	AVPixelFormat pixelFormat;
	AVCodecID codecId;

	AVFormatContext* formatContext;
	AVOutputFormat* outputFormat;
	AVStream* videoStream;

	AVFrame* picture;
	AVFrame* tmpPicture;
	AVFrame* imgFrame;

	int frameCount;	
	int swsFlags;
};

#endif // VIDEOWRITER_H
