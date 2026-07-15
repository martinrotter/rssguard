// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef ARTICLELISTNOTIFICATIONMODEL_H
#define ARTICLELISTNOTIFICATIONMODEL_H

#include "core/message.h"

#include <QAbstractListModel>
#include <QFont>

class ArticleListNotificationModel : public QAbstractListModel {
    Q_OBJECT

  public:
    explicit ArticleListNotificationModel(QObject* parent = nullptr);
    ~ArticleListNotificationModel() override = default;

    void setArticles(const QList<Message>& msgs);
    void setFont(const QFont& font);

    Message& message(const QModelIndex& idx);
    const Message& message(const QModelIndex& idx) const;
    void setMessageRead(const QModelIndex& idx, bool read);

    void nextPage();
    void previousPage();

    int rowCount(const QModelIndex& parent) const override;
    int columnCount(const QModelIndex& parent) const override;
    QVariant data(const QModelIndex& index, int role) const override;

  signals:
    void nextPagePossibleChanged(bool possible);
    void previousPagePossibleChanged(bool possible);

  private:
    void emitPageAvailability();
    int articlePosition(const QModelIndex& idx) const;

    bool nextPageAvailable() const;
    bool previousPageAvailable() const;

  private:
    QList<Message> m_articles;
    int m_currentPage;
    QFont m_fontArticlesNormal;
    QFont m_fontArticlesUnread;
};

#endif // ARTICLELISTNOTIFICATIONMODEL_H
