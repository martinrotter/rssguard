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
#include <QXmppMucManager.h>
#include <QXmppUtils.h>

XmppFeed::XmppFeed(RootItem* parent) : Feed(parent), m_type(Type::PubSubNode) {
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

void XmppFeed::obtainArticles() {
  QStringList existing_article_ids;

  qApp->database()->worker()->read([&](const QSqlDatabase& db) {
    existing_article_ids << DatabaseQueries::bagOfMessages(db, ServiceRoot::BagOfMessages::Read, this);
    existing_article_ids << DatabaseQueries::bagOfMessages(db, ServiceRoot::BagOfMessages::Unread, this);
  });

  auto service = serviceName();

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

void XmppFeed::storeRealTimeArticle(const Message& message) {
  m_articles.append(message);
}

void XmppFeed::removeItself() {
  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    DatabaseQueries::deleteFeed(db, this, serviceRoot()->accountId());
  });
}

XmppFeed::Type XmppFeed::type() const {
  return m_type;
}

void XmppFeed::setType(Type type) {
  m_type = type;
}

QString XmppFeed::extractXmppMessageTitle(const QString& text) {
  static QRegularExpression re(R"(^((?:\S+\s+){0,4}\S+))");

  auto match = re.match(text);

  if (match.hasMatch()) {
    return match.captured(1);
  }

  return text;
}

void XmppFeed::join(QXmppMucManager* muc_manager) {
  m_mucRoom = muc_manager->addRoom(customId());

  // JOIN STATE
  QObject::connect(m_mucRoom, &QXmppMucRoom::isJoinedChanged, [this]() {
    qDebug() << "[JOINED]" << m_mucRoom->jid() << m_mucRoom->isJoined();
  });

  // ERRORS
  QObject::connect(m_mucRoom, &QXmppMucRoom::error, [this](const QXmppStanza::Error& err) {
    qDebug() << "[ERROR]" << m_mucRoom->jid() << err.text();
  });

  // MESSAGES
  QObject::connect(m_mucRoom, &QXmppMucRoom::messageReceived, [this](const QXmppMessage& msg) {
    QString body = msg.body();

    if (body.isEmpty() || msg.type() != QXmppMessage::Type::GroupChat) {
      return;
    }

    Message article;
    QDateTime stamp = !msg.stamp().isValid() ? QDateTime::currentDateTimeUtc() : msg.stamp();

    article.m_author = QXmppUtils::jidToResource(msg.from());
    article.m_contents = body;
    article.m_customId = msg.id();
    article.m_createdFromFeed = msg.stamp().isValid();
    article.m_created = stamp;
    article.m_title = msg.subject().isEmpty() ? extractXmppMessageTitle(body) : msg.subject();

    m_articles.append(article);

    if (!isSwitchedOff()) {
      emit serviceRoot() -> feedFetchRequested({this});
    }
  });

  m_mucRoom->setNickName(QSL("rssguard_%1").arg(QString::number(QRandomGenerator::global()->generate())));
  m_mucRoom->join();
}

QString XmppFeed::typeToString(Type type) {
  switch (type) {
    case Type::PubSubNode:
      return QSL("PubSub");

    case Type::Chatroom:
      return QSL("MUC");

    default:
      return QSL("-");
  }
}

void XmppFeed::setArticles(const QList<Message>& articles) {
  m_articles = articles;
}

QString XmppFeed::serviceName() const {
  return parent() == nullptr ? QString() : parent()->customId();
}

QList<Message> XmppFeed::articles() const {
  return m_articles;
}

QVariantHash XmppFeed::customDatabaseData() const {
  QVariantHash data;

  data[QSL("type")] = int(type());

  return data;
}

void XmppFeed::setCustomDatabaseData(const QVariantHash& data) {
  int type = data[QSL("type")].toInt();

  if (type > 0) {
    setType(Type(type));
  }
}

QString XmppFeed::additionalTooltip() const {
  QString source_str = tr("Type: %1").arg(typeToString(type()));

  return source_str + QSL("\n\n") + Feed::additionalTooltip();
}
