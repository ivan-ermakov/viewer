#include <iostream>

#include <QWidget>
#include <QImage>
#include <QPainter>
#include <QMutexLocker>

#include "VideoRecorder.h"

VideoRecorder::VideoRecorder(QObject* parent, Renderer* targetWidget_) :
	record(false),
	stop(false),
	fps(0),
	lastFrameTime(0),
	lastFpsTime(0),
	QThread(parent),
	targetWidget(targetWidget_),
	vw(new VideoWriter(1920, 1080))
{
	VideoWriter::initAv();

	connect(targetWidget_, SIGNAL(recordFrame()), this, SLOT(recordFrame()));
	connect(this, SIGNAL(updateFrameBuffer()), targetWidget_, SLOT(updateFrameBuffer()));

	frameTimer.start();
}

VideoRecorder::~VideoRecorder()
{
	delete vw;
	vw = nullptr;
}

void VideoRecorder::startRecord()
{
	if (!vw->isOpen())
		vw->open("video");

	stop = false;
	record = true;
}

void VideoRecorder::pauseRecord()
{
	record = false;
}

void VideoRecorder::stopRecord()
{
	stop = true;
	record = false;
}

bool VideoRecorder::isRecording()
{
	return record && !stop;
}

bool VideoRecorder::needNextFrame()
{
	return true; //  curTime >= lastFrameTime.addMSecs(1000 / vw->getFps());
}

void VideoRecorder::run()
{
	qint64 curTime;

	for (; vw;)
	{
		if (record)
		{			
			curTime = frameTimer.elapsed();
			if (curTime >= lastFrameTime + 1000 / vw->getFps())
			{		
				updateFrameBuffer();
				{
					//QMutexLocker l(&mtx);
					vw->writeVideoFrame(targetWidget->getFrameBuffer(), curTime - lastFrameTime); //  1000 / vw->getFps()
					//vw->writeVideoFrame(img, 1000 / vw->getFps());
				}				
				
				lastFrameTime = curTime;

				if (curTime >= lastFpsTime + 1000)
				{
					std::cout << "FPS:\t" << fps << "\n";
					fps = 0;
					lastFpsTime = curTime;
				}
				else
					++fps;
			}

			/*QImage img(targetWidget->size(), QImage::Format::Format_ARGB32);
			QPainter painter(&img);
			targetWidget->render(&painter);
			vw->writeVideoFrame(img, 0.025);*/
		}

		if (stop)
			vw->close();
	}
}

void VideoRecorder::recordFrame()
{
	/*QMutexLocker l(&mtx);
	img = frm;*/

	/*{
		//QMutexLocker l(&mtx);
		vw->writeVideoFrame(targetWidget->getFrameBuffer(), 1. / vw->getFps());
	}

	qint64 curTime = frameTimer.elapsed();
	lastFrameTime = curTime;

	if (curTime >= lastFpsTime + 1000)
	{
		std::cout << "FPS:\t" << fps << "\n";
		fps = 0;
		lastFpsTime = curTime;
	}
	else
		++fps;*/
}