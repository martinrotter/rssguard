// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef WEBFACTORY_H
#define WEBFACTORY_H

#include <QObject>

#include "core/messagesmodel.h"

#include <QMap>

#if defined (USE_WEBENGINE)
#include <QWebEngineSettings>
#endif

#if defined (USE_WEBENGINE)
class QMenu;
class AdBlockManager;
class NetworkUrlInterceptor;
#endif

class WebFactory : public QObject {
  Q_OBJECT

  public:
    explicit WebFactory(QObject* parent = nullptr);
    virtual ~WebFactory();

    // Strips "<....>" (HTML, XML) tags from given text.
    QString stripTags(QString text);

    // HTML entity unescaping. This method
    // converts both HTML entity names and numbers to UTF-8 string.
    // Example of entities are:
    //   ∀ = &forall; (entity name), &#8704; (base-10 entity), &#x2200; (base-16 entity)
    QString unescapeHtml(const QString& html);

#if defined (USE_WEBENGINE)
    QAction* engineSettingsAction();
    AdBlockManager* adBlock();
    NetworkUrlInterceptor* urlIinterceptor();
#endif

  public slots:
    void updateProxy();
    bool openUrlInExternalBrowser(const QString& url) const;
    bool sendMessageViaEmail(const Message& message);

#if defined (USE_WEBENGINE)
  private slots:
    void createMenu(QMenu* menu = nullptr);
    void webEngineSettingChanged(bool enabled);

  private:
    QAction* createEngineSettingsAction(const QString& title, QWebEngineSettings::WebAttribute attribute);
#endif

  private:
    void generateUnescapes();

  private:
#if defined(USE_WEBENGINE)
    AdBlockManager * m_adBlock;
    NetworkUrlInterceptor* m_urlInterceptor;
    QAction* m_engineSettings;
#endif

    QMap<QString, char16_t> m_htmlNamedEntities;
};

#endif // WEBFACTORY_H
