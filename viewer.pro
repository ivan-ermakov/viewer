QT       += core gui widgets opengl

TARGET = viewer
TEMPLATE = app

INCLUDEPATH += ./src \
    ./libs/ffmpeg/include

HEADERS += ./src/debug/CrashDump.h \
    ./src/debug/DisableMemoryLeak.h \
    ./src/debug/EnableMemoryLeak.h \
    ./src/debug/MemoryLeaksDetection.h \
    ./src/debug/Stable.h \
    ./src/Vertex.h \
    ./src/VideoWriter.h \
    ./src/MainWindow.h \
    ./src/Model.h \
    ./src/ModelLoadDialog.h \
    ./src/ModelLoader.h \
    ./src/Renderer.h \
    ./src/VideoRecorder.h

SOURCES += ./src/debug/CrashDump.cpp \
    ./src/debug/MemoryLeaksDetection.cpp \
    ./src/Main.cpp \
    ./src/MainWindow.cpp \
    ./src/Model.cpp \
    ./src/ModelLoadDialog.cpp \
    ./src/ModelLoader.cpp \
    ./src/Renderer.cpp \
    ./src/VideoWriter.cpp \
    ./src/VideoRecorder.cpp

LIBS += -lshell32 \
    -lopengl32 \
    -luser32 \
    -ldbghelp \
    -L$$PWD"/libs/ffmpeg/lib" \
    -lavcodec \
    -lavformat \
    -lavutil \
    -lswscale \

FORMS +=

DEFINES += _CRTDBG_MAP_ALLOC

RESOURCES += \
    shaders.qrc
