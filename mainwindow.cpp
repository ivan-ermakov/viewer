#include <QMenuBar>
#include <QFileDialog>
#include <QColorDialog>

#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    renderer(new Renderer(this))
{
    renderer->setGeometry(geometry());

    openAct = new QAction(tr("&Open"), this);
    openAct->setShortcuts(QKeySequence::Open);
    openAct->setStatusTip(tr("Open model file"));
    connect(openAct, &QAction::triggered, this, &MainWindow::openModelDialog);

    lightColorAct = new QAction(tr("&Light Color"), this);
    lightColorAct->setStatusTip(tr("Set Light Color"));
    connect(lightColorAct, SIGNAL(triggered()), this, SLOT(lightColorDialog()));

    modelColorAct = new QAction(tr("&Model Color"), this);
    modelColorAct->setStatusTip(tr("Set Model Color"));
    connect(modelColorAct, SIGNAL(triggered()), this, SLOT(modelColorDialog()));

    exitAct = new QAction(tr("&Exit"), this);
    exitAct->setShortcuts(QKeySequence::Quit);
    exitAct->setStatusTip(tr("Exit program"));
    connect(exitAct, SIGNAL(triggered()), this, SLOT(close()));

    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(openAct);
    fileMenu->addAction(lightColorAct);
    fileMenu->addAction(modelColorAct);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAct);
}

MainWindow::~MainWindow()
{
    delete renderer;
    delete openAct;
    delete fileMenu;
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    renderer->setGeometry(QRect(0, 0, geometry().width(), geometry().height()));
}

void MainWindow::openModelDialog()
{
    QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("Open File"),"/data/",tr("Wavefront Model Files (*.obj)"));

    if (!fileNames.empty())
    {
        renderer->setModelFile(fileNames.first());
    }
}

void MainWindow::lightColorDialog()
{
    QColorDialog* cd = new QColorDialog(this);
    renderer->setLightColor(cd->getColor(renderer->getLightColor(), this, "Choose light color"));
}

void MainWindow::modelColorDialog()
{
    QColorDialog* cd = new QColorDialog(this);
    renderer->setModelColor(cd->getColor(renderer->getModelColor(), this, "Choose model color"));
}
