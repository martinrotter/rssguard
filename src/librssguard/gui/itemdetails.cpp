// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/itemdetails.h"

#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "network-web/webfactory.h"
#include "services/abstract/rootitem.h"

#include <QScrollBar>

ItemDetails::ItemDetails(QWidget* parent) : QWidget(parent) {
  m_ui.setupUi(this);

  m_ui.m_scrollInfo->setStyleSheet(QSL("background-color: transparent;"));

  connect(m_ui.m_lblInfo, &QLabel::linkActivated, this, [](const QString& link) {
    qApp->web()->openUrlInExternalBrowser(link);
  });
}

ItemDetails::~ItemDetails() {}

void ItemDetails::loadItemDetails(RootItem* item) {
  m_ui.m_scrollInfo->verticalScrollBar()->triggerAction(QAbstractSlider::SliderAction::SliderToMinimum);

  if (item == nullptr) {
    m_ui.m_lblIcon->setPixmap(QPixmap(APP_ICON_PATH).scaled(16, 16));
    m_ui.m_lblInfo->setText(QSL("<b>%1</b>").arg(QSL(APP_LONG_NAME)));
  }
  else {
    QString tool_tip = QSL("<b>%1</b>").arg(item->title());
    QString desc = item->description();
    QString extra_tooltip = item->additionalTooltip();

    if (!desc.isEmpty()) {
      tool_tip += QL1S("<br/><br/>") + desc.replace(QSL("\n"), QSL("<br/>"));
    }

    if (!extra_tooltip.isEmpty()) {
      tool_tip += QL1S("<br/><br/>") + extra_tooltip.replace(QSL("\n"), QSL("<br/>"));
    }

    m_ui.m_lblIcon->setPixmap(item->fullIcon().pixmap({16, 16}));
    m_ui.m_lblInfo->setText(tool_tip);
  }
}
