// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/notifications/articlelistnotification.h"

ArticleListNotification::ArticleListNotification(QWidget* parent) : BaseToastNotification(parent) {
  m_ui.setupUi(this);

  setupCloseButton(m_ui.m_btnClose);
  setupTimedClosing();
}
