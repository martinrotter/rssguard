#include <QVariant>

#include "core/defs.h"
#include "gui/iconthemefactory.h"
#include "core/feedsmodelstandardfeed.h"


FeedsModelStandardFeed::FeedsModelStandardFeed(FeedsModelRootItem *parent_item)
  : FeedsModelFeed(parent_item) {
}

FeedsModelStandardFeed::~FeedsModelStandardFeed() {
  qDebug("Destroying FeedsModelStandardFeed instance.");
}

QVariant FeedsModelStandardFeed::data(int column, int role) const {
  switch (role) {
    case Qt::DisplayRole:
      if (column == FDS_TITLE_INDEX) {
        return m_title;
      }
      else if (column == FDS_COUNTS_INDEX) {
        return QString("(%1)").arg(QString::number(m_unreadCount));
      }

    case Qt::DecorationRole:
      return column == FDS_TITLE_INDEX ?
            m_icon :
            QVariant();

    case Qt::TextAlignmentRole:
      if (column == FDS_COUNTS_INDEX) {
        return Qt::AlignRight;
      }
      else {
        return QVariant();
      }

    default:
      return QVariant();
  }
}
