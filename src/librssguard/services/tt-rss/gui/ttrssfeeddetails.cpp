// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/tt-rss/gui/ttrssfeeddetails.h"

#include "definitions/definitions.h"
#include "services/abstract/category.h"

TtRssFeedDetails::TtRssFeedDetails(QWidget* parent) : QWidget(parent) {
  ui.setupUi(this);

  ui.m_txtUrl->lineEdit()->setPlaceholderText(tr("Full feed URL including scheme"));
  ui.m_txtUrl->lineEdit()->setToolTip(tr("Provide URL for your feed."));

  connect(ui.m_txtUrl->lineEdit(), &BaseLineEdit::textChanged, this, &TtRssFeedDetails::onUrlChanged);
  onUrlChanged(QString());
}

void TtRssFeedDetails::onUrlChanged(const QString& new_url) {
  if (QRegularExpression(QSL(URL_REGEXP)).match(new_url).hasMatch()) {
    // New url is well-formed.
    ui.m_txtUrl->setStatus(LineEditWithStatus::StatusType::Ok, tr("The URL is ok."));
  }
  else if (!new_url.simplified().isEmpty()) {
    // New url is not well-formed but is not empty on the other hand.
    ui.m_txtUrl->setStatus(
      LineEditWithStatus::StatusType::Warning,
      tr(R"(The URL does not meet standard pattern. Does your URL start with "http://" or "https://" prefix.)"));
  }
  else {
    // New url is empty.
    ui.m_txtUrl->setStatus(LineEditWithStatus::StatusType::Error, tr("The URL is empty."));
  }
}

void TtRssFeedDetails::loadCategories(const QList<Category*>& categories,
                                      RootItem* root_item,
                                      RootItem* parent_to_select) {
  ui.m_cmbParentCategory->addItem(root_item->fullIcon(), root_item->title(), QVariant::fromValue((void*)root_item));

  for (Category* category : categories) {
    ui.m_cmbParentCategory->addItem(category->fullIcon(), category->title(), QVariant::fromValue((void*)category));
  }

  if (parent_to_select != nullptr) {
    if (parent_to_select->kind() == RootItem::Kind::Category) {
      ui.m_cmbParentCategory
        ->setCurrentIndex(ui.m_cmbParentCategory->findData(QVariant::fromValue((void*)parent_to_select)));
    }
    else if (parent_to_select->kind() == RootItem::Kind::Feed) {
      int target_item = ui.m_cmbParentCategory->findData(QVariant::fromValue((void*)parent_to_select->parent()));

      if (target_item >= 0) {
        ui.m_cmbParentCategory->setCurrentIndex(target_item);
      }
    }
  }
}
