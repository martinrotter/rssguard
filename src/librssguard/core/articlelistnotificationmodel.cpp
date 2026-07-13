// For license of this file, see <project-root-folder>/LICENSE.md.

#include "core/articlelistnotificationmodel.h"

#include "definitions/definitions.h"
#include "exceptions/applicationexception.h"

#include <algorithm>

ArticleListNotificationModel::ArticleListNotificationModel(QObject* parent)
  : QAbstractListModel(parent), m_currentPage(0) {}

void ArticleListNotificationModel::setArticles(const QList<Message>& msgs) {
  beginResetModel();
  m_currentPage = 0;
  m_articles = msgs;
  endResetModel();

  emitPageAvailability();
}

const Message& ArticleListNotificationModel::message(const QModelIndex& idx) const {
  if (!idx.isValid()) {
    throw ApplicationException(QSL("message cannot be loaded, wrong index"));
  }

  const int list_position = (m_currentPage * NOTIFICATIONS_PAGE_SIZE) + idx.row();

  if (list_position < 0 || list_position >= m_articles.size()) {
    throw ApplicationException(QSL("message cannot be loaded, wrong index"));
  }

  return m_articles.at(list_position);
}

void ArticleListNotificationModel::nextPage() {
  if (!nextPageAvailable()) {
    return;
  }

  beginResetModel();
  m_currentPage++;
  endResetModel();

  emitPageAvailability();
}

void ArticleListNotificationModel::previousPage() {
  if (!previousPageAvailable()) {
    return;
  }

  beginResetModel();
  m_currentPage--;
  endResetModel();

  emitPageAvailability();
}

void ArticleListNotificationModel::emitPageAvailability() {
  emit nextPagePossibleChanged(nextPageAvailable());
  emit previousPagePossibleChanged(previousPageAvailable());
}

int ArticleListNotificationModel::rowCount(const QModelIndex& parent) const {
  if (parent.isValid()) {
    return 0;
  }

  return std::clamp(int(m_articles.size() - (NOTIFICATIONS_PAGE_SIZE * m_currentPage)), 0, NOTIFICATIONS_PAGE_SIZE);
}

int ArticleListNotificationModel::columnCount(const QModelIndex& parent) const {
  return parent.isValid() ? 0 : 1;
}

QVariant ArticleListNotificationModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid() || index.row() < 0 || index.row() >= rowCount({})) {
    return {};
  }

  switch (role) {
    case Qt::ItemDataRole::DisplayRole:
    case Qt::ItemDataRole::ToolTipRole:
      return m_articles.at((m_currentPage * NOTIFICATIONS_PAGE_SIZE) + index.row()).m_title;
  }

  return {};
}

bool ArticleListNotificationModel::nextPageAvailable() const {
  return m_articles.size() - (NOTIFICATIONS_PAGE_SIZE * (m_currentPage + 1)) > 0;
}

bool ArticleListNotificationModel::previousPageAvailable() const {
  return m_currentPage > 0;
}
