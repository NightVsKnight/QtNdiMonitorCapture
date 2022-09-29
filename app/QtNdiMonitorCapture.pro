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
    ../lib/ndisender.h \
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
    ../lib/ndisender.cpp \
    ../lib/ndiwrapper.cpp \
    ../support/SimpleCapture.cpp \
    ../support/Win32MonitorEnumeration.cpp

RESOURCES = resources.qrc

TRANSLATIONS += \
    QtNdiMonitorCapture_en_US.ts

CONFIG += lrelease
CONFIG += embed_translations

QMAKE_CXXFLAGS += /Zc:twoPhase-

INCLUDEPATH += "../ndi/inc/"

win32 {
    LIBS += -lwindowsapp
}

CONFIG(package) {
    CONFIG(debug, debug|release) {
        OUT_PWD_CONFIG = $$OUT_PWD/debug
    } else {
        OUT_PWD_CONFIG = $$OUT_PWD/release
    }

    TARGET_FULL_NAME = $${TARGET}
    win32: TARGET_FULL_NAME = $${TARGET_FULL_NAME}.exe

    INSTALLER_DIR = $$PWD/installer
    INSTALLER_CONFIG_DIR = $$INSTALLER_DIR/config
    INSTALLER_PACKAGES_DIR = $$INSTALLER_DIR/packages
    INSTALLER_PACKAGES_APP_DIR = $$INSTALLER_PACKAGES_DIR/stream.NightVsKnight.$$TARGET
    INSTALLER_PACKAGES_APP_DATA_DIR = $$INSTALLER_PACKAGES_APP_DIR/data
    INSTALLER_PACKAGES_APP_META_DIR = $$INSTALLER_PACKAGES_APP_DIR/meta

    # Why were these removed from Qt ~v5?
    win32:QMAKE_DEL_DIR = rmdir /s /q
    win32:QMAKE_MKDIR = mkdir
    win32:QMAKE_COPY_FILE = copy /y

    exists($$shell_path($$INSTALLER_PACKAGES_APP_DATA_DIR)) {
        COMMANDS += "$$QMAKE_DEL_DIR \"$$shell_path($$INSTALLER_PACKAGES_APP_DATA_DIR)\""
    }
    COMMANDS += "$$QMAKE_MKDIR \"$$shell_path($$INSTALLER_PACKAGES_APP_DATA_DIR)\""
    !exists($$shell_path($$INSTALLER_PACKAGES_APP_META_DIR)) {
        COMMANDS += "$$QMAKE_MKDIR \"$$shell_path($$INSTALLER_PACKAGES_APP_META_DIR)\""
    }
    COMMANDS += "$$QMAKE_COPY_FILE \"$$shell_path($$OUT_PWD_CONFIG/$$TARGET_FULL_NAME)\" \"$$shell_path($$INSTALLER_PACKAGES_APP_DATA_DIR)\""
    COMMANDS += "$$QMAKE_COPY_FILE \"$$shell_path($$PWD/../LICENSE.md)\" \"$$shell_path($$INSTALLER_PACKAGES_APP_META_DIR)\""
    win32 {
        COMMANDS += "\"$$shell_path($$[QT_INSTALL_PREFIX]/bin/windeployqt.exe)\" --compiler-runtime \"$$shell_path($$INSTALLER_PACKAGES_APP_DATA_DIR/$$TARGET_FULL_NAME)\""
    }

    QT_IFW = $$shell_path($$[QT_INSTALL_PREFIX]/../../Tools/QtInstallerFramework/4.4)
    INSTALLER_TARGET_FULL_NAME = $${TARGET}Installer
    win32: INSTALLER_TARGET_FULL_NAME = $${INSTALLER_TARGET_FULL_NAME}.exe
    INSTALLER_PATH = $$INSTALLER_DIR/$$INSTALLER_TARGET_FULL_NAME
    win32 {
        COMMANDS += "\"$$shell_path($$QT_IFW/bin/binarycreator.exe)\" --offline-only -t \"$$shell_path($$QT_IFW/bin/installerbase.exe)\" -c \"$$shell_path($$INSTALLER_CONFIG_DIR/config.xml)\" -p \"$$shell_path($$INSTALLER_PACKAGES_DIR)\" \"$$shell_path($$INSTALLER_PATH)\""
    }

    QMAKE_POST_LINK = "cd"
    for(COMMAND,COMMANDS) {
        QMAKE_POST_LINK += && $$COMMAND
    }
}

# Default rules for deployment.
#qnx: target.path = /tmp/$${TARGET}/bin
#else: unix:!android: target.path = /opt/$${TARGET}/bin
#!isEmpty(target.path): INSTALLS += target
