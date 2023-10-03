// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/notifications/articlelistnotification.h"

#include "core/articlelistnotificationmodel.h"
#include "database/databasequeries.h"
#include "miscellaneous/iconfactory.h"
#include "network-web/webfactory.h"

#include <QTreeView>

ArticleListNotification::ArticleListNotification(QWidget* parent)
  : BaseToastNotification(parent), m_model(new ArticleListNotificationModel(this)) {
  m_ui.setupUi(this);

  setupHeading(m_ui.m_lblTitle);
  setupCloseButton(m_ui.m_btnClose);

  m_ui.m_btnNextPage->setIcon(qApp->icons()->fromTheme(QSL("arrow-right"), QSL("stock_right")));
  m_ui.m_btnPreviousPage->setIcon(qApp->icons()->fromTheme(QSL("arrow-left"), QSL("stock_left")));
  m_ui.m_btnOpenArticleList->setIcon(qApp->icons()->fromTheme(QSL("view-list-details")));
  m_ui.m_btnOpenWebBrowser->setIcon(qApp->icons()->fromTheme(QSL("document-open")));

  m_ui.m_treeArticles->setModel(m_model);

  connect(m_model,
          &ArticleListNotificationModel::nextPagePossibleChanged,
          m_ui.m_btnNextPage,
          &PlainToolButton::setEnabled);
  connect(m_model,
          &ArticleListNotificationModel::previousPagePossibleChanged,
          m_ui.m_btnPreviousPage,
          &PlainToolButton::setEnabled);
  connect(m_ui.m_btnNextPage, &PlainToolButton::clicked, m_model, &ArticleListNotificationModel::nextPage);
  connect(m_ui.m_btnPreviousPage, &PlainToolButton::clicked, m_model, &ArticleListNotificationModel::previousPage);
  connect(m_ui.m_treeArticles,
          &QAbstractItemView::doubleClicked,
          this,
          &ArticleListNotification::openArticleInWebBrowser);
  connect(m_ui.m_btnOpenWebBrowser, &PlainToolButton::clicked, this, &ArticleListNotification::openArticleInWebBrowser);
  connect(m_ui.m_btnOpenArticleList,
          &PlainToolButton::clicked,
          this,
          &ArticleListNotification::openArticleInArticleList);
  connect(m_ui.m_treeArticles->selectionModel(),
          &QItemSelectionModel::currentChanged,
          this,
          &ArticleListNotification::onMessageSelected);

  m_ui.m_treeArticles->setAttribute(Qt::WA_NoSystemBackground, true);

  auto pal = m_ui.m_treeArticles->palette();

  // Make background transparent.
  pal.setColor(QPalette::ColorRole::Base, Qt::transparent);

  m_ui.m_treeArticles->setPalette(pal);

  connect(m_ui.m_cmbFeeds,
          QOverload<int>::of(&QComboBox::currentIndexChanged),
          this,
          &ArticleListNotification::showFeed);
}

void ArticleListNotification::loadResults(const QHash<Feed*, QList<Message>>& new_messages) {
  setupTimedClosing();

  m_newMessages = new_messages;

  m_ui.m_lblTitle->setText(tr("%n feeds fetched", nullptr, new_messages.size()));

  m_ui.m_cmbFeeds->model()->sort(0, Qt::SortOrder::AscendingOrder);
  m_ui.m_cmbFeeds->clear();

  auto ks = new_messages.keys();

  std::sort(ks.begin(), ks.end(), [](Feed* lhs, Feed* rhs) {
    return QString::compare(lhs->sanitizedTitle(), rhs->sanitizedTitle(), Qt::CaseSensitivity::CaseInsensitive) < 0;
  });

  for (Feed* fd : ks) {
    m_ui.m_cmbFeeds->addItem(fd->sanitizedTitle(), QVariant::fromValue(fd));
  }
}

void ArticleListNotification::openArticleInArticleList() {
  emit openingArticleInArticleListRequested(m_ui.m_cmbFeeds->currentData().value<Feed*>(), selectedMessage());
}

void ArticleListNotification::onMessageSelected(const QModelIndex& current, const QModelIndex& previous) {
  m_ui.m_btnOpenArticleList->setEnabled(current.isValid());

  try {
    Message msg = selectedMessage();

    m_ui.m_btnOpenWebBrowser->setEnabled(!msg.m_url.isEmpty());
  }
  catch (...) {
    m_ui.m_btnOpenWebBrowser->setEnabled(false);
  }
}

void ArticleListNotification::showFeed(int index) {
  m_model->setArticles(m_newMessages.value(selectedFeed()));
  onMessageSelected({}, {});
}

void ArticleListNotification::openArticleInWebBrowser() {
  Feed* fd = selectedFeed();
  Message msg = selectedMessage();

  markAsRead(fd, {msg});
  qApp->web()->openUrlInExternalBrowser(msg.m_url);
}

void ArticleListNotification::markAsRead(Feed* feed, const QList<Message>& articles) {
  ServiceRoot* acc = feed->getParentServiceRoot();
  QStringList message_ids;
  message_ids.reserve(articles.size());

  // Obtain IDs of all desired messages.
  for (const Message& message : articles) {
    message_ids.append(QString::number(message.m_id));
  }

  if (acc->onBeforeSetMessagesRead(feed, articles, RootItem::ReadStatus::Read)) {
    auto db = qApp->database()->driver()->connection(metaObject()->className());

    if (DatabaseQueries::markMessagesReadUnread(db, message_ids, RootItem::ReadStatus::Read)) {
      acc->onAfterSetMessagesRead(feed, articles, RootItem::ReadStatus::Read);
    }
  }
}

Feed* ArticleListNotification::selectedFeed(int index) const {
  if (index < 0) {
    return m_ui.m_cmbFeeds->currentData().value<Feed*>();
  }
  else {
    return m_ui.m_cmbFeeds->itemData(index).value<Feed*>();
  }
}

Message ArticleListNotification::selectedMessage() const {
  if (m_ui.m_treeArticles->currentIndex().isValid()) {
    return m_model->message(m_ui.m_treeArticles->currentIndex());
  }
  else {
    throw ApplicationException(QSL("message cannot be loaded, wrong index"));
  }
}
