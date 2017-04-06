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
    QBasicTimer m_timer;
    QOpenGLShaderProgram m_ShaderProgram;
    Model* m_model;

    QMatrix4x4 m_modelView;
    QMatrix4x4 m_projection;

    QVector2D m_mousePressPosition;
    QVector3D m_rotationAxis;
    qreal m_angularSpeed;
    QQuaternion m_rotation;
    QVector2D m_mouseLastPosition;

    QVector3D m_translation;
    qreal m_translationSpeed;
    qreal m_scale;

    QColor m_modelColor;
    QColor m_lightColor;

    QImage m_frameBuffer;
    qint64 m_lastFrameBufferUpdateTime;
    QOpenGLBuffer* m_pixBufObj;
    bool m_frameBufferRead;
    bool m_frameBufUpdate;

signals:
    void recordFrame();

public slots:
	void updateFrameBuffer();
};

#endif // RENDERER_H
