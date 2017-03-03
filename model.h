#ifndef GEOMETRYENGINE_H
#define GEOMETRYENGINE_H

#include <string>

#include <QOpenGLFunctions_4_5_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>

class Model : protected QOpenGLFunctions_4_5_Core
{
public:
    Model();
    Model(std::string);
    virtual ~Model();

    void draw(QOpenGLShaderProgram *program);

    QVector3D pivot;

private:
    void loadObj(std::string);

    int bufSize;
    QOpenGLBuffer arrayBuf;
    QOpenGLBuffer indexBuf;
};

#endif // GEOMETRYENGINE_H
