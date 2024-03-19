// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef ARTICLELISTNOTIFICATIONMODEL_H
#define ARTICLELISTNOTIFICATIONMODEL_H

#include "core/message.h"

#include <QAbstractListModel>

class ArticleListNotificationModel : public QAbstractListModel {
    Q_OBJECT

  public:
    explicit ArticleListNotificationModel(QObject* parent = nullptr);
    virtual ~ArticleListNotificationModel();

    void setArticles(const QList<Message>& msgs);

    Message message(const QModelIndex& idx) const;

    void nextPage();
    void previousPage();

    virtual int rowCount(const QModelIndex& parent) const;
    virtual int columnCount(const QModelIndex& parent) const;
    virtual QVariant data(const QModelIndex& index, int role) const;

    void reloadWholeLayout();

  signals:
    void nextPagePossibleChanged(bool possible);
    void previousPagePossibleChanged(bool possible);

  private:
    bool nextPageAvailable() const;
    bool previousPageAvailable() const;

  private:
    QList<Message> m_articles;
    int m_currentPage;
};

#endif // ARTICLELISTNOTIFICATIONMODEL_H
