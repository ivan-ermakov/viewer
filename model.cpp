#include "model.h"

#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <iostream>
#include <locale>

#include <QString>
#include <QVector2D>
#include <QVector3D>
#include <QMutexLocker>
#include <QProgressDialog>

Model::Model()
    : indexBuf(QOpenGLBuffer::IndexBuffer)
{
    initializeOpenGLFunctions();

    arrayBuf.create();
    indexBuf.create();
}

Model::~Model()
{
    arrayBuf.destroy();
    indexBuf.destroy();
}

void Model::load(std::vector<Vertex>& vdata, std::vector<GLuint>& indices)
{
    arrayBuf.bind();
    arrayBuf.allocate(vdata.data(), (int)vdata.size() * sizeof(Vertex));

    indexBuf.bind();
    indexBuf.allocate(indices.data(), (int)indices.size() * sizeof(GLuint));

    bufSize = indices.size();
}

void Model::draw(QOpenGLShaderProgram *program)
{
    if (!mutex.tryLock())
        return;

    // Tell OpenGL which VBOs to use
    arrayBuf.bind();
    indexBuf.bind();

    // Tell OpenGL programmable pipeline how to locate vertex position data
    int vertexLocation = program->attributeLocation("vPos");
    program->enableAttributeArray(vertexLocation);
    program->setAttributeBuffer(vertexLocation, GL_FLOAT, 0, 3, sizeof(Vertex));

    int normalLocation = program->attributeLocation("vNormal");
    program->enableAttributeArray(normalLocation);
    program->setAttributeBuffer(normalLocation, GL_FLOAT, sizeof(QVector3D), 3, sizeof(Vertex));

    // Tell OpenGL programmable pipeline how to locate vertex texture coordinate data
    /*int texcoordLocation = program->attributeLocation("a_texcoord");
    program->enableAttributeArray(texcoordLocation);
    program->setAttributeBuffer(texcoordLocation, GL_FLOAT, 0, 2, sizeof(QVector3D));*/

    // Draw cube geometry using indices from VBO 1
    glDrawElements(GL_TRIANGLES, bufSize, GL_UNSIGNED_INT, 0);

    mutex.unlock();
}
