#include <cmath>

#include "debug/Stable.h"

#include <QMouseEvent>
#include <QFileDialog>
#include <QColorDialog>

#include "Renderer.h"
#include "ModelLoadDialog.h"
#include "VideoRecorder.h"

Renderer::Renderer(QWidget *parent) :
    QOpenGLWidget(parent),
    m_model(nullptr),
    m_angularSpeed(0),
    m_translationSpeed(0.005),
    m_scale(1),
    m_translation(0, 0, -5),
    m_lightColor(Qt::white),
    m_modelColor(Qt::white),
    m_lastFrameBufferUpdateTime(QDateTime::currentMSecsSinceEpoch()),
    m_pixBufObj(new QOpenGLBuffer(QOpenGLBuffer::PixelPackBuffer)),
    m_frameBufferRead(false),
    m_frameBufUpdate(false)
{}

Renderer::~Renderer()
{
    // Make sure the context is current when deleting the texture
    // and the buffers.

    makeCurrent();

    delete m_model;
    delete m_pixBufObj;

    doneCurrent();
}

QImage& Renderer::getFrameBuffer()
{
    return m_frameBuffer;
}

qint64 Renderer::getLastFrameBufferUpdateTime()
{
    return m_lastFrameBufferUpdateTime;
}

void Renderer::updateFrameBuffer()
{
    // glFenceSync - maybe useful for sync

    m_frameBufUpdate = true;

    m_pixBufObj->bind();

    if (!m_frameBufferRead)
    {
        m_frameBufferRead = true;
        glReadPixels(0, 0, geometry().width(), geometry().height(), GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        m_pixBufObj->release();
        QTimer::singleShot(0, Qt::PreciseTimer, this, &Renderer::updateFrameBuffer);
        return;
    }

    unsigned char* fb = (unsigned char*) m_pixBufObj->map(QOpenGLBuffer::ReadOnly);
    if (!fb)
    {
        qDebug() << "fb: fail\n";
        m_pixBufObj->release();
        QTimer::singleShot(0, Qt::PreciseTimer, this, &Renderer::updateFrameBuffer);
        return;
    }

    m_frameBuffer = QImage(fb, geometry().width(), geometry().height(), QImage::Format_RGB32).mirrored();

    if (!m_pixBufObj->unmap())
    {
        qDebug() << "FrameBuffer: failed unmap\n";
        return;
    }

    if (m_frameBuffer.isNull())
    {
        qDebug() << "fb: null frame\n";
        QTimer::singleShot(0, Qt::PreciseTimer, this, &Renderer::updateFrameBuffer);
        return;
    }

    m_pixBufObj->release();
    m_frameBufferRead = false;
    m_lastFrameBufferUpdateTime = QDateTime::currentMSecsSinceEpoch();
	recordFrame();
}

void Renderer::loadModel(QString fileName)
{
    ModelLoadDialog* mld = new ModelLoadDialog(this, fileName);
    mld->exec();

    if (mld->isReady())
    {
        if (m_model)
            delete m_model;
        m_model = nullptr;

        m_model = new Model();
        mld->read(m_model);
    }
}

void Renderer::setLightColor(QColor c)
{
    m_lightColor = c;
}

void Renderer::setModelColor(QColor c)
{
    m_modelColor = c;
}

int Renderer::getWidth()
{
	GLint vp[4];
	glGetIntegerv(GL_VIEWPORT, vp);
	return vp[2];
}

int Renderer::getHeight()
{
	GLint vp[4];
	glGetIntegerv(GL_VIEWPORT, vp);
	return vp[3];
}

QColor Renderer::getLightColor()
{
    return m_lightColor;
}

QColor Renderer::getModelColor()
{
    return m_modelColor;
}

void Renderer::mousePressEvent(QMouseEvent *e)
{
    m_mousePressPosition = QVector2D(e->localPos());
    m_mouseLastPosition = m_mousePressPosition;
}

void Renderer::mouseMoveEvent(QMouseEvent *e)
{
    QVector2D diff = QVector2D(e->localPos()) - m_mouseLastPosition;
    m_mouseLastPosition = QVector2D(e->localPos());

    if (e->buttons() == Qt::MiddleButton && e->modifiers() & Qt::ShiftModifier)
    {
        QVector3D v(m_mouseLastPosition - diff);
        QVector3D v2(QVector2D(e->localPos()));

        GLint vp[4];
        glGetIntegerv(GL_VIEWPORT, vp);

        v.setY(vp[3] - v.y());
        v2.setY(vp[3] - v2.y());

        m_modelView.setToIdentity();
        m_modelView.translate(m_translation);
        m_modelView.rotate(m_rotation);
        m_modelView.scale(m_scale);

        v.unproject(m_modelView, m_projection, QRect(vp[0], v[1], vp[2], vp[3]));
        v2.unproject(m_modelView, m_projection, QRect(vp[0], v[1], vp[2], vp[3]));

        QVector3D d = (v2 - v) * m_translationSpeed;
        m_translation += d;
    }
    else if (e->buttons() == Qt::MiddleButton)
    {
        // Rotation axis is perpendicular to the mouse position difference vector
        QVector3D n = QVector3D(diff.y(), diff.x(), 0.0).normalized();

        qreal angle = diff.length() / 10.0;

        m_rotation = QQuaternion::fromAxisAndAngle(n, angle) * m_rotation;
    }

    update();
}

void Renderer::wheelEvent(QWheelEvent* e)
{
    QPoint numPixels = e->pixelDelta();
    QPoint numDegrees = e->angleDelta() / 8;
    qreal d = 0;

    if (!numPixels.isNull())
    {
        d = numPixels.y();
    } else if (!numDegrees.isNull())
    {
        d = (numDegrees / 15).y();
    }

    m_scale *= 1 + d / 100.;

    if (m_scale < 0.05)
        m_scale = 0.05;

    e->accept();
    update();
}

void Renderer::initializeGL()
{
    initializeOpenGLFunctions();

    glClearColor(0, 0, 0, 1);

    // Compile vertex shader
    if (!m_ShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/rsc/vshader.glsl"))
        close();

    // Compile fragment shader
    if (!m_ShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/rsc/fshader.glsl"))
        close();

    // Link shader pipeline
    if (!m_ShaderProgram.link())
        close();

    // Bind shader pipeline for use
    if (!m_ShaderProgram.bind())
        close();

    glEnable(GL_DEPTH_TEST); // Enable depth buffer
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);

    m_model = new Model();

    m_pixBufObj->create();
    m_pixBufObj->setUsagePattern(QOpenGLBuffer::StaticRead);
    m_pixBufObj->bind();
    m_pixBufObj->allocate(geometry().width() * geometry().height() * 4);
    m_pixBufObj->release();

    // Use QBasicTimer because its faster than QTimer
    m_timer.start(12, this);
}

void Renderer::resizeGL(int w, int h)
{
    // Calculate aspect ratio
    qreal aspect = qreal(w) / qreal(h ? h : 1);

    // Set near plane to 3.0, far plane to 7.0, field of view 45 degrees
    const qreal zNear = 1.0, zFar = 200.0, fov = 45.0;

    // Reset projection
    m_projection.setToIdentity();

    // Set perspective projection
    m_projection.perspective(fov, aspect, zNear, zFar);

    m_pixBufObj->bind();
    m_pixBufObj->allocate(w * h * 4);
    m_pixBufObj->release();
}

void Renderer::paintGL()
{
	makeCurrent();

    // Clear color and depth buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!m_model)
        return;

    // Calculate model view transformation
    m_modelView.setToIdentity();
    m_modelView.translate(m_translation);
    m_modelView.translate(m_model->pivot);
    m_modelView.rotate(m_rotation);
    m_modelView.translate(-m_model->pivot);
    m_modelView.scale(m_scale);

    // Set modelview-projection matrix
    m_ShaderProgram.setUniformValue("m_projection", m_projection);
    m_ShaderProgram.setUniformValue("m_model_view", m_modelView);
    m_ShaderProgram.setUniformValue("mvp_matrix", m_projection * m_modelView);
    m_ShaderProgram.setUniformValue("lightPos", QVector3D(0., 0., -1.));
    m_ShaderProgram.setUniformValue("lightColor", QVector4D(m_lightColor.redF(), m_lightColor.greenF(), m_lightColor.blueF(), 1.));
    m_ShaderProgram.setUniformValue("modelColor", QVector4D(m_modelColor.redF(), m_modelColor.greenF(), m_modelColor.blueF(), 1.));
    // Use texture

    // Draw
    m_model->draw(&m_ShaderProgram);

    doneCurrent();
}
