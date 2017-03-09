#include <iostream>

#include "modelloaddialog.h"

ModelLoadDialog::ModelLoadDialog(QWidget *parent, Model* mdl, QString fileName) :
    QWidget(parent)
{
    progress = new QProgressDialog("Loading model...", "Abort", 0, 100, nullptr);
    progress->show();

    mdlLoader = new ModelLoader(mdl, fileName);
    //connect(mdlLoadThread, &ModelLoader::setProgress, progress, &QProgressDialog::setValue);
    //connect(mdlLoadThread, &ModelLoader::setMaxProgress, progress, &QProgressDialog::setMaximum);
    //connect(mdlLoadThread, &ModelLoader::resultReady, mdl, &Model::handleResults);

    connect(progress, &QProgressDialog::canceled, mdlLoader, &ModelLoader::cancel);
    //connect(mdlLoader, &ModelLoader::finished, progress, &QObject::deleteLater);
    //connect(mdlLoader, &ModelLoader::finished, mdlLoader, &QObject::deleteLater);

   // mdlLoader->read(mdl);
    mdlLoader->start();
    timer = new QTimer(0);
    timer->setInterval(1);
    connect(timer, &QTimer::timeout, this, &ModelLoadDialog::update);
    //connect(timer, SIGNAL(timeout()), this, SLOT(update()));
    timer->start();

    //mdlLoader->start();
}

void ModelLoadDialog::update()
{
    std::cout << "TIMERED\n";

    progress->setValue(mdlLoader->getProgress());
    progress->setMaximum(mdlLoader->getMaxProgress());

    if (mdlLoader->isReady())
    {
        timer->stop();
        timer->deleteLater();

        mdlLoader->read();
        std::cout << "FINISH LOAD\n";
    }
}
