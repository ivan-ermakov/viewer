#ifndef MODEL_H
#define MODEL_H

#include <vector>

#include "debug/Stable.h"

#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QMutex>

#include "vertex.h"

class Model : public QObject
{
    Q_OBJECT

    friend class ModelLoader;

public:
    Model();
    ~Model();

    void draw(QOpenGLShaderProgram *program);
    void load(const std::vector<Vertex>&, const std::vector<GLuint>&, QVector3D);

    QVector3D pivot;

private:
    int bufSize;
    QOpenGLBuffer arrayBuf;
    QOpenGLBuffer indexBuf;
    QMutex mutex;
};

#endif // MODEL_H
