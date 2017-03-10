#include <iostream>

#include "modelloaddialog.h"

ModelLoadDialog::ModelLoadDialog(QWidget *parent, Model* mdl, QString fileName) :
    QWidget(parent)
{
    progress = new QProgressDialog("Loading model...", "Abort", 0, 200, this);
    progress->show();

    mdlLoader = new ModelLoader(mdl, fileName);
    connect(progress, &QProgressDialog::canceled, mdlLoader, &ModelLoader::cancel);
    mdlLoader->start();

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &ModelLoadDialog::update);
    timer->start(25);

    //progress->setMaximum(mdlLoader->getMaxProgress()); Too early
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

        mdlLoader->read();

        timer->deleteLater();
        progress->deleteLater();
        mdlLoader->deleteLater();
        deleteLater();
    }
}
