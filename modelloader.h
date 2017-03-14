#ifndef MODELLOADER_H
#define MODELLOADER_H

#include "debug/Stable.h"

#include <QTimer>
#include <QString>
#include <QVector2D>
#include <QVector3D>
#include <QMutex>
#include <QThread>
#include <QOpenGLFunctions>

#include "model.h"

// Widget + Thread

class ModelLoader : public QThread
{
    Q_OBJECT

public:
    ModelLoader();
    ModelLoader(QString);
    ~ModelLoader();

    bool isReady();
    bool isCancelled();
    int getProgress();
    int getMaxProgress();
    const std::vector<Vertex>& getVertices();
    const std::vector<GLuint>& getIndices();
    QVector3D getPivot();
    void cancel();
    void read(Model*);

private:
    void run() override;

    bool ready;
    bool cancelled;
    int progress;
    int maxProgress;
    QString fileName;
    QMutex* mtx;
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    QVector3D pivot;

signals:
    void resultReady(std::vector<Vertex> vertices, std::vector<GLuint> indices);
};

#endif // MODELLOADER_H
