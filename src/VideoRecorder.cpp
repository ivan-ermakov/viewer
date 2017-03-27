#include <iostream>

#include <QWidget>
#include <QImage>
#include <QPainter>
#include <QMutexLocker>

#include "VideoRecorder.h"

VideoRecorder::VideoRecorder(QObject* parent, QWidget* targetWidget_) :
	record(false),
	stop(false),
	QThread(parent),
	targetWidget(targetWidget_),
	vw(new VideoWriter())
{
	VideoWriter::initAv();

	connect(targetWidget_, SIGNAL(recordFrame(QImage)), this, SLOT(recordFrame(QImage)));
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

void VideoRecorder::run()
{
	for (; vw;)
	{
		msleep(25);

		if (record)
		{
			//std::cout << "recording\n";

			//QOpenGLWidget* oglw = dynamic_cast<QOpenGLWidget*>(targetWidget);
			if (QTime::currentTime() > lastFrameTime.addMSecs(1000 / vw->getFps()))
			{		
				{
					QMutexLocker l(&mtx);
					//vw->writeVideoFrame(targetWidget->grab().toImage(), 0.025);
					vw->writeVideoFrame(img, 1. / vw->getFps());
					//img = QImage();
				}
				
				lastFrameTime = QTime::currentTime();
			}

			/*QImage img(targetWidget->size(), QImage::Format::Format_ARGB32);
			QPainter painter(&img);
			targetWidget->render(&painter);
			vw->writeVideoFrame(img, 0.025);*/
		}

		if (stop)
			vw->close();

		//std::cout << "running\n";
	}
}

void VideoRecorder::recordFrame(QImage frm)
{
	QMutexLocker l(&mtx);
	img = frm;
}