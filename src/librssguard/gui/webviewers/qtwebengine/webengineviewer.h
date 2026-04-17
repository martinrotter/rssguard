// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef WEBENGINEVIEWER_H
#define WEBENGINEVIEWER_H

#include "core/message.h"
#include "gui/webviewers/qtwebengine/webenginepage.h"
#include "gui/webviewers/webviewer.h"
#include "miscellaneous/externaltool.h"

#include <QPrinter>
#include <QWebEngineFullScreenRequest>
#include <QWebEngineSettings>
#include <QWebEngineView>

class RootItem;
class WebBrowser;

class RSSGUARD_DLLSPEC WebEngineViewer : public QWebEngineView, public WebViewer {
    Q_OBJECT
    Q_INTERFACES(WebViewer)

  public:
    explicit WebEngineViewer(QWidget* parent = nullptr);

  public:
    virtual void bindToBrowser(WebBrowser* browser);
    virtual void findText(const QString& text, bool backwards);

    virtual void reloadNetworkSettings();

    virtual void loadMessage(const Message& message, RootItem* root);
    virtual QString htmlForMessage(const Message& message, RootItem* root) const;

    virtual void loadUrl(const QUrl& url);
    virtual void setHtml(const QString& html, const QUrl& url = {}, RootItem* root = nullptr);
    virtual QString html() const;
    virtual QString plainText() const;
    virtual QUrl url() const;
    virtual void clear();
    virtual void reloadPage();
    virtual void goBack();
    virtual void goForward();

    virtual void cleanupCache();

    virtual void printToPrinter(QPrinter* printer);

    virtual void setLoadExternalResources(bool load_resources);

    virtual double verticalScrollBarPosition() const;
    virtual void setVerticalScrollBarPosition(double pos);

    virtual void processContextMenu(QMenu* specific_menu, QContextMenuEvent* event);

    virtual void applyFont(const QFont& fon);
    virtual qreal zoomFactor() const;
    virtual void setZoomFactor(qreal zoom_factor);

  signals:
    void pageTitleChanged(const QString& new_title);
    void pageUrlChanged(const QUrl& url);
    void pageIconChanged(const QIcon& icon);
    void linkMouseHighlighted(const QUrl& url);
    void linkMouseClicked(const QUrl& url);
    void loadingStarted();
    void loadingProgress(int progress);
    void loadingFinished(bool success);
    void openUrlInNewTab(const QUrl& url);
    void openViewerInNewTab(WebViewer* viewer);

  private slots:
    void printToPdf();
    void saveCompleteWebPage();

  protected:
    virtual ContextMenuData provideContextMenuData(QContextMenuEvent* event);

    virtual QWebEngineView* createWindow(QWebEnginePage::WebWindowType type);
    virtual void contextMenuEvent(QContextMenuEvent* event);
    virtual bool event(QEvent* event);

  private:
    QList<QAction*> advancedActions() const;
    WebEnginePage* page() const;

  private:
    WebBrowser* m_browser;
    QString m_html;
    QString m_plainText;
    QScopedPointer<QAction> m_actionPrintToPdf;
    QScopedPointer<QAction> m_actionSaveFullPage;
};

#endif // WEBENGINEVIEWER_H
