// For license of this file, see <project-root-folder>/LICENSE.md.

#include <QCoreApplication>
#include <QDomElement>
#include <QXmppQt6/QXmppPubSubBaseItem.h>
#include <QXmppQt6/QXmppPubSubEventHandler.h>
#include <QXmppQt6/QXmppPubSubManager.h>

class AtomPubSubBaseItem : public QXmppPubSubBaseItem {
  private:
    QDomElement payload;

    QString title;
    QString content;
    QString aid;
    QString author;

  private:
    static void writeDomNode(QXmlStreamWriter* writer, const QDomNode& node);

    static void writeDomElement(QXmlStreamWriter* writer, const QDomElement& element);

  protected:
    virtual void parsePayload(const QDomElement& payloadElement);
    virtual void serializePayload(QXmlStreamWriter* writer) const;
};

class PubSubManager : public QXmppPubSubManager, public QXmppPubSubEventHandler {
    Q_OBJECT

  public:
    explicit PubSubManager(QObject* parent = nullptr);

  protected:
    virtual bool handlePubSubEvent(const QDomElement& element, const QString& pubSubService, const QString& nodeName);
};

/*
class PubSubExplorer : public QObject {
  public:
    PubSubExplorer(QXmppClient* client);

  public:
    void start();

  private:
    void checkService(const QString& jid);

    void discoverNodes(const QString& service);

    void fetchSubscriptions(const QString& service);

    void fetchItems(const QString& service, const QString& node, const QStringList& itemIds);

  private:
    QXmppClient* m_client;
    QXmppDiscoveryManager* m_disco;
    QXmppPubSubManager* m_pubsub;
};
*/

/*
int main(int argc, char* argv[]) {
  QCoreApplication app(argc, argv);

  QXmppClient client;

  client.addExtension(new QXmppDiscoveryManager);
  client.addExtension(new PubSubManager);

  client.logger()->setLoggingType(QXmppLogger::StdoutLogging);

  PubSubExplorer explorer(&client);

  QObject::connect(&client, &QXmppClient::connected, &explorer, &PubSubExplorer::start);

  client.connectToServer("skunkoss@movim.eu", "+Sz%P,3&&@fTY4y");

  return app.exec();
}
*/
