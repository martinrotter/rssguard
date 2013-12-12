#include <QVariant>

#include "core/feedsmodelstandardcategory.h"
#include "core/defs.h"


FeedsModelStandardCategory::FeedsModelStandardCategory(FeedsModelRootItem *parent_item)
  : FeedsModelCategory(parent_item) {
}

FeedsModelStandardCategory::~FeedsModelStandardCategory() {
  qDebug("Destroying FeedsModelStandardCategory instance.");
}

QVariant FeedsModelStandardCategory::data(int column, int role) const {
  switch (role) {
    case Qt::DisplayRole:
      if (column == FDS_TITLE_INDEX) {
        return "m_title";
      }
      else if (column == FDS_COUNTS_INDEX) {
        return QString("(%1)").arg(QString::number(countOfUnreadMessages()));
      }

    case Qt::DecorationRole:
      return column == FDS_TITLE_INDEX ?
            m_icon :
            QVariant();

    case Qt::TextAlignmentRole:
      if (column == FDS_COUNTS_INDEX) {
        return Qt::AlignCenter;
      }
      else {
        return QVariant();
      }

    default:
      return QVariant();
  }
}
