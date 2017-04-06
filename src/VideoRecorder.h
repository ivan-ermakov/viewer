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

    int getFps();
    qint64 getVideoLength();
    int getBitRate();

    bool setBitRate(int);

private:
    void run() override;

    Status m_currentStatus;
    int m_fps;
    int m_lastSecondFrameCount;

    bool m_frameReady;

    Renderer* m_targetWidget;
    VideoWriter* m_videoWriter;

    qint64 m_lastFrameTime;
    qint64 m_lastFpsTime;
    qint64 m_pauseTime;
    qint64 m_videoLength;
    QElapsedTimer m_frameTimer;

signals:
	void updateFrameBuffer();

public slots:
    void recordFrame();
};
