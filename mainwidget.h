#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <string>

#include <QOpenGLWidget>
#include <QOpenGLFunctions_4_5_Core>
#include <QMatrix4x4>
#include <QQuaternion>
#include <QVector2D>
#include <QBasicTimer>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QPushButton>
#include <QMenuBar>
#include <QMenu>
#include <QColor>

#include "model.h"

enum class Tool
{
    Rotate,
    Move,
    Scale
};

class MainWidget : public QOpenGLWidget, public QOpenGLFunctions_4_5_Core
{
    Q_OBJECT

public:
    explicit MainWidget(std::string mdl_file, QWidget *parent = 0);
    ~MainWidget();

protected:
    void mousePressEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void wheelEvent(QWheelEvent*) override;
    void timerEvent(QTimerEvent*) override;
    void contextMenuEvent(QContextMenuEvent*) override;

    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

public slots:
    void openModelDialog();
    void modelColorDialog();

private:
    QMenuBar* menuBar;
    QMenu* fileMenu;
    QAction* openAct;
    QAction* exitAct;

    QBasicTimer timer;
    QOpenGLShaderProgram program;
    Model* geometries;
    std::string model_file;
    //QOpenGLTexture *texture;

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
};

#endif // MAINWIDGET_H
