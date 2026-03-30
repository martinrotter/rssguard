// For license of this file, see <project-root-folder>/LICENSE.md.

#include <librssguard/core/message.h>

#include <QCoreApplication>
#include <QDomElement>
#include <QXmppPubSubBaseItem.h>
#include <QXmppPubSubEventHandler.h>
#include <QXmppPubSubManager.h>

class AtomPubSubBaseItem : public QXmppPubSubBaseItem {
  public:
    Message message() const;

  private:
    Message m_message;

  protected:
    virtual void parsePayload(const QDomElement& element);
    virtual void serializePayload(QXmlStreamWriter* writer) const;
};

class PubSubManager : public QXmppPubSubManager, public QXmppPubSubEventHandler {
    Q_OBJECT

  public:
    explicit PubSubManager(QObject* parent = nullptr);

  signals:
    void pushArticleObtained(QString service, QString node, Message message);

  protected:
    virtual bool handlePubSubEvent(const QDomElement& element, const QString& service, const QString& node);
};
