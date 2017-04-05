#include "debug/Stable.h"

#include <QWidget>
#include <QImage>
#include <QPainter>
#include <QMutexLocker>

#include "VideoRecorder.h"
#include "MainWindow.h"

VideoRecorder::VideoRecorder(QObject* parent, Renderer* targetWidget_) :
    m_currentStatus(VIDEO_STATUS_STOP),
    m_fps(0),
    m_lastSecondFrameCount(0),
    m_frameReady(false),
    m_lastFrameTime(0),
    m_lastFpsTime(0),
    m_videoLength(0),
	QThread(parent),
    m_targetWidget(targetWidget_),
    m_videoWriter(new VideoWriter(1920, 1080, 25, 5000000))
{
	VideoWriter::initAv();

	connect(targetWidget_, SIGNAL(recordFrame()), this, SLOT(recordFrame()));
	connect(this, SIGNAL(updateFrameBuffer()), targetWidget_, SLOT(updateFrameBuffer()));
}

VideoRecorder::~VideoRecorder()
{
    if (m_videoWriter->isOpen())
        m_videoWriter->close();

    delete m_videoWriter;
    m_videoWriter = nullptr;
}

 int VideoRecorder::getFps()
 {
     return m_fps;
 }

 qint64 VideoRecorder::getVideoLength()
 {
    return m_videoLength;
 }

 int VideoRecorder::getBitRate()
 {
     return m_videoWriter->getBitRate();
 }

 bool VideoRecorder::setBitRate(int bitRate)
 {
    return m_videoWriter->setBitRate(bitRate);
 }

void VideoRecorder::startRecord()
{
    if (m_currentStatus == VIDEO_STATUS_STOP)
	{
        m_lastFrameTime = 0; // QDateTime::currentMSecsSinceEpoch();
        m_lastFpsTime = m_lastFrameTime;
        m_videoLength = 0;

        m_frameTimer.start();
	}
    else if (m_currentStatus == VIDEO_STATUS_PAUSE)
	{
        m_lastFrameTime += m_frameTimer.elapsed() - m_pauseTime;
        m_lastFpsTime += m_frameTimer.elapsed() - m_pauseTime;
	}

    m_lastSecondFrameCount = 0;
    m_currentStatus = VIDEO_STATUS_RECORD;

    m_frameReady = false;
	updateFrameBuffer();
}

void VideoRecorder::pauseRecord()
{
    m_currentStatus = VIDEO_STATUS_PAUSE;
    m_pauseTime = m_frameTimer.elapsed();
    m_fps = 0;
}

void VideoRecorder::stopRecord()
{
    m_currentStatus = VIDEO_STATUS_STOP;
    m_fps = 0;
}

bool VideoRecorder::isRecording()
{
    return m_currentStatus == VIDEO_STATUS_RECORD;
}

void VideoRecorder::terminate()
{
    m_currentStatus = VIDEO_STATUS_TERMINATE;
}

void VideoRecorder::run()
{
	qint64 curTime;
    qint64 frameLength;
    qint64 frameDelay;

    for (; m_currentStatus != VIDEO_STATUS_TERMINATE;)
    {
        switch (m_currentStatus)
        {
        case VIDEO_STATUS_RECORD:
            if (!m_videoWriter->isOpen())
            {
                m_videoWriter->open("video");
                frameDelay = 1000 / m_videoWriter->getFps();
            }

            curTime = m_frameTimer.elapsed(); // targetWidget->getLastFrameBufferUpdateTime()

            if(m_frameReady && curTime >= m_lastFrameTime + frameDelay || m_videoLength == 0)
            {
                //frameReady = false;
				updateFrameBuffer();
				{
					//QMutexLocker l(&mtx);
                    frameLength = curTime - m_lastFrameTime;
                    if (frameLength < 0)
                        frameLength = 1000 / m_videoWriter->getFps();

                    m_videoLength += frameLength;

                    QImage img = m_targetWidget->getFrameBuffer();

                    QPainter p(&img);
                    p.setPen(QPen(Qt::white));
                    p.setFont(QFont("Times", 14, QFont::Bold));
                    p.drawText(QPoint(40, 40), QDateTime::fromMSecsSinceEpoch(m_videoLength).toUTC().toString("hh:mm:ss.zzz"));
                    p.end();

                    m_videoWriter->writeVideoFrame(img, frameLength);
                    qDebug() << "VR\t" << frameLength << " ms\n";
                }
                m_lastFrameTime = curTime;

                if (curTime >= m_lastFpsTime + 1000)
				{
                    m_fps = m_lastSecondFrameCount;
                    m_lastSecondFrameCount = 0;
                    m_lastFpsTime = curTime;
				}
                else
                    ++m_lastSecondFrameCount;
            }
            break;

        case VIDEO_STATUS_STOP:
            if (m_videoWriter->isOpen())
            {
                m_videoWriter->close();
            }
        }
	}
}

void VideoRecorder::recordFrame()
{
    m_frameReady = true;
}
