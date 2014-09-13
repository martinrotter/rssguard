#include "core/feedsmodelrecyclebin.h"

#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"


FeedsModelRecycleBin::FeedsModelRecycleBin(FeedsModelRootItem *parent) : FeedsModelRootItem(parent) {
  m_kind = FeedsModelRootItem::RecycleBin;
  m_icon = qApp->icons()->fromTheme("folder-recycle-bin");
  m_id = ID_RECYCLE_BIN;
  m_title = tr("Recycle bin");
  m_description = tr("Recycle bin contains all deleted messages from all feeds.");
  m_creationDate = QDateTime::currentDateTime();
}

FeedsModelRecycleBin::~FeedsModelRecycleBin() {
}


int FeedsModelRecycleBin::childCount() const {
  return 0;
}

void FeedsModelRecycleBin::appendChild(FeedsModelRootItem *child) {
  Q_UNUSED(child)
}

int FeedsModelRecycleBin::countOfUnreadMessages() const {
  return 0;
}

int FeedsModelRecycleBin::countOfAllMessages() const {
  return 0;
}


QVariant FeedsModelRecycleBin::data(int column, int role) const {
  switch (role) {
    case Qt::DisplayRole:
      if (column == FDS_MODEL_TITLE_INDEX) {
        return m_title;
      }
      else if (column == FDS_MODEL_COUNTS_INDEX) {
        return qApp->settings()->value(APP_CFG_FEEDS,
                                       "count_format",
                                       "(%unread)").toString()
            .replace("%unread", QString::number(countOfUnreadMessages()))
            .replace("%all", QString::number(countOfAllMessages()));
      }
      else {
        return QVariant();
      }

    case Qt::EditRole:
      if (column == FDS_MODEL_TITLE_INDEX) {
        return m_title;
      }
      else if (column == FDS_MODEL_COUNTS_INDEX) {
        return countOfUnreadMessages();
      }
      else {
        return QVariant();
      }

    case Qt::FontRole:
      return countOfUnreadMessages() > 0 ? m_boldFont : m_normalFont;

    case Qt::DecorationRole:
      if (column == FDS_MODEL_TITLE_INDEX) {
        return m_icon;
      }
      else {
        return QVariant();
      }

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
