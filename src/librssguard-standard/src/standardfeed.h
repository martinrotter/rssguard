// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FEEDSMODELFEED_H
#define FEEDSMODELFEED_H

#include <librssguard/network-web/networkfactory.h>
#include <librssguard/services/abstract/feed.h>

#include <QCoreApplication>
#include <QDateTime>
#include <QMetaType>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QPair>

class StandardServiceRoot;

// Represents BASE class for feeds contained in FeedsModel.
// NOTE: This class should be derived to create PARTICULAR feed types.
class StandardFeed : public Feed {
    Q_OBJECT

    friend class StandardCategory;

  public:
    enum class SourceType {
      Url = 0,
      Script = 1,
      LocalFile = 2,
      EmbeddedBrowser = 3
    };

    enum class Type {
      Rss0X = 0,
      Rss2X = 1,
      Rdf = 2, // Sometimes denoted as RSS 1.0.
      Atom10 = 3,
      Json = 4,
      Sitemap = 5,
      iCalendar = 6
    };

    explicit StandardFeed(RootItem* parent_item = nullptr);
    explicit StandardFeed(const StandardFeed& other);

    virtual QList<QAction*> contextMenuFeedsList();
    virtual QString additionalTooltip() const;
    virtual bool canBeDeleted() const;
    virtual bool deleteItem();
    virtual QVariantHash customDatabaseData() const;
    virtual void setCustomDatabaseData(const QVariantHash& data);
    virtual Qt::ItemFlags additionalFlags() const;
    virtual bool performDragDropChange(RootItem* target_item);

    // Other getters/setters.
    Type type() const;
    void setType(Type type);

    SourceType sourceType() const;
    void setSourceType(SourceType source_type);

    QString encoding() const;
    void setEncoding(const QString& encoding);

    QString postProcessScript() const;
    void setPostProcessScript(const QString& post_process_script);

    NetworkFactory::NetworkAuthentication protection() const;
    void setProtection(NetworkFactory::NetworkAuthentication protect);

    QString username() const;
    void setUsername(const QString& username);

    QString password() const;
    void setPassword(const QString& password);

    NetworkFactory::Http2Status http2Status() const;
    void setHttp2Status(NetworkFactory::Http2Status status);

    // Tries to guess feed hidden under given URL
    // and uses given credentials.
    // Returns pointer to guessed feed (if at least partially
    // guessed) and retrieved error/status code from network layer
    // or nullptr feed.
    static QPair<StandardFeed*, NetworkResult> guessFeed(SourceType source_type,
                                                         const QString& url,
                                                         const QString& post_process_script,
                                                         StandardServiceRoot* account,
                                                         NetworkFactory::NetworkAuthentication protection,
                                                         bool fetch_icons = true,
                                                         const QString& username = {},
                                                         const QString& password = {},
                                                         const QList<QPair<QByteArray, QByteArray>>& http_headers = {},
                                                         const QNetworkProxy& custom_proxy =
                                                           QNetworkProxy::ProxyType::DefaultProxy,
                                                         NetworkFactory::Http2Status http2_status =
                                                           NetworkFactory::Http2Status::DontSet);

    // Converts particular feed type to string.
    static QString typeToString(Type type);
    static QString sourceTypeToString(SourceType type);

    // Scraping + post+processing.
    static QStringList prepareExecutionLine(const QString& execution_line);
    static QByteArray generateFeedFileWithScript(const QString& execution_line, int run_timeout);
    static QByteArray postProcessFeedFileWithScript(const QString& execution_line,
                                                    const QString& raw_feed_data,
                                                    int run_timeout);
    static QByteArray runScriptProcess(const QStringList& cmd_args,
                                       const QString& working_directory,
                                       int run_timeout,
                                       bool provide_input,
                                       const QString& input = {});

    static QList<QPair<QByteArray, QByteArray>> httpHeadersToList(const QVariantHash& headers);

    QString lastEtag() const;
    void setLastEtag(const QString& etag);

    QString dateTimeFormat() const;
    void setDateTimeFormat(const QString& dt_format);

    bool dontUseRawXmlSaving() const;
    void setDontUseRawXmlSaving(bool no_raw_xml_saving);

    // NOTE: Contains hash table where key is name of HTTP header.
    QVariantHash httpHeaders() const;
    void setHttpHeaders(const QVariantHash& http_headers);

  public slots:
    void fetchMetadataForItself();

  private:
    StandardServiceRoot* serviceRoot() const;
    bool removeItself();

  private:
    SourceType m_sourceType;
    Type m_type;
    QString m_postProcessScript;
    QString m_encoding;
    QString m_dateTimeFormat;
    NetworkFactory::NetworkAuthentication m_protection = NetworkFactory::NetworkAuthentication::NoAuthentication;
    QString m_username;
    QString m_password;
    QString m_lastEtag;
    bool m_dontUseRawXmlSaving;
    QVariantHash m_httpHeaders;
    NetworkFactory::Http2Status m_http2Status;
};

Q_DECLARE_METATYPE(StandardFeed::SourceType)
Q_DECLARE_METATYPE(StandardFeed::Type)

#endif // FEEDSMODELFEED_H
