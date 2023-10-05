// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef ARTICLELISTNOTIFICATION_H
#define ARTICLELISTNOTIFICATION_H

#include "gui/notifications/basetoastnotification.h"

#include "core/message.h"

#include "ui_articlelistnotification.h"

class Feed;
class ArticleListNotificationModel;

class ArticleListNotification : public BaseToastNotification {
    Q_OBJECT

  public:
    explicit ArticleListNotification(QWidget* parent = nullptr);

    void loadResults(const QHash<Feed*, QList<Message>>& new_messages);

  signals:
    void openingArticleInArticleListRequested(Feed* feed, const Message& msg);
    void reloadMessageListRequested(bool mark_selected_messages_read);

  public:
    virtual bool eventFilter(QObject* watched, QEvent* event);

  private slots:
    void openArticleInArticleList();
    void onMessageSelected(const QModelIndex& current, const QModelIndex& previous);
    void showFeed(int index);
    void openArticleInWebBrowser();
    void markAllRead();

  private:
    void markAsRead(Feed* feed, const QList<Message>& articles);

    Feed* selectedFeed(int index = -1) const;
    Message selectedMessage() const;

  private:
    Ui::ArticleListNotification m_ui;
    ArticleListNotificationModel* m_model;
    QHash<Feed*, QList<Message>> m_newMessages;
};

#endif // ARTICLELISTNOTIFICATION_H
