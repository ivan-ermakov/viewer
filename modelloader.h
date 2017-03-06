#ifndef MODELLOADER_H
#define MODELLOADER_H

#include <QString>
#include <QVector2D>
#include <QVector3D>
#include <QThread>

#include "model.h"
#include "vertex.h"

class ModelLoader : public QThread
{
    Q_OBJECT

public:

    ModelLoader(Model*);

private:

    void run() override;

    Model* mdl;
    //QString modelFile;

signals:
    void setProgress(int);
    void setMaxProgress(int);
    void resultReady(std::vector<Vertex> vdata, std::vector<GLuint> indices);
};

#endif // MODELLOADER_H
