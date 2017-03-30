#include <iostream>

#include <QWidget>
#include <QImage>
#include <QPainter>
#include <QMutexLocker>

#include "VideoRecorder.h"

VideoRecorder::VideoRecorder(QObject* parent, Renderer* targetWidget_) :
	curStatus(Status::Stop),
	fps(0),
	frameReady(false),
	lastFrameTime(0),
	lastFpsTime(0),
	QThread(parent),
	targetWidget(targetWidget_),
	vw(new VideoWriter(1920, 1080))
{
	VideoWriter::initAv();

	connect(targetWidget_, SIGNAL(recordFrame()), this, SLOT(recordFrame()));
	connect(this, SIGNAL(updateFrameBuffer()), targetWidget_, SLOT(updateFrameBuffer()));
}

VideoRecorder::~VideoRecorder()
{
	delete vw;
	vw = nullptr;
}

void VideoRecorder::startRecord()
{
	if (curStatus == Status::Stop)
	{
		lastFrameTime = 0;
		lastFpsTime = 0;

		frameTimer.start();
	}
	else if (curStatus == Status::Pause)
	{
		lastFrameTime += frameTimer.elapsed() - pauseTime;
		lastFpsTime += frameTimer.elapsed() - pauseTime;
	}

	curStatus = Status::Record;

	frameReady = false;
	updateFrameBuffer();
}

void VideoRecorder::pauseRecord()
{
	curStatus = Status::Pause;
	pauseTime = frameTimer.elapsed();
}

void VideoRecorder::stopRecord()
{
	curStatus = Status::Stop;
}

bool VideoRecorder::isRecording()
{
	return curStatus == Status::Record;
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
		if (curStatus == Status::Record)
		{			
			if (!vw->isOpen())
				vw->open("video");

			curTime = frameTimer.elapsed();

			if(frameReady)
			//if (curTime >= lastFrameTime + 1000 / vw->getFps())
			{		
				frameReady = false;
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
		else if (curStatus == Status::Stop && vw->isOpen())
			vw->close();
	}
}

void VideoRecorder::recordFrame()
{
	frameReady = true;
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