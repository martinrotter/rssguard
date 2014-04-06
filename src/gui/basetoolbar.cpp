#include "gui/basetoolbar.h"

#include "definitions/definitions.h"
#include "gui/formmain.h"
#include "miscellaneous/settings.h"

#include <QWidgetAction>


BaseToolBar::BaseToolBar(const QString &title, QWidget *parent)
  : QToolBar(title, parent) {
}

BaseToolBar::~BaseToolBar() {
  qDebug("Destroying BaseToolBar instance.");
}
