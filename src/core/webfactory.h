#ifndef WEBFACTORY_H
#define WEBFACTORY_H

#include <QObject>
#include <QPointer>
#include <QMap>


class QWebSettings;

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
    // TODO: Optimize these methods.
    QString escapeHtml(const QString &html);
    QString deEscapeHtml(const QString &text);

    // Switchers.
    bool javascriptEnabled() const;
    bool pluginsEnabled() const;
    bool autoloadImages() const;

    // Singleton getter.
    static WebFactory *instance();

  public slots:
    // Opens given string URL in external browser.
    bool openUrlInExternalBrowser(const QString &url);

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
    QMap<QString, QString> generetaEscapes();
    QMap<QString, QString> generateDeescapes();

    QWebSettings *m_globalSettings;

    // Singleton.
    static QPointer<WebFactory> s_instance;
};

#endif // WEBFACTORY_H
