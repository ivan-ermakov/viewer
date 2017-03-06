#include "mainwidget.h"

#include <QMouseEvent>
#include <QFileDialog>
#include <QColorDialog>

#include <math.h>

MainWidget::MainWidget(std::string mdl_file, QWidget *parent) :
    QOpenGLWidget(parent),
    menuBar(new QMenuBar(this)),
    geometries(0),
    angularSpeed(0),
    translationSpeed(0.005),
    scale(1),
    translation(0, 0, -5),
    model_file(mdl_file),
    lightColor(Qt::white),
    modelColor(Qt::white)
{    
    openAct = new QAction(tr("&Open"), this);
    openAct->setShortcuts(QKeySequence::Open);
    openAct->setStatusTip(tr("Open model file"));
    connect(openAct, &QAction::triggered, this, &MainWidget::openModelDialog);

    lightColorAct = new QAction(tr("&Light Color"), this);
    lightColorAct->setStatusTip(tr("Set Light Color"));
    connect(lightColorAct, SIGNAL(triggered()), this, SLOT(lightColorDialog()));

    modelColorAct = new QAction(tr("&Model Color"), this);
    modelColorAct->setStatusTip(tr("Set Model Color"));
    connect(modelColorAct, SIGNAL(triggered()), this, SLOT(modelColorDialog()));

    exitAct = new QAction(tr("&Exit"), this);
    exitAct->setShortcuts(QKeySequence::Quit);
    exitAct->setStatusTip(tr("Exit program"));
    connect(exitAct, SIGNAL(triggered()), this, SLOT(close()));

    fileMenu = menuBar->addMenu(tr("&File"));
    fileMenu->addAction(openAct);
    fileMenu->addAction(lightColorAct);
    fileMenu->addAction(modelColorAct);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAct);

    //menuBar->addMenu(fileMenu);

    /*rotateButton = new QPushButton("Rotate", this);
    rotateButton->setGeometry(QRect(QPoint(10, 10), QSize(80, 25)));
    connect(rotateButton, SIGNAL (released()), this, SLOT (handleRotateButton()));

    moveButton = new QPushButton("Move", this);
    moveButton->setGeometry(QRect(QPoint(90, 10), QSize(80, 25)));
    connect(moveButton, SIGNAL (released()), this, SLOT (handleMoveButton()));

    scaleButton = new QPushButton("Scale", this);
    scaleButton->setGeometry(QRect(QPoint(170, 10), QSize(80, 25)));
    connect(scaleButton, SIGNAL (released()), this, SLOT (handleScaleButton()));

    modelButton = new QPushButton("Model", this);
    modelButton->setGeometry(QRect(QPoint(250, 10), QSize(80, 25)));
    connect(modelButton, SIGNAL (released()), this, SLOT (handleModelButton()));*/
}

MainWidget::~MainWidget()
{
    // Make sure the context is current when deleting the texture
    // and the buffers.
    makeCurrent();
    delete geometries;
    doneCurrent();

    delete openAct;
    delete fileMenu;
    delete menuBar;

    //delete rotateButton;
}

void MainWidget::mousePressEvent(QMouseEvent *e)
{
    mousePressPosition = QVector2D(e->localPos());
    mouseLastPosition = mousePressPosition;
}

void MainWidget::mouseReleaseEvent(QMouseEvent *e)
{
}

void MainWidget::mouseMoveEvent(QMouseEvent *e)
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

void MainWidget::wheelEvent(QWheelEvent* e)
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

void MainWidget::timerEvent(QTimerEvent *)
{
    //update();
}

void MainWidget::contextMenuEvent(QContextMenuEvent *event)
{
    /*QMenu menu(this);
    menu.addAction(openAct);
    menu.addAction(exitAct);
    menu.exec(event->globalPos());*/
}



void MainWidget::openModelDialog()
{
    QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("Open File"),"/data/",tr("Wavefront Model Files (*.obj)"));

    if (!fileNames.empty())
    {
        if (model_file != fileNames.first().toStdString())
        {
            model_file = fileNames.first().toStdString();
            delete geometries;
            geometries = nullptr;
            geometries = new Model(model_file);
        }
    }
}

void MainWidget::lightColorDialog()
{
    QColorDialog* cd = new QColorDialog(this);
    lightColor = cd->getColor(Qt::white, this, "Choose light color");
}

void MainWidget::modelColorDialog()
{
    QColorDialog* cd = new QColorDialog(this);
    modelColor = cd->getColor(Qt::white, this, "Choose model color");
}

void MainWidget::initializeGL()
{
    initializeOpenGLFunctions();

    glClearColor(0, 0, 0, 1);

    // Compile vertex shader
    if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/vshader.glsl"))
        close();

    // Compile fragment shader
    if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/fshader.glsl"))
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
    //glEnable(GL_CULL_FACE); // Enable back face culling
    //glDisable(GL_CULL_FACE);

    geometries = new Model(model_file);

    // Use QBasicTimer because its faster than QTimer
    timer.start(12, this);
}

void MainWidget::resizeGL(int w, int h)
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

void MainWidget::paintGL()
{
    // Clear color and depth buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Calculate model view transformation
    modelView.setToIdentity();
    /*modelView.translate(translation);
    modelView.rotate(rotation);*/
    modelView.translate(translation);
    modelView.translate(geometries->pivot);
    modelView.rotate(rotation);
    modelView.translate(-geometries->pivot);
    modelView.scale(scale);

    // Set modelview-projection matrix
    program.setUniformValue("m_projection", projection);
    program.setUniformValue("m_model_view", modelView);
    program.setUniformValue("mvp_matrix", projection * modelView);
    program.setUniformValue("lightPos", QVector3D(0., 0., -1.));
    program.setUniformValue("lightColor", QVector4D(lightColor.redF(), lightColor.greenF(), lightColor.blueF(), 1.));
    program.setUniformValue("modelColor", QVector4D(modelColor.redF(), modelColor.greenF(), modelColor.blueF(), 1.));
    //program.setUniformValue("lightColor", QVector4D(1., 1., 1., 1.));


    // Use texture unit 0 which contains cube.png
    //program.setUniformValue("texture", 0);

    // Draw
    if (geometries)
        geometries->draw(&program);
}
