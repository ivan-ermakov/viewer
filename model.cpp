#include "model.h"
#include "modelloader.h"

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

Model::Model(QString file)
    : Model()
{
    loadObj(file);
}

Model::~Model()
{
    arrayBuf.destroy();
    indexBuf.destroy();
}

void Model::loadObj(QString filename)
{
    modelFile = filename;

    progress = new QProgressDialog("Loading model...", "Abort", 0, 100, nullptr);

    ModelLoader* mdlLoadThread = new ModelLoader(this);
    mdlLoadThread->setPriority(QThread::HighPriority);
    connect(mdlLoadThread, &ModelLoader::setProgress, progress, &QProgressDialog::setValue);
    connect(mdlLoadThread, &ModelLoader::setMaxProgress, progress, &QProgressDialog::setMaximum);
    mdlLoadThread->connect(mdlLoadThread, &ModelLoader::resultReady, this, &Model::handleResults);
    mdlLoadThread->connect(mdlLoadThread, &ModelLoader::finished, progress, &QObject::deleteLater);
    mdlLoadThread->connect(mdlLoadThread, &ModelLoader::finished, mdlLoadThread, &QObject::deleteLater);
    mdlLoadThread->start();
}

void Model::handleResults(std::vector<Vertex> vdata, std::vector<GLuint> indices)
{
    arrayBuf.bind();
    arrayBuf.allocate(vdata.data(), (int)vdata.size() * sizeof(Vertex));

    indexBuf.bind();
    indexBuf.allocate(indices.data(), (int)indices.size() * sizeof(GLuint));

    progress = nullptr;
}

void Model::draw(QOpenGLShaderProgram *program)
{
    QMutexLocker lck(&mutex);

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
}
