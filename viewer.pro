QT       += core gui widgets

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

RESOURCES += \
    shaders.qrc

FORMS +=
