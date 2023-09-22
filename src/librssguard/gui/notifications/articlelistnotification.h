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

  private slots:
    void onMessageSelected(const QModelIndex& current, const QModelIndex& previous);
    void showFeed(int index);

  private:
    Message selectedMessage() const;

  private:
    Ui::ArticleListNotification m_ui;
    ArticleListNotificationModel* m_model;
    QHash<Feed*, QList<Message>> m_newMessages;
};

#endif // ARTICLELISTNOTIFICATION_H
