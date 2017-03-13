QT       += core gui widgets opengl

TARGET = viewer
TEMPLATE = app

SOURCES += main.cpp \
    model.cpp \
    modelloader.cpp \
    modelloaddialog.cpp \
    mainwindow.cpp \
    renderer.cpp \
    debug/CrashDump.cpp \
    debug/MemoryLeaksDetection.cpp

HEADERS += \
    model.h \
    modelloader.h \
    vertex.h \
    modelloaddialog.h \
    mainwindow.h \
    renderer.h \
    debug/CrashDump.h \
    debug/DisableMemoryLeak.h \
    debug/EnableMemoryLeak.h \
    debug/MemoryLeaksDetection.h \
    debug/Stable.h

LIBS += -lopengl32 -ldbghelp -luser32

RESOURCES += \
    shaders.qrc

FORMS +=

DEFINES +=_CRTDBG_MAP_ALLOC
