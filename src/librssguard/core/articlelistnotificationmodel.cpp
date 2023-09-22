// For license of this file, see <project-root-folder>/LICENSE.md.

#include "core/articlelistnotificationmodel.h"

#include "definitions/definitions.h"

ArticleListNotificationModel::ArticleListNotificationModel(QObject* parent)
  : QAbstractListModel(parent), m_currentPage(-1) {}

ArticleListNotificationModel::~ArticleListNotificationModel() {}

void ArticleListNotificationModel::setArticles(const QList<Message>& msgs) {
  m_articles = msgs;
  m_currentPage = 0;

  reloadWholeLayout();
}

void ArticleListNotificationModel::nextPage() {
  emit nextPagePossibleChanged(true);
  emit previousPagePossibleChanged(true);
}

void ArticleListNotificationModel::previousPage() {
  emit nextPagePossibleChanged(true);
  emit previousPagePossibleChanged(true);
}

int ArticleListNotificationModel::rowCount(const QModelIndex& parent) const {
  return std::min(int(m_articles.size() - (NOTIFICATIONS_PAGE_SIZE * m_currentPage)), NOTIFICATIONS_PAGE_SIZE);
}

int ArticleListNotificationModel::columnCount(const QModelIndex& parent) const {
  return 1;
}

QVariant ArticleListNotificationModel::data(const QModelIndex& index, int role) const {
  switch (role) {
    case Qt::ItemDataRole::DisplayRole:
      return m_articles.at((m_currentPage * NOTIFICATIONS_PAGE_SIZE) + index.row()).m_title;
  }

  return QVariant();
}

void ArticleListNotificationModel::reloadWholeLayout() {
  emit layoutAboutToBeChanged();
  emit layoutChanged();
}
