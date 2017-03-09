#ifndef MODELLOADER_H
#define MODELLOADER_H

#include <QTimer>
#include <QString>
#include <QVector2D>
#include <QVector3D>
#include <QMutex>
#include <QThread>
#include <QOpenGLFunctions>

#include "model.h"
#include "vertex.h"

// Widget + Thread

class ModelLoader : public QThread, public QOpenGLFunctions
{
    Q_OBJECT

public:
    ModelLoader();
    ModelLoader(Model*, QString);
    ~ModelLoader();

    bool isReady() const;
    int getProgress() const;
    int getMaxProgress() const;
    void cancel();
    //void read(Model*);
    void read();
    //void exec();

private:
    void run() override;

    bool ready;
    bool cancelled;
    int progress;
    int maxProgress;
    QString fileName;
    Model* mdl;
    QMutex* mtx;
    std::vector<Vertex> vdata;
    std::vector<GLuint> indices;

signals:
    void resultReady(std::vector<Vertex> vdata, std::vector<GLuint> indices);
};

#endif // MODELLOADER_H
