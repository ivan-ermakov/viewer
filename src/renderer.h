#ifndef RENDERER_H
#define RENDERER_H

#include "debug/Stable.h"

#include <QString>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QMatrix4x4>
#include <QQuaternion>
#include <QVector2D>
#include <QBasicTimer>
#include <QOpenGLShaderProgram>
#include <QColor>

#include "model.h"

class Renderer : public QOpenGLWidget, public QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit Renderer(QWidget *parent = 0);
    ~Renderer();

    
    void setLightColor(QColor);
    void setModelColor(QColor);

	int getWidth();
	int getHeight();
    QColor getLightColor();
    QColor getModelColor();

	QImage& getFrameBuffer();
    qint64 getLastFrameBufferUpdateTime();

	void loadModel(QString);

protected:
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void wheelEvent(QWheelEvent*) override;

    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private:
    QBasicTimer timer;
    QOpenGLShaderProgram program;
    Model* mdl;

    QMatrix4x4 modelView;
    QMatrix4x4 projection;

    QVector2D mousePressPosition;
    QVector3D rotationAxis;
    qreal angularSpeed;
    QQuaternion rotation;
    QVector2D mouseLastPosition;

    QVector3D translation;
    qreal translationSpeed;
    qreal scale;

    QColor modelColor;
    QColor lightColor;

	QImage frameBuffer;
    qint64 lastFrameBufferUpdateTime;

signals:
	void recordFrame();
	void recordFrame(QImage&);

public slots:
	void updateFrameBuffer();
};

#endif // RENDERER_H
