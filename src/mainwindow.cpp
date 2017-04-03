#include "debug/Stable.h"

#include <QMenuBar>
#include <QFileDialog>
#include <QColorDialog>
#include <QGridLayout>

#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    renderer(new Renderer(this)),
    videoRecorder(new VideoRecorder(this, renderer)),
    fpsLabel(new QLabel(this)),
    timerLabel(new QLabel(this))
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

	startRecordAct = new QAction(tr("&Record"), this);
	startRecordAct->setStatusTip(tr("Start recording video"));
	connect(startRecordAct, &QAction::triggered, this, &MainWindow::startRecord);

	/*pauseRecordAct = new QAction(tr("&Pause"), this);
	pauseRecordAct->setStatusTip(tr("Pause video"));
	connect(pauseRecordAct, &QAction::triggered, videoRecorder, &VideoRecorder::pauseRecord);*/

	stopRecordAct = new QAction(tr("&Stop"), this);
	stopRecordAct->setStatusTip(tr("Stop recording video"));
	connect(stopRecordAct, &QAction::triggered, this, &MainWindow::stopRecord);

    bitRateHighAct = new QAction(tr("High Bitrate"), this);
    bitRateHighAct->setStatusTip(tr("Set high bit rate"));
    bitRateHighAct->setEnabled(false);
    connect(bitRateHighAct, &QAction::triggered, this, &MainWindow::setHighBitRate);

    bitRateLowAct = new QAction(tr("Low Bitrate"), this);
    bitRateLowAct->setStatusTip(tr("Set low bit rate"));
    connect(bitRateLowAct, &QAction::triggered, this, &MainWindow::setLowBitRate);

	videoMenu = menuBar()->addMenu(tr("&Video"));
	videoMenu->addAction(startRecordAct);
	//videoMenu->addAction(pauseRecordAct);
	videoMenu->addAction(stopRecordAct);
    videoMenu->addAction(bitRateHighAct);
    videoMenu->addAction(bitRateLowAct);

    fpsLabel->setText("FPS: ");
    fpsLabel->setGeometry(20, 20, 100, 100);
    fpsLabel->setStyleSheet("QLabel { color : white; }");
    fpsLabel->show();
    fpsLabel->activateWindow();
    fpsLabel->raise();

    timerLabel->setText("00:00:00.000");
    timerLabel->setGeometry(20, 40, 100, 100);
    timerLabel->setStyleSheet("QLabel { color : white; }");
    timerLabel->show();
    timerLabel->activateWindow();
    timerLabel->raise();

	videoRecorder->start();

    QTimer* timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(update()));
    timer->start(50);

	/*VideoWriter vw;
	vw.open("test");
	vw.writeVideoFrame(QImage("data/0.png"), 1000);
	vw.writeVideoFrame(QImage("data/1.png"), 1000);
	vw.writeVideoFrame(QImage("data/2.png"), 1000);
	vw.writeVideoFrame(QImage("data/3.png"), 1000);
	vw.writeVideoFrame(QImage("data/4.png"), 1000);
	vw.writeVideoFrame(QImage("data/4.jpg"), 1000);
	vw.close();*/
}

MainWindow::~MainWindow()
{
	videoRecorder->stopRecord();
    videoRecorder->terminate();
	videoRecorder->wait();

	delete videoRecorder;
    delete renderer;

    delete openAct;
	delete lightColorAct;
	delete modelColorAct;
	delete exitAct;
    delete fileMenu;

    delete startRecordAct;
	delete stopRecordAct;
	delete videoMenu;
}

void MainWindow::update()
{
    fpsLabel->setText("FPS: " + QString::number(videoRecorder->getFps()));
    timerLabel->setText(QDateTime::fromMSecsSinceEpoch(videoRecorder->getVideoLength()).toUTC().toString("hh:mm:ss.zzz"));
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    // Maybe swap
    QMainWindow::resizeEvent(event);
    renderer->setGeometry(QRect(0, 0, geometry().width(), geometry().height()));
}

void MainWindow::openModelDialog()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),"/data/",tr("Wavefront Model Files (*.obj)"));

    if (!fileName.isEmpty())
        renderer->loadModel(fileName);
}

void MainWindow::lightColorDialog()
{
    renderer->setLightColor(QColorDialog::getColor(renderer->getLightColor(), this, "Choose light color"));
}

void MainWindow::modelColorDialog()
{
    renderer->setModelColor(QColorDialog::getColor(renderer->getModelColor(), this, "Choose model color"));
}

void MainWindow::startRecord()
{
	if (videoRecorder->isRecording())
	{
		videoRecorder->pauseRecord();
        startRecordAct->setText("Resume");
	}
	else
	{
		videoRecorder->startRecord();
		startRecordAct->setText("Pause");
	}
}

void MainWindow::stopRecord()
{
	videoRecorder->stopRecord();
    startRecordAct->setText("Record");
}

void MainWindow::setHighBitRate()
{
    videoRecorder->setBitRate(5000000);
    bitRateHighAct->setEnabled(false);
    bitRateLowAct->setEnabled(true);
}

void MainWindow::setLowBitRate()
{
    videoRecorder->setBitRate(400000);
    bitRateHighAct->setEnabled(true);
    bitRateLowAct->setEnabled(false);
}
