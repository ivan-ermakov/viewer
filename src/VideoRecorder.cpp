#include "debug/Stable.h"

#include <QWidget>
#include <QImage>
#include <QPainter>
#include <QMutexLocker>

#include "VideoRecorder.h"
#include "MainWindow.h"

VideoRecorder::VideoRecorder(QObject* parent, Renderer* targetWidget_) :
    curStatus(VIDEO_STATUS_STOP),
	fps(0),
    lastSecondFrameCount(0),
	frameReady(false),
	lastFrameTime(0),
	lastFpsTime(0),
    videoLength(0),
	QThread(parent),
	targetWidget(targetWidget_),
    vw(new VideoWriter(1920, 1080, 25, 5000000))
{
	VideoWriter::initAv();

	connect(targetWidget_, SIGNAL(recordFrame()), this, SLOT(recordFrame()));
	connect(this, SIGNAL(updateFrameBuffer()), targetWidget_, SLOT(updateFrameBuffer()));
}

VideoRecorder::~VideoRecorder()
{
    if (vw->isOpen())
        vw->close();

	delete vw;
	vw = nullptr;
}

 int VideoRecorder::getFps()
 {
     return fps;
 }

 qint64 VideoRecorder::getVideoLength()
 {
    return videoLength;
 }

 int VideoRecorder::getBitRate()
 {
     return vw->getBitRate();
 }

 bool VideoRecorder::setBitRate(int bitRate)
 {
    return vw->setBitRate(bitRate);
 }

void VideoRecorder::startRecord()
{
    if (curStatus == VIDEO_STATUS_STOP)
	{
        lastFrameTime = 0;//QDateTime::currentMSecsSinceEpoch();
        lastFpsTime = lastFrameTime;
        videoLength = 0;

		frameTimer.start();
	}
    else if (curStatus == VIDEO_STATUS_PAUSE)
	{
		lastFrameTime += frameTimer.elapsed() - pauseTime;
		lastFpsTime += frameTimer.elapsed() - pauseTime;
	}

    lastSecondFrameCount = 0;
    curStatus = VIDEO_STATUS_RECORD;

	frameReady = false;
	updateFrameBuffer();
}

void VideoRecorder::pauseRecord()
{
    curStatus = VIDEO_STATUS_PAUSE;
	pauseTime = frameTimer.elapsed();
    fps = 0;
}

void VideoRecorder::stopRecord()
{
    curStatus = VIDEO_STATUS_STOP;
    fps = 0;
}

bool VideoRecorder::isRecording()
{
    return curStatus == VIDEO_STATUS_RECORD;
}

void VideoRecorder::terminate()
{
    curStatus = VIDEO_STATUS_TERMINATE;
}

bool VideoRecorder::needNextFrame()
{
	return true; //  curTime >= lastFrameTime.addMSecs(1000 / vw->getFps());
}

void VideoRecorder::run()
{
	qint64 curTime;
    qint64 frameLength;
    qint64 frameDelay;

    for (; curStatus != VIDEO_STATUS_TERMINATE;)
	{
        switch (curStatus)
        {
        case VIDEO_STATUS_RECORD:
			if (!vw->isOpen())
            {
                vw->open("video");
                frameDelay = 1000 / vw->getFps();
            }

            curTime = frameTimer.elapsed();//targetWidget->getLastFrameBufferUpdateTime();//

            if(frameReady && curTime >= lastFrameTime + frameDelay || videoLength == 0)
            {
                //frameReady = false;
				updateFrameBuffer();
				{
					//QMutexLocker l(&mtx);
                    frameLength = curTime - lastFrameTime;
                    if (frameLength < 0)
                        frameLength = 1000 / vw->getFps();

                    videoLength += frameLength;
                    QImage img = targetWidget->getFrameBuffer();
                    QPainter p(&img);
                    p.setPen(QPen(Qt::white));
                    p.setFont(QFont("Times", 14, QFont::Bold));
                    p.drawText(QPoint(40, 40), QDateTime::fromMSecsSinceEpoch(videoLength).toUTC().toString("hh:mm:ss.zzz"));
                    vw->writeVideoFrame(img, frameLength);
                    qDebug() << "VR\t" << frameLength << " ms\n";
                }
                lastFrameTime = curTime;

				if (curTime >= lastFpsTime + 1000)
				{
                    fps = lastSecondFrameCount;
                    lastSecondFrameCount = 0;
					lastFpsTime = curTime;
				}
                else
                    ++lastSecondFrameCount;
			}

			/*QImage img(targetWidget->size(), QImage::Format::Format_ARGB32);
			QPainter painter(&img);
			targetWidget->render(&painter);
			vw->writeVideoFrame(img, 0.025);*/
            break;

        case VIDEO_STATUS_STOP:
            if (vw->isOpen())
                vw->close();
        }
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
