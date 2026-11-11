// For license of this file, see <project-root-folder>/LICENSE.md.
// Offline wire test: real STARTTLS, SASL, discovery and PubSub IQ/event traffic.
#include "src/xmppubsubpmanager.h"
#include "tls_fixture.h"

#include <QNetworkProxy>
#include <QPluginLoader>
#include <QSignalSpy>
#include <QSslCertificate>
#include <QSslKey>
#include <QSslSocket>
#include <QTest>
#include <QXmppClient.h>
#include <QXmppConfiguration.h>
#include <QXmppDiscoveryManager.h>
#include <QXmppIncomingClient.h>
#include <QXmppPasswordChecker.h>
#include <QXmppPubSubSubscription.h>
#include <QXmppServer.h>

namespace {
  const QString node = QStringLiteral("rssguard-test");
  const QString atom = QStringLiteral(
    "<entry xmlns='http://www.w3.org/2005/Atom'><id>atom-id</id>"
    "<title>Windows 8.1 PubSub article</title><author><name>Fixture Author</name></author>"
    "<updated>2026-01-01T12:00:00Z</updated><content type='html'>&lt;p&gt;Delivered over TLS&lt;/p&gt;</content>"
    "<link href='https://example.invalid/article'/></entry>");

  class Passwords : public QXmppPasswordChecker {
    QXmppPasswordReply::Error getPassword(const QXmppPasswordRequest& request, QString& password) override {
      if (request.username() != QStringLiteral("reader")) {
        return QXmppPasswordReply::AuthorizationError;
      }
      password = QStringLiteral("fixture-only");
      return QXmppPasswordReply::NoError;
    }
  };

  class Fixture : public QObject {
    public:
      Passwords passwords;
      QXmppSslServer listener;
      QXmppIncomingClient* stream = nullptr;
      bool encrypted = false;
      bool subscribed = false;
      int itemRequests = 0;

      Fixture() {
        listener.setLocalCertificate(QSslCertificate(testCertificate));
        listener.setPrivateKey(QSslKey(testPrivateKey, QSsl::Rsa));
        connect(&listener, &QXmppSslServer::newConnection, this, [this](QSslSocket* socket) {
          connect(socket, &QSslSocket::encrypted, this, [this] { encrypted = true; });
          stream = new QXmppIncomingClient(socket, QStringLiteral("localhost"), this);
          socket->setParent(stream);
          stream->setPasswordChecker(&passwords);
          connect(stream, &QXmppIncomingClient::elementReceived, this, [this](const QDomElement& stanza) {
            if (stanza.tagName() != QStringLiteral("iq")) {
              return;
            }
            const auto child = stanza.firstChildElement();
            QString payload;
            if (child.namespaceURI() == QStringLiteral("http://jabber.org/protocol/disco#info")) {
              payload = QStringLiteral("<query xmlns='http://jabber.org/protocol/disco#info'>"
                                       "<identity category='pubsub' type='service' name='Fixture'/>"
                                       "<feature var='http://jabber.org/protocol/pubsub'/></query>");
            }
            else if (child.namespaceURI() == QStringLiteral("http://jabber.org/protocol/pubsub")) {
              const auto operation = child.firstChildElement();
              const auto name = operation.tagName();
              if (name == QStringLiteral("subscribe")) {
                subscribed = true;
                payload = QStringLiteral("<pubsub xmlns='http://jabber.org/protocol/pubsub'>"
                                         "<subscription node='rssguard-test' jid='reader@localhost' subscription='subscribed'/>"
                                         "</pubsub>");
              }
              else if (name == QStringLiteral("subscriptions")) {
                payload = QStringLiteral("<pubsub xmlns='http://jabber.org/protocol/pubsub'><subscriptions>"
                                         "<subscription node='rssguard-test' jid='reader@localhost' subscription='subscribed'/>"
                                         "</subscriptions></pubsub>");
              }
              else if (name == QStringLiteral("items")) {
                ++itemRequests;
                payload = QStringLiteral("<pubsub xmlns='http://jabber.org/protocol/pubsub'>"
                                         "<items node='rssguard-test'><item id='item-42'>%1</item></items></pubsub>").arg(atom);
              }
            }
            else if (child.namespaceURI() == QStringLiteral("http://jabber.org/protocol/disco#items")) {
              payload = QStringLiteral("<query xmlns='http://jabber.org/protocol/disco#items' node='rssguard-test'>"
                                       "<item jid='localhost' node='rssguard-test' name='item-42'/></query>");
            }
            if (!payload.isEmpty()) {
              QDomDocument reply;
              reply.setContent(QStringLiteral("<iq type='result'>%1</iq>").arg(payload), true);
              auto iq = reply.documentElement();
              iq.setAttribute(QStringLiteral("id"), stanza.attribute(QStringLiteral("id")));
              iq.setAttribute(QStringLiteral("to"), stanza.attribute(QStringLiteral("from")));
              iq.setAttribute(QStringLiteral("from"), QStringLiteral("localhost"));
              stream->sendData(reply.toByteArray(-1));
            }
          });
        });
      }
      ~Fixture() override {
        // Destroy streams while their password checker still exists.
        delete stream;
      }

      QXmppConfiguration configuration(bool trust = true) const {
        QXmppConfiguration config;
        config.setHost(QStringLiteral("127.0.0.1"));
        config.setDomain(QStringLiteral("localhost"));
        config.setPort(listener.serverPort());
        config.setUser(QStringLiteral("reader"));
        config.setPassword(QStringLiteral("fixture-only"));
        config.setResource(QStringLiteral("rssguard-test"));
        config.setAutoReconnectionEnabled(false);
        config.setStreamSecurityMode(QXmppConfiguration::TLSRequired);
        config.setUseSasl2Authentication(true);
        config.setSaslAuthMechanism(QStringLiteral("PLAIN"));
        config.setDisabledSaslMechanisms({}); // fixture-only, over required TLS
        config.setNetworkProxy(QNetworkProxy::NoProxy);
        if (trust) {
          config.setCaCertificates({QSslCertificate(testCertificate)});
        }
        return config;
      }
  };
}

class TestXmppPubSub : public QObject {
  Q_OBJECT
  private slots:
    void tlsAndPubSub() {
      QVERIFY2(QSslSocket::supportsSsl(), "TLS backend or its OpenSSL dependencies are missing");
      Fixture fixture;
      QVERIFY(fixture.listener.listen(QHostAddress::LocalHost, 0));
      QXmppClient client;
      auto* pubsub = new PubSubManager;
      client.addExtension(pubsub);
      int received = 0;
      Message pushed;
      connect(pubsub, &PubSubManager::realTimeArticleObtained, this,
              [&](const QString& service, const QString& receivedNode, const Message& message) {
        QCOMPARE(service, QStringLiteral("localhost"));
        QCOMPARE(receivedNode, node);
        pushed = message;
        ++received;
      });
      client.connectToServer(fixture.configuration());
      QTRY_VERIFY_WITH_TIMEOUT(client.isConnected(), 10000);
      QVERIFY(fixture.encrypted);

      auto* disco = client.findExtension<QXmppDiscoveryManager>();
      QVERIFY(disco);
      bool discovered = false;
      disco->info(QStringLiteral("localhost")).then(this, [&](auto result) {
        const auto* info = std::get_if<QXmppDiscoInfo>(&result);
        QVERIFY(info);
        QVERIFY(info->features().contains(QStringLiteral("http://jabber.org/protocol/pubsub")));
        discovered = true;
      });
      QTRY_VERIFY(discovered);

      bool subscribed = false;
      pubsub->subscribeToNode(QStringLiteral("localhost"), node, QStringLiteral("reader@localhost"))
        .then(this, [&](auto result) { QVERIFY(!std::holds_alternative<QXmppError>(result)); subscribed = true; });
      QTRY_VERIFY(subscribed);
      QVERIFY(fixture.subscribed);

      bool subscriptions = false;
      pubsub->requestSubscriptions(QStringLiteral("localhost")).then(this, [&](auto result) {
        const auto* list = std::get_if<QList<QXmppPubSubSubscription>>(&result);
        QVERIFY(list);
        QCOMPARE(list->size(), 1);
        QCOMPARE(list->first().node(), node);
        subscriptions = true;
      });
      QTRY_VERIFY(subscriptions);

      bool identifiers = false;
      pubsub->requestItemIds(QStringLiteral("localhost"), node).then(this, [&](auto result) {
        const auto* ids = std::get_if<QVector<QString>>(&result);
        QVERIFY(ids);
        QCOMPARE(*ids, QStringList({QStringLiteral("item-42")}));
        identifiers = true;
      });
      QTRY_VERIFY(identifiers);

      bool fetched = false;
      pubsub->requestItems<AtomPubSubBaseItem>(QStringLiteral("localhost"), node, {QStringLiteral("item-42")})
        .then(this, [&](auto result) {
          const auto* items = std::get_if<QXmppPubSubManager::Items<AtomPubSubBaseItem>>(&result);
          QVERIFY(items);
          QCOMPARE(items->items.size(), 1);
          QCOMPARE(items->items.first().message().m_title, QStringLiteral("Windows 8.1 PubSub article"));
          fetched = true;
        });
      QTRY_VERIFY(fetched);

      // The production manager must fetch the full Atom item after a push event.
      fixture.stream->sendData("<message from='localhost' to='reader@localhost/rssguard-test'>"
                               "<event xmlns='http://jabber.org/protocol/pubsub#event'>"
                               "<items node='rssguard-test'><item id='item-42'/></items></event></message>");
      QTRY_COMPARE(received, 1);
      QCOMPARE(pushed.m_customId, QStringLiteral("item-42"));
      QCOMPARE(pushed.m_author, QStringLiteral("Fixture Author"));
      QVERIFY(pushed.m_contents.contains(QStringLiteral("Delivered over TLS")));
      QCOMPARE(fixture.itemRequests, 2);
      client.disconnectFromServer();
      QTRY_VERIFY(!client.isConnected());
    }

    void rejectsUntrustedTls() {
      Fixture fixture;
      QVERIFY(fixture.listener.listen(QHostAddress::LocalHost, 0));
      QXmppClient client;
      QSignalSpy errors(&client, &QXmppClient::errorOccurred);
      client.connectToServer(fixture.configuration(false));
      QTRY_VERIFY_WITH_TIMEOUT(!errors.isEmpty(), 10000);
      QVERIFY(!client.isConnected());
      client.disconnectFromServer();
    }
};

int main(int argc, char** argv) {
  QCoreApplication app(argc, argv);
  // Optional deployed-plugin check used by the Windows package validation script.
  const int index = app.arguments().indexOf(QStringLiteral("--plugin"));
  QStringList args = app.arguments();
  std::unique_ptr<QPluginLoader> loader;
  if (index >= 0) {
    if (index + 1 >= args.size()) return 2;
    loader = std::make_unique<QPluginLoader>(args.at(index + 1));
    if (!loader->instance()) {
      qCritical() << loader->errorString();
      return 2;
    }
    args.removeAt(index + 1);
    args.removeAt(index);
  }
  TestXmppPubSub tests;
  return QTest::qExec(&tests, args);
}

#include "test_xmpp_pubsub.moc"
