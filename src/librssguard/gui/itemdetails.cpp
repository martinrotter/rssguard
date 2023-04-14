// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/itemdetails.h"

#include "definitions/definitions.h"
#include "services/abstract/rootitem.h"

ItemDetails::ItemDetails(QWidget* parent) : QWidget(parent) {
  m_ui.setupUi(this);
}

ItemDetails::~ItemDetails() {}

void ItemDetails::loadItemDetails(RootItem* item) {
  if (item == nullptr) {
    m_ui.m_lblIcon->setPixmap(QPixmap(APP_ICON_PATH).scaled(128, 128));
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

    m_ui.m_lblIcon->setPixmap(item->fullIcon().pixmap({128, 128}));
    m_ui.m_lblInfo->setText(tool_tip);
  }
}
