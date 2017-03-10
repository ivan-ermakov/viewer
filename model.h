#ifndef MODEL_H
#define MODEL_H

#include <string>
#include <vector>

#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QString>
#include <QMutex>

#include "vertex.h"

// Clean up

class Model : public QObject, protected QOpenGLFunctions
{
   Q_OBJECT

public:
    Model();
    virtual ~Model();

    void draw(QOpenGLShaderProgram *program);
    void load(std::vector<Vertex>&, std::vector<GLuint>&);

    QVector3D pivot;
    QMutex mutex;

    friend class ModelLoader;

private:
    int bufSize;
    QOpenGLBuffer arrayBuf;
    QOpenGLBuffer indexBuf;
};

#endif // MODEL_H
