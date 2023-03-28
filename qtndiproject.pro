TEMPLATE = subdirs

SUBDIRS += \
    app/QtNdiMonitorCapture.pro \
    lib/QtNdi.pro

QtNdiMonitorCapture.SUBDIRS = app
QtNdi.SUBDIRS = lib

QtNdiMonitorCapture.depends = QtNdi
