// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/notifications/articlelistnotification.h"

#include "core/articlelistnotificationmodel.h"
#include "miscellaneous/iconfactory.h"

ArticleListNotification::ArticleListNotification(QWidget* parent)
  : BaseToastNotification(parent), m_model(new ArticleListNotificationModel(this)) {
  m_ui.setupUi(this);

  setupHeading(m_ui.m_lblTitle);
  setupCloseButton(m_ui.m_btnClose);

  m_ui.m_btnNextPage->setIcon(qApp->icons()->fromTheme(QSL("arrow-right"), QSL("stock_right")));
  m_ui.m_btnPreviousPage->setIcon(qApp->icons()->fromTheme(QSL("arrow-left"), QSL("stock_left")));
  m_ui.m_btnOpenArticleList->setIcon(qApp->icons()->fromTheme(QSL("view-list-details")));
  m_ui.m_btnOpenWebBrowser->setIcon(qApp->icons()->fromTheme(QSL("document-open")));

  connect(m_model,
          &ArticleListNotificationModel::nextPagePossibleChanged,
          m_ui.m_btnNextPage,
          &PlainToolButton::setEnabled);
  connect(m_model,
          &ArticleListNotificationModel::previousPagePossibleChanged,
          m_ui.m_btnPreviousPage,
          &PlainToolButton::setEnabled);

  m_ui.m_treeArticles->setAttribute(Qt::WA_NoSystemBackground, true);

  auto pal = m_ui.m_treeArticles->palette();

  // Make background transparent.
  pal.setColor(QPalette::ColorRole::Base, Qt::transparent);

  m_ui.m_treeArticles->setPalette(pal);
  m_ui.m_treeArticles->setModel(m_model);

  connect(m_ui.m_cmbFeeds, &QComboBox::currentIndexChanged, this, &ArticleListNotification::showFeed);
}

void ArticleListNotification::loadResults(const QHash<Feed*, QList<Message>>& new_messages) {
  setupTimedClosing();

  m_newMessages = new_messages;

  m_ui.m_lblTitle->setText(tr("%n feeds fetched", nullptr, new_messages.size()));

  m_ui.m_cmbFeeds->clear();

  for (Feed* fd : new_messages.keys()) {
    m_ui.m_cmbFeeds->addItem(fd->sanitizedTitle(), QVariant::fromValue(fd));
  }
}

void ArticleListNotification::showFeed(int index) {
  m_model->setArticles(m_newMessages.value(m_ui.m_cmbFeeds->itemData(index).value<Feed*>()));
}
