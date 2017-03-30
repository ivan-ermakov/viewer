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
    mdl(nullptr),
    angularSpeed(0),
    translationSpeed(0.005),
    scale(1),
    translation(0, 0, -5),
    lightColor(Qt::white),
    modelColor(Qt::white)
{}

Renderer::~Renderer()
{
    // Make sure the context is current when deleting the texture
    // and the buffers.
    makeCurrent();
    delete mdl;
    doneCurrent();
}

QImage& Renderer::getFrameBuffer()
{
	return frameBuffer;
}

void Renderer::updateFrameBuffer()
{
	//makeCurrent();
	frameBuffer = grabFramebuffer();
	//doneCurrent();

	recordFrame();
}

void Renderer::loadModel(QString fileName)
{
    ModelLoadDialog* mld = new ModelLoadDialog(this, fileName);
    mld->exec();

    if (mld->isReady())
    {
        if (mdl)
            delete mdl;
        mdl = nullptr;

        mdl = new Model();
        mld->read(mdl);
    }

    //delete mld; deleteLater
}

void Renderer::setLightColor(QColor c)
{
    lightColor = c;
}

void Renderer::setModelColor(QColor c)
{
    modelColor = c;
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
    return lightColor;
}

QColor Renderer::getModelColor()
{
    return modelColor;
}

void Renderer::mousePressEvent(QMouseEvent *e)
{
    mousePressPosition = QVector2D(e->localPos());
    mouseLastPosition = mousePressPosition;
}

void Renderer::mouseMoveEvent(QMouseEvent *e)
{
    QVector2D diff = QVector2D(e->localPos()) - mouseLastPosition;
    mouseLastPosition = QVector2D(e->localPos());

    if (e->buttons() == Qt::MiddleButton && e->modifiers() & Qt::ShiftModifier)
    {
        QVector3D v(mouseLastPosition - diff);
        QVector3D v2(QVector2D(e->localPos()));

        GLint vp[4];
        glGetIntegerv(GL_VIEWPORT, vp);

        v.setY(vp[3] - v.y());
        v2.setY(vp[3] - v2.y());

        modelView.setToIdentity();
        modelView.translate(translation);
        modelView.rotate(rotation);
        modelView.scale(scale);

        v.unproject(modelView, projection, QRect(vp[0], v[1], vp[2], vp[3]));
        v2.unproject(modelView, projection, QRect(vp[0], v[1], vp[2], vp[3]));

        QVector3D d = (v2 - v) * translationSpeed;
        translation += d;
    }
    else if (e->buttons() == Qt::MiddleButton)
    {
        // Rotation axis is perpendicular to the mouse position difference vector
        QVector3D n = QVector3D(diff.y(), diff.x(), 0.0).normalized();

        qreal angle = diff.length() / 10.0;

        rotation = QQuaternion::fromAxisAndAngle(n, angle) * rotation;
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

    scale *= 1 + d / 100.;

    if (scale < 0.05)
        scale = 0.05;

    e->accept();
    update();
}

void Renderer::initializeGL()
{
    initializeOpenGLFunctions();

    glClearColor(0, 0, 0, 1);

    // Compile vertex shader
    if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/rsc/vshader.glsl"))
        close();

    // Compile fragment shader
    if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/rsc/fshader.glsl"))
        close();

    // Link shader pipeline
    if (!program.link())
        close();

    // Bind shader pipeline for use
    if (!program.bind())
        close();

    glEnable(GL_DEPTH_TEST); // Enable depth buffer
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);

    mdl = new Model();

    // Use QBasicTimer because its faster than QTimer
    timer.start(12, this);
}

void Renderer::resizeGL(int w, int h)
{
    // Calculate aspect ratio
    qreal aspect = qreal(w) / qreal(h ? h : 1);

    // Set near plane to 3.0, far plane to 7.0, field of view 45 degrees
    const qreal zNear = 1.0, zFar = 200.0, fov = 45.0;

    // Reset projection
    projection.setToIdentity();

    // Set perspective projection
    projection.perspective(fov, aspect, zNear, zFar);
}

void Renderer::paintGL()
{
	makeCurrent();
    // Clear color and depth buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!mdl)
        return;

    // Calculate model view transformation
    modelView.setToIdentity();
    /*modelView.translate(translation);
    modelView.rotate(rotation);*/
    modelView.translate(translation);
    modelView.translate(mdl->pivot);
    modelView.rotate(rotation);
    modelView.translate(-mdl->pivot);
    modelView.scale(scale);

    // Set modelview-projection matrix
    program.setUniformValue("m_projection", projection);
    program.setUniformValue("m_model_view", modelView);
    program.setUniformValue("mvp_matrix", projection * modelView);
    program.setUniformValue("lightPos", QVector3D(0., 0., -1.));
    program.setUniformValue("lightColor", QVector4D(lightColor.redF(), lightColor.greenF(), lightColor.blueF(), 1.));
    program.setUniformValue("modelColor", QVector4D(modelColor.redF(), modelColor.greenF(), modelColor.blueF(), 1.));


    // Use texture unit 0 which contains cube.png
    //program.setUniformValue("texture", 0);

    // Draw
    mdl->draw(&program);
	//frameBuffer = grabFramebuffer();
	doneCurrent();

	//if (needNextFrame())
	//recordFrame(grabFramebuffer());
}
