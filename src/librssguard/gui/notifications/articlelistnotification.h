// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef ARTICLELISTNOTIFICATION_H
#define ARTICLELISTNOTIFICATION_H

#include "gui/notifications/basetoastnotification.h"

#include "core/message.h"

#include "ui_articlelistnotification.h"

class Feed;

class ArticleListNotification : public BaseToastNotification {
    Q_OBJECT

  public:
    explicit ArticleListNotification(QWidget* parent = nullptr);

    void loadResults(const QHash<Feed*, QList<Message>>& new_messages);

  private:
    Ui::ArticleListNotification m_ui;
};

#endif // ARTICLELISTNOTIFICATION_H
