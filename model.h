#ifndef GEOMETRYENGINE_H
#define GEOMETRYENGINE_H

#include <string>
#include <vector>

#include <QOpenGLFunctions_4_5_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QString>
#include <QMutex>
#include <QProgressDialog>

#include "vertex.h"

class Model : public QObject, protected QOpenGLFunctions_4_5_Core
{
   Q_OBJECT

public:
    Model();
    Model(QString);
    virtual ~Model();

    void draw(QOpenGLShaderProgram *program);

    QVector3D pivot;
    QString modelFile;
    QMutex mutex;

    friend class ModelLoader;

public slots:
    void handleResults(std::vector<Vertex> vdata, std::vector<GLuint> indices);

private:
    void loadObj(QString);

    int bufSize;
    QOpenGLBuffer arrayBuf;
    QOpenGLBuffer indexBuf;

    QProgressDialog* progress;
};

#endif // GEOMETRYENGINE_H
