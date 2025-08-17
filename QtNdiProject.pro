TEMPLATE = subdirs

SUBDIRS += QtNdi QtNdiMonitorCapture

QtNdi.file = lib/QtNdi.pro

QtNdiMonitorCapture.file = app/QtNdiMonitorCapture.pro
QtNdiMonitorCapture.depends = QtNdi
