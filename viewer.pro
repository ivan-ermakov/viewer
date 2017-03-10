QT       += core gui widgets opengl

TARGET = viewer
TEMPLATE = app

SOURCES += main.cpp \
    model.cpp \
    modelloader.cpp \
    modelloaddialog.cpp \
    mainwindow.cpp \
    renderer.cpp

SOURCES +=

HEADERS += \
    model.h \
    modelloader.h \
    vertex.h \
    modelloaddialog.h \
    mainwindow.h \
    renderer.h

LIBS += -lopengl32

RESOURCES += \
    shaders.qrc

FORMS +=
