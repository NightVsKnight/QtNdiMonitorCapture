QT -= gui
QT += multimedia

TEMPLATE = lib
CONFIG += staticlib

CONFIG += c++17

HEADERS += \
    qtndi.h \
    ndireceiver.h \
    ndireceiverworker.h \
    ndisender.h \
    ndiwrapper.h

SOURCES += \
    qtndi.cpp \
    ndireceiver.cpp \
    ndireceiverworker.cpp \
    ndisender.cpp \
    ndiwrapper.cpp

QMAKE_CXXFLAGS += /Zc:twoPhase-

INCLUDEPATH += "../ndi/inc/"

# Default rules for deployment.
unix {
    target.path = $$[QT_INSTALL_PLUGINS]/generic
}
!isEmpty(target.path): INSTALLS += target
