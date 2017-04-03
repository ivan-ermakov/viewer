/*
QUALITY SELECTION
*/

#pragma once

#include "debug/Stable.h"

#include <QThread>
#include <QTime>
#include <QElapsedTimer>
#include <QMutex>
#include <QOpenGLWidget>

#include "VideoWriter.h"
#include "Renderer.h"

enum Status
{
    VIDEO_STATUS_STOP,
    VIDEO_STATUS_PAUSE,
    VIDEO_STATUS_RECORD,
    VIDEO_STATUS_TERMINATE // Terminate thread
};

class VideoRecorder : public QThread
{
	Q_OBJECT

public:
	VideoRecorder(QObject* parent, Renderer* targetWidget_);
	~VideoRecorder();

	void startRecord();
	void pauseRecord();
	void stopRecord();
    void terminate();

	bool isRecording();
	bool needNextFrame();

    int getFps();
    qint64 getVideoLength();
    int getBitRate();

    bool setBitRate(int);

private:
    void run() override;

	Status curStatus;
	int fps;
    int lastSecondFrameCount;

	bool frameReady;

	Renderer* targetWidget;
	VideoWriter* vw;

	QImage img;
	QMutex mtx;
	qint64 lastFrameTime;
	qint64 lastFpsTime;
	qint64 pauseTime;
    qint64 videoLength;
	QElapsedTimer frameTimer;

signals:
	void updateFrameBuffer();

public slots:
	void recordFrame();
	//void recordFrame(QImage&);
};
