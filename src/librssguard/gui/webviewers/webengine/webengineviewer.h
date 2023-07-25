// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef WEBENGINEVIEWER_H
#define WEBENGINEVIEWER_H

#include <QWebEngineView>

#include "gui/webviewers/webviewer.h"

#include "core/message.h"
#include "miscellaneous/externaltool.h"
#include "network-web/webengine/webenginepage.h"

class RootItem;
class WebBrowser;

class WebEngineViewer : public QWebEngineView, public WebViewer {
    Q_OBJECT
    Q_INTERFACES(WebViewer)

  public:
    explicit WebEngineViewer(QWidget* parent = nullptr);

    RootItem* root() const;

  public:
    virtual void loadMessages(const QList<Message>& messages, RootItem* root);
    virtual void bindToBrowser(WebBrowser* browser);
    virtual void findText(const QString& text, bool backwards);
    virtual void setUrl(const QUrl& url);
    virtual void setHtml(const QString& html, const QUrl& base_url = {});
    virtual void setReadabledHtml(const QString& html, const QUrl& base_url = {});
    virtual void clear();
    virtual double verticalScrollBarPosition() const;
    virtual void setVerticalScrollBarPosition(double pos);
    virtual void applyFont(const QFont& fon);
    virtual qreal zoomFactor() const;
    virtual void setZoomFactor(qreal zoom_factor);
    virtual QString html() const;
    virtual QUrl url() const;

  signals:
    void pageTitleChanged(const QString& new_title);
    void pageUrlChanged(const QUrl& url);
    void pageIconChanged(const QIcon&);
    void linkMouseHighlighted(const QUrl& url);
    void loadingStarted();
    void loadingProgress(int progress);
    void loadingFinished(bool success);
    void newWindowRequested(WebViewer* viewer);
    void closeWindowRequested();

  protected:
    virtual QWebEngineView* createWindow(QWebEnginePage::WebWindowType type);
    virtual void contextMenuEvent(QContextMenuEvent* event);
    virtual bool event(QEvent* event);

  private slots:
    void openUrlWithExternalTool(ExternalTool tool, const QString& target_url);

  private:
    WebEnginePage* page() const;

  private:
    WebBrowser* m_browser;
    RootItem* m_root;
    QUrl m_messageBaseUrl;
    QString m_messageContents;
};

#endif // WEBENGINEVIEWER_H
