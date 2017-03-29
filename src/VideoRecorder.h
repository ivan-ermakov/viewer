#pragma once

#include <QThread>
#include <QTime>
#include <QElapsedTimer>
#include <QMutex>
#include <QOpenGLWidget>

#include "VideoWriter.h"
#include "Renderer.h"

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

private:
	void run() override;

	bool record;
	bool stop;
	int fps;

	Renderer* targetWidget;
	VideoWriter* vw;

	QImage img;
	QMutex mtx;
	qint64 lastFrameTime;
	qint64 lastFpsTime;
	QElapsedTimer frameTimer;

signals:
	void updateFrameBuffer();

public slots:
	void recordFrame();
};
