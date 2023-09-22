// For license of this file, see <project-root-folder>/LICENSE.md.

#include "core/articlelistnotificationmodel.h"

ArticleListNotificationModel::ArticleListNotificationModel(QObject* parent)
  : QAbstractListModel(parent), m_currentPage(-1) {}

ArticleListNotificationModel::~ArticleListNotificationModel() {}

void ArticleListNotificationModel::setArticles(const QList<Message>& msgs) {
  m_articles = msgs;
  m_currentPage = 0;
}

void ArticleListNotificationModel::nextPage() {}

void ArticleListNotificationModel::previousPage() {}

int ArticleListNotificationModel::rowCount(const QModelIndex& parent) const {}

int ArticleListNotificationModel::columnCount(const QModelIndex& parent) const {}

QVariant ArticleListNotificationModel::data(const QModelIndex& index, int role) const {}
