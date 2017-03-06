QT       += core gui widgets

TARGET = viewer
TEMPLATE = app

SOURCES += main.cpp \
    model.cpp \
    modelloader.cpp

SOURCES += \
    mainwidget.cpp

HEADERS += \
    mainwidget.h \
    model.h \
    modelloader.h \
    vertex.h

RESOURCES += \
    shaders.qrc

FORMS +=
