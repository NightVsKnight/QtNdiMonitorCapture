#ifndef QTQUICKNDI_PLUGIN_H
#define QTQUICKNDI_PLUGIN_H

#include <QQmlExtensionPlugin>

class QtQuickNdiPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    void registerTypes(const char *uri) override;
};

#endif // QTQUICKNDI_PLUGIN_H
