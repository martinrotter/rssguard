// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/itemdetails.h"

#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "network-web/webfactory.h"
#include "services/abstract/rootitem.h"

#include <QScrollBar>

ItemDetails::ItemDetails(QWidget* parent) : QWidget(parent) {
  m_ui.setupUi(this);

  m_ui.m_txtDetails->setAutoFillBackground(false);
  m_ui.m_txtDetails->viewport()->setAutoFillBackground(false);

  connect(m_ui.m_txtDetails, &QTextBrowser::anchorClicked, this, [](const QUrl& link) {
    qApp->web()->openUrlInExternalBrowser(link);
  });
}

ItemDetails::~ItemDetails() {}

void ItemDetails::loadItemDetails(RootItem* item) {
  m_ui.m_txtDetails->verticalScrollBar()->triggerAction(QAbstractSlider::SliderAction::SliderToMinimum);

  if (item == nullptr) {
    m_ui.m_lblIcon->setPixmap(QPixmap(APP_ICON_PATH)
                                .scaled(48,
                                        48,
                                        Qt::AspectRatioMode::KeepAspectRatio,
                                        Qt::TransformationMode::SmoothTransformation));
    m_ui.m_txtDetails->setText(QSL("<b>%1</b>").arg(QSL(APP_LONG_NAME)));
  }
  else {
    QString tool_tip = item->data(FDS_MODEL_TITLE_INDEX, Qt::ItemDataRole::ToolTipRole).toString();

    m_ui.m_lblIcon->setPixmap(item->fullIcon().pixmap({48, 48}));
    m_ui.m_txtDetails->setText(tool_tip);
  }
}
