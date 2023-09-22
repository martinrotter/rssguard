// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/notifications/articlelistnotification.h"

#include "miscellaneous/iconfactory.h"

ArticleListNotification::ArticleListNotification(QWidget* parent) : BaseToastNotification(parent) {
  m_ui.setupUi(this);

  setupCloseButton(m_ui.m_btnClose);

  m_ui.m_btnNextPage->setIcon(qApp->icons()->fromTheme(QSL("arrow-right"), QSL("stock_right")));
  m_ui.m_btnPreviousPage->setIcon(qApp->icons()->fromTheme(QSL("arrow-left"), QSL("stock_left")));
  m_ui.m_btnOpenArticleList->setIcon(qApp->icons()->fromTheme(QSL("view-list-details")));
  m_ui.m_btnOpenWebBrowser->setIcon(qApp->icons()->fromTheme(QSL("document-open")));
}

void ArticleListNotification::loadResults(const QHash<Feed*, QList<Message>>& new_messages) {
  setupTimedClosing();

  m_ui.m_treeArticles->setModel()
}
