// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/notifications/articlelistnotification.h"

#include "core/articlelistnotificationmodel.h"
#include "database/databasequeries.h"
#include "miscellaneous/iconfactory.h"
#include "network-web/webfactory.h"

#include <QItemSelectionModel>
#include <QMouseEvent>
#include <QTreeView>

ArticleListNotification::ArticleListNotification(QWidget* parent)
  : BaseToastNotification(parent), m_model(new ArticleListNotificationModel(this)) {
  m_ui.setupUi(this);

  setupHeading(m_ui.m_lblTitle);
  setupCloseButton(m_ui.m_btnClose);

  m_ui.m_treeArticles->viewport()->installEventFilter(this);

  m_ui.m_btnNextPage->setIcon(qApp->icons()->fromTheme(QSL("arrow-right"), QSL("stock_right")));
  m_ui.m_btnPreviousPage->setIcon(qApp->icons()->fromTheme(QSL("arrow-left"), QSL("stock_left")));
  m_ui.m_btnOpenArticleList->setIcon(qApp->icons()->fromTheme(QSL("view-list-details")));
  m_ui.m_btnOpenWebBrowser->setIcon(qApp->icons()->fromTheme(QSL("document-open")));
  m_ui.m_btnMarkAllRead->setIcon(qApp->icons()->fromTheme(QSL("mail-mark-read")));

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
  connect(m_ui.m_btnMarkAllRead, &PlainToolButton::clicked, this, &ArticleListNotification::markAllRead);
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
          &QItemSelectionModel::currentRowChanged,
          this,
          &ArticleListNotification::onMessageSelected);

  setAttribute(Qt::WidgetAttribute::WA_ShowWithoutActivating, true);
  m_ui.m_treeArticles->setAttribute(Qt::WidgetAttribute::WA_NoSystemBackground, true);

  // Make background transparent.
  auto pal = m_ui.m_treeArticles->palette();
  pal.setColor(QPalette::ColorRole::Base, Qt::transparent);
  m_ui.m_treeArticles->setPalette(pal);

  connect(m_ui.m_cmbFeeds,
          QOverload<int>::of(&QComboBox::currentIndexChanged),
          this,
          &ArticleListNotification::showFeed);
}

void ArticleListNotification::loadResults(const QHash<Feed*, QList<Message>>& new_messages) {
  qDebugNN << LOGSEC_NOTIFICATIONS << "Setting up RESULTS notification.";
  setupTimedClosing(false);

  m_newMessages = new_messages;

  m_ui.m_cmbFeeds->model()->sort(0, Qt::SortOrder::AscendingOrder);
  m_ui.m_cmbFeeds->clear();

  auto ks = new_messages.keys();

  std::sort(ks.begin(), ks.end(), [](Feed* lhs, Feed* rhs) {
    return QString::compare(lhs->sanitizedTitle(), rhs->sanitizedTitle(), Qt::CaseSensitivity::CaseInsensitive) < 0;
  });

  for (Feed* fd : ks) {
    if (fd->isQuiet()) {
      continue;
    }

    if (m_newMessages[fd].size() > 0) {
      m_ui.m_cmbFeeds->addItem(fd->sanitizedTitle(), QVariant::fromValue(fd));
    }
  }

  m_ui.m_lblTitle->setText(tr("%n feeds fetched", nullptr, m_ui.m_cmbFeeds->count()));
  m_ui.m_lblTitle->setToolTip(m_ui.m_lblTitle->text());
}

void ArticleListNotification::openArticleInArticleList() {
  emit openingArticleInArticleListRequested(m_ui.m_cmbFeeds->currentData().value<Feed*>(), selectedMessage());

  if (m_newMessages.size() == 1 && m_newMessages.value(m_newMessages.keys().at(0)).size() == 1) {
    // We only have 1 message in 1 feed.
    emit closeRequested(this, false);
  }
}

void ArticleListNotification::onMessageSelected(const QModelIndex& current, const QModelIndex& previous) {
  Q_UNUSED(previous)

  m_ui.m_btnOpenArticleList->setEnabled(m_ui.m_treeArticles->currentIndex().isValid());

  try {
    Message msg = selectedMessage();

    m_ui.m_btnOpenWebBrowser->setEnabled(!msg.m_url.isEmpty());
  }
  catch (...) {
    m_ui.m_btnOpenWebBrowser->setEnabled(false);
  }
}

void ArticleListNotification::showFeed(int index) {
  Q_UNUSED(index)
  m_model->setArticles(m_newMessages.value(selectedFeed()));
  onMessageSelected({}, {});
}

void ArticleListNotification::openArticleInWebBrowser() {
  Feed* fd = selectedFeed();
  Message msg = selectedMessage();

  markAsRead(fd, {msg});
  emit dataChangeNotificationTriggered(fd, FeedsModel::ExternalDataChange::MarkedRead);

  qApp->web()->openUrlInExternalBrowser(msg.m_url);

  if (m_newMessages.size() == 1 && m_newMessages.value(m_newMessages.keys().at(0)).size() == 1) {
    // We only have 1 message in 1 feed.
    emit closeRequested(this, false);
  }
}

void ArticleListNotification::markAllRead() {
  for (Feed* fd : m_newMessages.keys()) {
    markAsRead(fd, m_newMessages.value(fd));
  }

  emit dataChangeNotificationTriggered(nullptr, FeedsModel::ExternalDataChange::MarkedRead);
}

void ArticleListNotification::markAsRead(Feed* feed, const QList<Message>& articles) {
  ServiceRoot* acc = feed->account();
  QStringList message_ids;
  message_ids.reserve(articles.size());

  // Obtain IDs of all desired messages.
  for (const Message& message : articles) {
    message_ids.append(QString::number(message.m_id));
  }

  acc->onBeforeSetMessagesRead(feed, articles, RootItem::ReadStatus::Read);
  auto db = qApp->database()->driver()->connection(metaObject()->className());

  DatabaseQueries::markMessagesReadUnread(db, message_ids, RootItem::ReadStatus::Read);
  acc->onAfterSetMessagesRead(feed, articles, RootItem::ReadStatus::Read);
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

bool ArticleListNotification::eventFilter(QObject* watched, QEvent* event) {
  if (event->type() == QEvent::Type::MouseButtonRelease) {
    if (dynamic_cast<QMouseEvent*>(event)->button() == Qt::MouseButton::MiddleButton) {
      openArticleInArticleList();
    }
  }

  return BaseToastNotification::eventFilter(watched, event);
}
