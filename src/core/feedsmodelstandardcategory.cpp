#include "core/feedsmodelstandardcategory.h"

#include "core/defs.h"
#include "core/textfactory.h"
#include "gui/iconfactory.h"

#include <QVariant>


FeedsModelStandardCategory::FeedsModelStandardCategory(FeedsModelRootItem *parent_item)
  : FeedsModelCategory(parent_item) {
  m_type = Standard;
}

FeedsModelStandardCategory::~FeedsModelStandardCategory() {
  qDebug("Destroying FeedsModelStandardCategory instance.");
}

QVariant FeedsModelStandardCategory::data(int column, int role) const {
  switch (role) {
    case Qt::ToolTipRole:
      if (column == FDS_MODEL_TITLE_INDEX) {
        return QObject::tr("%1\n\n"
                           "Category type: standard\n"
                           "Creation date: %2%3").arg(m_title,
                                                      m_creationDate.toString(Qt::DefaultLocaleShortDate),
                                                      m_childItems.size() == 0 ?
                                                        QObject::tr("\n\nThis category does not contain any nested items.") :
                                                        "");
      }
      else if (column == FDS_MODEL_COUNTS_INDEX) {
        return QObject::tr("%n unread message(s).", "", countOfUnreadMessages());
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

    case Qt::ForegroundRole:
      if (m_childItems.size() == 0) {
        // TODO: Make this configurable.
        return QColor(Qt::red);
      }
      else {
        return QVariant();
      }

    case Qt::DisplayRole:
      if (column == FDS_MODEL_TITLE_INDEX) {
        return m_title;
      }
      else if (column == FDS_MODEL_COUNTS_INDEX) {
        return QString("(%1)").arg(QString::number(countOfUnreadMessages()));
      }
      else {
        return QVariant();
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
