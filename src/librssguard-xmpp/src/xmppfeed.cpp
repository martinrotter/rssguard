// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/xmppfeed.h"

#include "src/definitions.h"
#include "src/xmppnetwork.h"
#include "src/xmppserviceroot.h"
#include "src/xmppubsubpmanager.h"

#include <librssguard/database/databasequeries.h>
#include <librssguard/definitions/definitions.h>
#include <librssguard/miscellaneous/application.h>
#include <librssguard/miscellaneous/iconfactory.h>

#include <QPointer>
#include <QXmppMamManager.h>
#include <QXmppMucManager.h>
#include <QXmppUtils.h>

XmppFeed::XmppFeed(RootItem* parent) : Feed(parent), m_mucRoom(nullptr) {
  // QTimer::singleShot(5000, this, &XmppFeed::obtainArticles);
}

XmppServiceRoot* XmppFeed::serviceRoot() const {
  return qobject_cast<XmppServiceRoot*>(account());
}

bool XmppFeed::canBeDeleted() const {
  return true;
}

void XmppFeed::deleteItem() {
  /*
  serviceRoot()->network()->subscriptionEdit(QSL(XMPP_API_EDIT_SUBSCRIPTION_DELETE),
                                             customId(),
                                             {},
                                             {},
                                             {},
                                             serviceRoot()->networkProxy());
*/
  removeItself();
  serviceRoot()->requestItemRemoval(this, false);
}

void XmppFeed::obtainPubSubArticles(bool notify_on_finish) {
  QStringList existing_article_ids;

  qApp->database()->worker()->read([&](const QSqlDatabase& db) {
    existing_article_ids << DatabaseQueries::bagOfMessages(db, ServiceRoot::BagOfMessages::Read, this);
    existing_article_ids << DatabaseQueries::bagOfMessages(db, ServiceRoot::BagOfMessages::Unread, this);
  });

  auto service = this->service();

  if (service.isEmpty()) {
    return;
  }

  serviceRoot()->network()->pubSubManager()->requestItemIds(service, customId()).then(this, [=, this](auto result) {
    if (auto items = std::get_if<QVector<QString>>(&result)) {
      // Determine IDs we do not have yet.
      QSet<QString> set_existing(existing_article_ids.cbegin(), existing_article_ids.cend());
      QSet<QString> set_new(items->cbegin(), items->cend());
      QSet<QString> set_to_download = set_new.subtract(set_existing);

      if (set_to_download.isEmpty()) {
        return;
      }

      QStringList list_do_download(set_to_download.cbegin(), set_to_download.cend());

      serviceRoot()
        ->network()
        ->pubSubManager()
        ->requestItems<AtomPubSubBaseItem>(service, customId(), list_do_download.mid(0, 10))
        .then(this, [=, this](auto items_result) {
          if (auto items = std::get_if<QXmppPubSubManager::Items<AtomPubSubBaseItem>>(&items_result)) {
            for (const auto& item : items->items) {
              Message msg = item.message();

              if (msg.m_customId.isEmpty()) {
                continue;
              }

              m_articles.append(msg);
            }

            if (notify_on_finish) {
              emit serviceRoot() -> feedFetchRequested({this});
            }
          };
        });
    }
    else if (QXmppError* error = std::get_if<QXmppError>(&result)) {
      QString desc = XmppSimpleError::fromQXmppError(*error).m_description;

      qDebugNN << LOGSEC_XMPP << "Getting entries IDs failed:" << NONQUOTE_W_SPACE_DOT(desc);
    }
    else {
      qDebugNN << LOGSEC_XMPP << "Getting entries IDs failed with unspecified error.";
    }
  });
}

void XmppFeed::obtainArchivedArticles(bool notify_on_finish) {
  QString param_to = type() == XmppCategory::Type::SingleUserChats ? serviceRoot()->network()->username() : customId();
  QString param_jid = type() == XmppCategory::Type::SingleUserChats ? customId() : QString();

  serviceRoot()
    ->network()
    ->mamManager()
    ->retrieveMessages(param_to, QString(), param_jid)
    .then(this, [=, this](auto result) {
      if (auto data = std::get_if<QXmppMamManager::RetrievedMessages>(&result)) {
        for (const QXmppMessage& msg : data->messages) {
          Message m = articleFromXmppMessage(type(), msg);

          if ((msg.type() != QXmppMessage::Type::Chat && msg.type() != QXmppMessage::Type::GroupChat) ||
              m.m_contents.isEmpty() || m.m_title.isEmpty() || m.m_customId.isEmpty()) {
            continue;
          }

          m_articles.append(m);
        }

        if (notify_on_finish) {
          emit serviceRoot() -> feedFetchRequested({this});
        }
      }
      else if (auto err = std::get_if<QXmppError>(&result)) {
        qDebug() << "MAM error:" << err->description;
      }
    });
}

void XmppFeed::obtainArticles(bool notify_on_finish) {
  switch (type()) {
    case XmppCategory::SingleUserChats:
    case XmppCategory::MultiUserChats:
      obtainArchivedArticles(notify_on_finish);
      break;

    case XmppCategory::PubSubServices:
    case XmppCategory::PubSubPeps:
      obtainPubSubArticles(notify_on_finish);
      break;

    default:
    case XmppCategory::Unknown:
      break;
  }
}

void XmppFeed::storeRealTimeArticle(const Message& message) {
  m_articles.append(message);
}

void XmppFeed::removeItself() {
  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    DatabaseQueries::deleteFeed(db, this, serviceRoot()->accountId());
  });
}

QString XmppFeed::service() const {
  return m_service;
}

void XmppFeed::setService(const QString& service) {
  m_service = service;
}

XmppCategory::Type XmppFeed::type() const {
  return parent() == nullptr ? XmppCategory::Type::Unknown : qobject_cast<XmppCategory*>(parent())->type();
}

QString XmppFeed::extractXmppMessageTitle(const QString& text) {
  static QRegularExpression re(R"(^((?:\S+\s+){0,4}\S+))");

  auto match = re.match(text);

  if (match.hasMatch()) {
    return match.captured(1);
  }

  return text;
}

Message XmppFeed::articleFromXmppMessage(XmppCategory::Type source_type, const QXmppMessage& msg) {
  Message article;
  QDateTime stamp = !msg.stamp().isValid() ? QDateTime::currentDateTimeUtc() : msg.stamp();

  auto fr = msg.from();

  if (source_type == XmppCategory::Type::SingleUserChats) {
    article.m_author = QXmppUtils::jidToBareJid(msg.from());
  }
  else {
    article.m_author = QXmppUtils::jidToResource(msg.from());
  }

  article.m_contents = msg.xhtml();

  if (article.m_contents.isEmpty()) {
    article.m_contents = msg.body();
  }

  article.m_customId = msg.id();
  article.m_createdFromFeed = msg.stamp().isValid();
  article.m_created = stamp;
  article.m_title = msg.subject().isEmpty() ? extractXmppMessageTitle(article.m_contents) : msg.subject();

  return article;
}

void XmppFeed::onJoinedChanged() {
  qDebug() << "[JOINED]" << m_mucRoom->jid() << m_mucRoom->isJoined();
}

void XmppFeed::onError(const QXmppStanza::Error& error) {
  qDebug() << "[ERROR]" << m_mucRoom->jid() << error.text();
}

void XmppFeed::onMucMessageReceived(const QXmppMessage& msg) {
  if (msg.body().isEmpty()) {
    return;
  }

  if (msg.type() == QXmppMessage::Type::Error) {
    qCriticalNN << LOGSEC_XMPP << "Received real-time chat error message:" << QUOTE_W_SPACE_DOT(msg.body());
    return;
  }

  auto* acc = serviceRoot();

  if (acc == nullptr) {
    return;
  }

  Message article = articleFromXmppMessage(type(), msg);

  acc->onRealTimeArticleObtained({}, {}, article, this);
}

void XmppFeed::join(QXmppMucManager* muc_manager) {
  m_mucRoom = muc_manager->addRoom(customId());

  connect(m_mucRoom.data(), &QXmppMucRoom::isJoinedChanged, this, &XmppFeed::onJoinedChanged);
  connect(m_mucRoom.data(), &QXmppMucRoom::error, this, &XmppFeed::onError);
  connect(m_mucRoom.data(), &QXmppMucRoom::messageReceived, this, &XmppFeed::onMucMessageReceived);

  m_mucRoom->setNickName(serviceRoot()->network()->nickName());
  m_mucRoom->join();
}

void XmppFeed::unjoin() {
  if (m_mucRoom == nullptr) {
    return;
  }

  if (m_mucRoom->isJoined()) {
    m_mucRoom->leave();
  }
}

void XmppFeed::setArticles(const QList<Message>& articles) {
  m_articles = articles;
}

QList<Message> XmppFeed::articles() const {
  return m_articles;
}

QVariantHash XmppFeed::customDatabaseData() const {
  QVariantHash data;

  data[QSL("service")] = service();

  return data;
}

void XmppFeed::setCustomDatabaseData(const QVariantHash& data) {
  setService(data[QSL("service")].toString());
}

QString XmppFeed::additionalTooltip() const {
  QString source_str = tr("Type: %1\nService: %2").arg(XmppCategory::typeToString(type()), m_service);

  return source_str + QSL("\n\n") + Feed::additionalTooltip();
}
