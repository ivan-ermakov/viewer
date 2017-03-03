QT       += core gui widgets

TARGET = viewer
TEMPLATE = app

SOURCES += main.cpp \
    model.cpp

SOURCES += \
    mainwidget.cpp

HEADERS += \
    mainwidget.h \
    model.h

RESOURCES += \
    shaders.qrc

FORMS +=
