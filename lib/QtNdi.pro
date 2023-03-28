QT -= gui
QT += multimedia

TEMPLATE = lib
CONFIG += staticlib

CONFIG += c++17

INCLUDEPATH += GraphicsCapture/

PRECOMPILED_HEADER = GraphicsCapture/pch.h

HEADERS += \
    qtndi.h \
    ndireceiver.h \
    ndireceiverworker.h \
    ndisender.h \
    ndiwrapper.h \
    GraphicsCapture/composition.interop.h \
    GraphicsCapture/d3dHelpers.h \
    GraphicsCapture/direct3d11.interop.h \
    GraphicsCapture/SimpleCapture.h

SOURCES += \
    qtndi.cpp \
    ndireceiver.cpp \
    ndireceiverworker.cpp \
    ndisender.cpp \
    ndiwrapper.cpp \
    GraphicsCapture/SimpleCapture.cpp

DISTFILES += \
    GraphicsCapture/readme.md

QMAKE_CXXFLAGS += /Zc:twoPhase-

INCLUDEPATH += "../ndi/inc/"

# Default rules for deployment.
unix {
    target.path = $$[QT_INSTALL_PLUGINS]/generic
}
!isEmpty(target.path): INSTALLS += target
