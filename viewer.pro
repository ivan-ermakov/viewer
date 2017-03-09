QT       += core gui widgets

TARGET = viewer
TEMPLATE = app

SOURCES += main.cpp \
    model.cpp \
    modelloader.cpp \
    modelloaddialog.cpp

SOURCES += \
    mainwidget.cpp

HEADERS += \
    mainwidget.h \
    model.h \
    modelloader.h \
    vertex.h \
    modelloaddialog.h

RESOURCES += \
    shaders.qrc

FORMS +=
