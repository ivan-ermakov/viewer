#pragma once

#include <QThread>
#include <QTime>
#include <QTimer>
#include <QMutex>

#include "VideoWriter.h"

class VideoRecorder : public QThread
{
	Q_OBJECT

public:
	VideoRecorder(QObject* parent, QWidget* targetWidget_);
	~VideoRecorder();

	void startRecord();
	void pauseRecord();
	void stopRecord();

	bool isRecording();

private:
	void run() override;

	bool record;
	bool stop;

	QWidget* targetWidget;
	VideoWriter* vw;

	QImage img;
	QMutex mtx;
	QTime lastFrameTime;

public slots:
	void recordFrame(QImage);
};
