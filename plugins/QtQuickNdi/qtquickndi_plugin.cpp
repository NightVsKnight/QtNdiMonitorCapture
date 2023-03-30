#include "qtquickndi_plugin.h"

#include "qtndiitem.h"

#include <qqml.h>

void QtQuickNdiPlugin::registerTypes(const char *uri)
{
    // @uri QtQuickNdi
    qmlRegisterType<QtNdiItem>(uri, 1, 0, "QtNdiItem");
}

