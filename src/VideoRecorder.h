/*
 * KILL THREAD
FPS
TIMER
PAUSING
SLOWMO
QUALITY SELECTION
*/

#pragma once

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
    VIDEO_STATUS_RECORD
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

	bool isRecording();
	bool needNextFrame();

    int getFps();

private:
    void run() override;

	Status curStatus;
	int fps;

	bool frameReady;

	Renderer* targetWidget;
	VideoWriter* vw;

	QImage img;
	QMutex mtx;
	qint64 lastFrameTime;
	qint64 lastFpsTime;
	qint64 pauseTime;
	QElapsedTimer frameTimer;

signals:
	void updateFrameBuffer();

public slots:
	void recordFrame();
	//void recordFrame(QImage&);
};
