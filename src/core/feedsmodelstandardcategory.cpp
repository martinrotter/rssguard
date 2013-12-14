#include <QVariant>

#include "core/textfactory.h"
#include "core/feedsmodelstandardcategory.h"
#include "core/defs.h"
#include "gui/iconfactory.h"


FeedsModelStandardCategory::FeedsModelStandardCategory(FeedsModelRootItem *parent_item)
  : FeedsModelCategory(parent_item) {
  m_type = Standard;
}

FeedsModelStandardCategory::~FeedsModelStandardCategory() {
  qDebug("Destroying FeedsModelStandardCategory instance.");
}

QVariant FeedsModelStandardCategory::data(int column, int role) const {
  switch (role) {
    case Qt::DisplayRole:
      if (column == FDS_MODEL_TITLE_INDEX) {
        return QString("%1%2").arg(m_title, "-C");
      }
      else if (column == FDS_MODEL_COUNTS_INDEX) {
        return QString("(%1)").arg(QString::number(countOfUnreadMessages()));
      }

    case Qt::DecorationRole:
      return column == FDS_MODEL_TITLE_INDEX ?
            m_icon :
            QVariant();

    case Qt::TextAlignmentRole:
      if (column == FDS_MODEL_COUNTS_INDEX) {
        return Qt::AlignCenter;
      }
      else {
        return QVariant();
      }

    default:
      return QVariant();
  }
}

void FeedsModelStandardCategory::update() {
  // Update all children.
  foreach (FeedsModelRootItem *child, m_childItems) {
    child->update();
  }
}

FeedsModelStandardCategory *FeedsModelStandardCategory::loadFromRecord(const QSqlRecord &record) {
  FeedsModelStandardCategory *category = new FeedsModelStandardCategory(NULL);

  category->setId(record.value(CAT_DB_ID_INDEX).toInt());
  category->setTitle(record.value(CAT_DB_TITLE_INDEX).toString());
  category->setDescription(record.value(CAT_DB_DESCRIPTION_INDEX).toString());
  category->setCreationDate(QDateTime::fromString(record.value(CAT_DB_DCREATED_INDEX).toString(),
                                                  Qt::ISODate));
  category->setIcon(IconFactory::fromByteArray(record.value(CAT_DB_ICON_INDEX).toByteArray()));

  return category;
}
