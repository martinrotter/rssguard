// For license of this file, see <project-root-folder>/LICENSE.md.

#include "core/articlelistnotificationmodel.h"

#include "definitions/definitions.h"

ArticleListNotificationModel::ArticleListNotificationModel(QObject* parent)
  : QAbstractListModel(parent), m_currentPage(-1) {}

ArticleListNotificationModel::~ArticleListNotificationModel() {}

void ArticleListNotificationModel::setArticles(const QList<Message>& msgs) {
  m_currentPage = 0;
  m_articles = msgs;

  reloadWholeLayout();
  emit nextPagePossibleChanged(nextPageAvailable());
  emit previousPagePossibleChanged(previousPageAvailable());
}

Message ArticleListNotificationModel::message(const QModelIndex& idx) const {
  return m_articles.at((m_currentPage * NOTIFICATIONS_PAGE_SIZE) + idx.row());
}

void ArticleListNotificationModel::nextPage() {
  m_currentPage++;
  reloadWholeLayout();

  emit nextPagePossibleChanged(nextPageAvailable());
  emit previousPagePossibleChanged(previousPageAvailable());
}

void ArticleListNotificationModel::previousPage() {
  m_currentPage--;
  reloadWholeLayout();

  emit nextPagePossibleChanged(nextPageAvailable());
  emit previousPagePossibleChanged(previousPageAvailable());
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
    case Qt::ItemDataRole::ToolTipRole:
      return m_articles.at((m_currentPage * NOTIFICATIONS_PAGE_SIZE) + index.row()).m_title;
  }

  return QVariant();
}

void ArticleListNotificationModel::reloadWholeLayout() {
  emit layoutAboutToBeChanged();
  emit layoutChanged();
}

bool ArticleListNotificationModel::nextPageAvailable() const {
  return m_articles.size() - (NOTIFICATIONS_PAGE_SIZE * (m_currentPage + 1)) > 0;
}

bool ArticleListNotificationModel::previousPageAvailable() const {
  return m_currentPage > 0;
}
