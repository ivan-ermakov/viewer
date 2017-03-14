#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "debug/Stable.h"

#include <QMenu>
#include <QMainWindow>

#include "renderer.h"

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

    Renderer* renderer;

signals:

public slots:
    void openModelDialog();
    void lightColorDialog();
    void modelColorDialog();
};

#endif // MAINWINDOW_H
