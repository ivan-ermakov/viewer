#ifndef MODELLOADDIALOG_H
#define MODELLOADDIALOG_H

#include <QString>
#include <QWidget>
#include <QProgressDialog>

#include "model.h"
#include "modelloader.h"

class ModelLoadDialog : public QWidget
{
    Q_OBJECT

public:
    ModelLoadDialog(QWidget*, QString);

    bool isReady();
    const std::vector<Vertex>& getVertices();
    const std::vector<GLuint>& getIndices();
    QVector3D getPivot();
    void read(Model* mdl);

    void exec();

private:
    QTimer* timer;
    QProgressDialog* progress;
    ModelLoader* mdlLoader;

public slots:
    void update();
};

#endif // MODELLOADDIALOG_H
