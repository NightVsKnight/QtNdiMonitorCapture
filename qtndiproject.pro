TEMPLATE = subdirs

SUBDIRS += \
    app/QtNdiMonitorCapture.pro \
    lib/QtNdi.pro \
    plugins/QtQuickNdi \
    examples\QtNdiQmlExample \


QtNdiMonitorCapture.SUBDIRS = app
QtNdi.SUBDIRS = lib
QtQuickNdi.SUBDIRS = plugins
QtNdiQmlExample.SUBDIRS = examples

QtQuickNdi.depends = QtNdi
QtNdiMonitorCapture.depends = QtNdi
QtNdiQmlExample.depends = QtQuickNdi
