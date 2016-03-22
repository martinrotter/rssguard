#ifndef WEBFACTORY_H
#define WEBFACTORY_H

#include <QObject>

#include "core/messagesmodel.h"

#include <QPointer>
#include <QMap>


class QWebEngineSettings;

class WebFactory : public QObject {
    Q_OBJECT

  public:
    // Destructor.
    virtual ~WebFactory();

    // Loads the web settings directly from
    // application settings and notifies the rest of
    // the world about current situation.
    void loadState();

    // Strips "<....>" (HTML, XML) tags from given text.
    QString stripTags(QString text);

    // HTML entity escaping.
    QString escapeHtml(const QString &html);
    QString deEscapeHtml(const QString &text);

    // BUG: Version for Qt < 4.8 has one issue, it will wrongly
    // count .co.uk (and others) as second-level domain
    QString toSecondLevelDomain(const QUrl &url);

    // Switchers.
    bool javascriptEnabled() const;
    bool pluginsEnabled() const;
    bool autoloadImages() const;

    // Singleton getter.
    static WebFactory *instance();

  public slots:
    // Opens given string URL in external browser.
    bool openUrlInExternalBrowser(const QString &url);
    bool sendMessageViaEmail(const Message &message);

    // Switchers.
    void switchJavascript(bool enable, bool save_settings = true);
    void switchPlugins(bool enable, bool save_settings = true);
    void switchImages(bool enable, bool save_settings = true);

  signals:
    void javascriptSwitched(bool enabled);
    void pluginsSwitched(bool enabled);
    void imagesLoadingSwitched(bool enabled);

  private:
    // Constructor.
    explicit WebFactory(QObject *parent = 0);

    // Escape sequences generators.
    void generetaEscapes();
    void generateDeescapes();

    QMap<QString, QString> m_escapes;
    QMap<QString, QString> m_deEscapes;
    QWebEngineSettings *m_globalSettings;

    // Singleton.
    static QPointer<WebFactory> s_instance;
};

#endif // WEBFACTORY_H
