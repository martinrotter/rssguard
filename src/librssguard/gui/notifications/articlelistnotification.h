// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef ARTICLELISTNOTIFICATION_H
#define ARTICLELISTNOTIFICATION_H

#include "gui/notifications/basetoastnotification.h"

#include "ui_articlelistnotification.h"

class ArticleListNotification : public BaseToastNotification {
    Q_OBJECT

  public:
    explicit ArticleListNotification(QWidget* parent = nullptr);

  private:
    Ui::ArticleListNotification m_ui;
};

#endif // ARTICLELISTNOTIFICATION_H
