TEMPLATE = lib
TARGET = QtQuickNdi
QT += qml quick quickwidgets multimedia multimediawidgets
CONFIG += plugin c++11 qmltypes metatypes

TARGET = $$qtLibraryTarget($$TARGET)
uri = QtQuickNdi

QML_IMPORT_NAME = QtQuickNdi
QML_IMPORT_MAJOR_VERSION = 1

INCLUDEPATH +=  $$PWD/../../lib/ \
                $$PWD/../../lib/GraphicsCapture/ \
                $$PWD/../../ndi/inc/

QMAKE_CXXFLAGS += /Zc:twoPhase-

win32 {
    LIBS += -lwindowsapp
}

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../../lib/release/ -lQtNdi
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../../lib/debug/ -lQtNdi
else:unix: LIBS += -L$$OUT_PWD/../../lib/ -lQtNdi

DEPENDPATH +=   $$PWD/../../lib/ \
                $$PWD/../../lib/GraphicsCapture/

# Input
SOURCES += \
        qtquickndi_plugin.cpp \
        qtndiitem.cpp

HEADERS += \
        qtquickndi_plugin.h \
        qtndiitem.h

DISTFILES = qmldir

!equals(_PRO_FILE_PWD_, $$OUT_PWD) {
    copy_qmldir.target = $$OUT_PWD/qmldir
    copy_qmldir.depends = $$_PRO_FILE_PWD_/qmldir
    copy_qmldir.commands = $(COPY_FILE) "$$replace(copy_qmldir.depends, /, $$QMAKE_DIR_SEP)" "$$replace(copy_qmldir.target, /, $$QMAKE_DIR_SEP)"
    QMAKE_EXTRA_TARGETS += copy_qmldir
    PRE_TARGETDEPS += $$copy_qmldir.target
}

qmldir.files = qmldir
unix {
    installPath = $$[QT_INSTALL_QML]/$$replace(uri, \., /)
    qmldir.path = $$installPath
    target.path = $$installPath
    INSTALLS += target qmldir
}
