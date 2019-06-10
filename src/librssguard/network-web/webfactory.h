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
#endif

class WebFactory : public QObject {
  Q_OBJECT

  public:
    explicit WebFactory(QObject* parent = nullptr);
    virtual ~WebFactory();

    // Strips "<....>" (HTML, XML) tags from given text.
    QString stripTags(QString text);

    // HTML entity escaping.
    QString escapeHtml(const QString& html);
    QString deEscapeHtml(const QString& text);

    QString toSecondLevelDomain(const QUrl& url);

#if defined (USE_WEBENGINE)
    QAction* engineSettingsAction();
#endif

  public slots:
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
    void generateEscapes();
    void generateDeescapes();

    QMap<QString, QString> m_escapes;
    QMap<QString, QString> m_deEscapes;

#if defined (USE_WEBENGINE)
    QAction* m_engineSettings;
#endif
};

#endif // WEBFACTORY_H
