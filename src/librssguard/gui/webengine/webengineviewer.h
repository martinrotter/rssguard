// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef WEBENGINEVIEWER_H
#define WEBENGINEVIEWER_H

#include <QWebEngineView>

#include "gui/webviewer.h"

#include "core/message.h"
#include "miscellaneous/externaltool.h"
#include "network-web/webengine/webenginepage.h"

class RootItem;

class WebEngineViewer : public QWebEngineView, public WebViewer {
  Q_OBJECT

  public:
    explicit WebEngineViewer(QWidget* parent = nullptr);

    RootItem* root() const;

  public:
    virtual void loadMessages(const QList<Message>& messages, RootItem* root);
    virtual void bindToBrowser(WebBrowser* browser);
    virtual void findText(const QString& text, bool backwards);
    virtual void setUrl(const QUrl& url);
    virtual void setHtml(const QString& html, const QUrl& base_url = {});
    virtual void clear();
    virtual double verticalScrollBarPosition() const;
    virtual void setVerticalScrollBarPosition(double pos);
    virtual void reloadFontSettings(const QFont& fon);
    virtual bool canZoomIn() const;
    virtual bool canZoomOut() const;
    virtual qreal zoomFactor() const;
    virtual void zoomIn();
    virtual void zoomOut();
    virtual void setZoomFactor(qreal zoom_factor);
    virtual QString html() const;
    virtual QUrl url() const;

  signals:
    void zoomFactorChanged();

  protected:
    virtual QWebEngineView* createWindow(QWebEnginePage::WebWindowType type);
    virtual void contextMenuEvent(QContextMenuEvent* event);
    virtual void wheelEvent(QWheelEvent* event);
    virtual bool event(QEvent* event);
    virtual bool eventFilter(QObject* object, QEvent* event);

  private slots:
    void displayMessage();
    void openUrlWithExternalTool(ExternalTool tool, const QString& target_url);

  private:
    WebEnginePage* page() const;

  private:
    RootItem* m_root;
    QString m_messageBaseUrl;
    QString m_messageContents;
};

#endif // WEBENGINEVIEWER_H
