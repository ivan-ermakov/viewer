#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "debug/Stable.h"

#include <QMenu>
#include <QMainWindow>

#include "Renderer.h"
#include "VideoRecorder.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    void resizeEvent(QResizeEvent*);

private:
    QMenu* fileMenu;
    QAction* openAct;
    QAction* modelColorAct;
    QAction* lightColorAct;
    QAction* exitAct;

	QMenu* videoMenu;
    QAction* startRecordAct;
	QAction* stopRecordAct;
    QAction* bitRateHighAct;
    QAction* bitRateLowAct;

    QLabel* fpsLabel;
    QLabel* timerLabel;

    Renderer* renderer;
	VideoRecorder* videoRecorder;

signals:
	void record(QImage);

public slots:
    void update();

    void openModelDialog();
    void lightColorDialog();
    void modelColorDialog();

    void startRecord();
	void stopRecord();
    void setHighBitRate();
    void setLowBitRate();

};

#endif // MAINWINDOW_H
