QT += core gui multimedia multimediawidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += ../lib/
INCLUDEPATH += ../support/

PRECOMPILED_HEADER = ../support/pch.h

HEADERS += \
    MainWindow.h \
    ../lib/ndireceiver.h \
    ../lib/ndireceiverworker.h \
    ../lib/ndiwrapper.h \
    ../support/capture.interop.h \
    ../support/composition.interop.h \
    ../support/d3dHelpers.h \
    ../support/direct3d11.interop.h \
    ../support/SafeQueue.h \
    ../support/SimpleCapture.h \
    ../support/Win32MonitorEnumeration.h

SOURCES += \
    main.cpp \
    MainWindow.cpp \
    ../lib/ndireceiver.cpp \
    ../lib/ndireceiverworker.cpp \
    ../lib/ndiwrapper.cpp \
    ../support/SimpleCapture.cpp \
    ../support/Win32MonitorEnumeration.cpp

RESOURCES = resources.qrc

TRANSLATIONS += \
    QtNdiMonitorCapture_en_US.ts

CONFIG += lrelease
CONFIG += embed_translations

QMAKE_CXXFLAGS += /Zc:twoPhase-

INCLUDEPATH += "$$(NDI_Advanced_SDK_DIR)/Include/"

win32 {
    LIBS += -lwindowsapp

    LIBS += -L"$$(NDI_Advanced_SDK_DIR)/Lib/x64/"
    LIBS += -lProcessing.NDI.Lib.Advanced.x64
    NDI_DLL = "$$(NDI_Advanced_SDK_DIR)/Bin/x64/Processing.NDI.Lib.Advanced.x64.dll"

    message(NDI_DLL: ($$NDI_DLL))
    copydata.commands = $(COPY_FILE) \"$$shell_path($$NDI_DLL)\" \"$$shell_path($$OUT_PWD)\"
    first.depends = $(first) copydata
    export(first.depends)
    export(copydata.commands)
    QMAKE_EXTRA_TARGETS += first copydata
}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
