#ifndef WEBFACTORY_H
#define WEBFACTORY_H

#include <QObject>
#include <QPointer>
#include <QMap>


class WebFactory : public QObject {
    Q_OBJECT

  public:
    // Destructor.
    virtual ~WebFactory();

    // Strips "<....>" (HTML, XML) tags from given text.
    QString stripTags(QString text);

    // HTML entity escaping.
    // TODO: Optimize these methods.
    QString escapeHtml(const QString &html);
    QString deEscapeHtml(const QString &text);

    // Singleton getter.
    static WebFactory *instance();

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

    // Singleton.
    static QPointer<WebFactory> s_instance;
};

#endif // WEBFACTORY_H
