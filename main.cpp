#include <QApplication>
#include <QLabel>
#include <QSurfaceFormat>

#include "mainwindow.h"
#include "renderer.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    QSurfaceFormat::setDefaultFormat(format);

    app.setApplicationName("Viewer");
    app.setApplicationVersion("1.0");

    qRegisterMetaType<std::vector<Vertex>>("std::vector<Vertex>");
    qRegisterMetaType<std::vector<GLuint>>("std::vector<GLuint>");

#ifndef QT_NO_OPENGL
    MainWindow window;
    window.setGeometry(QRect(50, 50, 1024, 768));
    window.show();
#else
    QLabel note("OpenGL Support required");
    note.show();
#endif

    return app.exec();
}
