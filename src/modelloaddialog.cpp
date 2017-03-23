#include <iostream>

#include "debug/Stable.h"

#include "modelloaddialog.h"

ModelLoadDialog::ModelLoadDialog(QWidget *parent, QString fileName) :
    QWidget(parent)
{
    progress = new QProgressDialog("Loading model...", "Abort", 0, 200, this);
    progress->show();

    mdlLoader = new ModelLoader(fileName);
    connect(progress, &QProgressDialog::canceled, mdlLoader, &ModelLoader::cancel);
    mdlLoader->start();

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &ModelLoadDialog::update);
    timer->start(25);

    //progress->setMaximum(mdlLoader->getMaxProgress()); Too early
}

bool ModelLoadDialog::isReady()
{
    return mdlLoader->isReady();
}

const std::vector<Vertex>& ModelLoadDialog::getVertices()
{
    return mdlLoader->getVertices();
}

const std::vector<GLuint>& ModelLoadDialog::getIndices()
{
    return mdlLoader->getIndices();
}

QVector3D ModelLoadDialog::getPivot()
{
    return mdlLoader->getPivot();
}

void ModelLoadDialog::read(Model* mdl)
{
    mdlLoader->read(mdl);
}

void ModelLoadDialog::exec()
{
    progress->exec();
}

void ModelLoadDialog::update()
{
    progress->setValue(mdlLoader->getProgress());
    progress->setMaximum(mdlLoader->getMaxProgress());

    if (mdlLoader->isReady() || mdlLoader->isCancelled())
    {
        timer->stop();

        timer->deleteLater();
        progress->deleteLater();
        mdlLoader->deleteLater();
        deleteLater();
    }
}
