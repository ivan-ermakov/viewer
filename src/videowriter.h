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
    VideoWriter(AVCodecID codecId_ = AV_CODEC_ID_NONE, AVPixelFormat pixelFormat_ = AV_PIX_FMT_YUV420P, int w = 352, int h = 288, int fps_ = 25, int bitRate_ = 400000); // AVCodecID
	~VideoWriter();

	static void initAv();

	bool open(std::string);
	void close();

	bool writeVideoFrame(std::string, int frames = 1);
	bool writeVideoFrame(QImage, int frames = 1);
	bool writeVideoFrame(std::string, double time);
	bool writeVideoFrame(QImage, double time);

private:
	AVStream* openVideo(AVCodecID); // add a video output stream
	void closeVideo(AVStream*);

	AVFrame* allocateFrame(AVCodecContext*);
	void freeFrame(AVFrame*);
	int writeVideoFrame(AVStream* st, AVFrame* frame);
	bool writeVideoFrames(AVStream* st, AVFrame* frame, int frames);
	bool writeVideoFrames(AVStream* st, AVFrame* frame, double time);
	bool writeBufferedFrames(AVStream*);
	bool convertVideoFrame(AVCodecContext*, AVFrame*, AVFrame*);
	AVFrame* loadFrame(QImage);
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

	int frameCount;	
	int swsFlags;
};

#endif // VIDEOWRITER_H
