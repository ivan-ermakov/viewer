#include <QApplication>
#include <QLabel>
#include <QSurfaceFormat>

#ifndef QT_NO_OPENGL
#include "mainwidget.h"
#endif

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    QSurfaceFormat::setDefaultFormat(format);

    app.setApplicationName("Viewer");
    app.setApplicationVersion("1.0");

#ifndef QT_NO_OPENGL
    MainWidget widget(argc > 1 ? argv[1] : "data/f16.obj");
    widget.setGeometry(QRect(50, 50, 1024, 768));
    widget.show();
#else
    QLabel note("OpenGL Support required");
    note.show();
#endif

    return app.exec();
}
