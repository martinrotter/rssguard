// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef WEBENGINEVIEWER_H
#define WEBENGINEVIEWER_H

#include "core/message.h"
#include "gui/webviewers/qtwebengine/webenginepage.h"
#include "gui/webviewers/webviewer.h"

#include <QPrinter>
#include <QWebEngineFullScreenRequest>
#include <QWebEngineSettings>
#include <QWebEngineView>

class RootItem;
class WebBrowser;

class ActionWatcher : public QObject {
    Q_OBJECT
  public:
    explicit ActionWatcher(QObject* parent = nullptr) : QObject(parent), m_action(nullptr), m_last(false) {}

    QAction* action() const {
      return m_action;
    }

    void setAction(QAction* action) {
      if (m_action == action) {
        return;
      }

      if (m_action) {
        disconnect(m_action, &QAction::changed, this, &ActionWatcher::onChanged);
      }

      m_action = action;

      if (m_action) {
        m_last = m_action->isEnabled();
        connect(m_action, &QAction::changed, this, &ActionWatcher::onChanged);
      }
    }

  signals:
    void enabledChanged(bool enabled);

  private slots:
    void onChanged() {
      if (!m_action) {
        return;
      }

      bool now = m_action->isEnabled();

      if (now != m_last) {
        m_last = now;
        emit enabledChanged(now);
      }
    }

  private:
    QAction* m_action;
    bool m_last;
};

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

    virtual bool supportsNavigation() const;
    virtual bool supportImagesLoading() const;

    virtual double verticalScrollBarPosition() const;
    virtual void setVerticalScrollBarPosition(double pos);

    virtual void processContextMenu(QMenu* specific_menu, QContextMenuEvent* event);

    virtual void applyFont(const QFont& fon);
    virtual qreal zoomFactor() const;
    virtual void setZoomFactor(qreal zoom_factor);

  signals:
    void reloadPageEnabledChanged(bool can_go_forward);
    void goBackEnabledChanged(bool can_go_back);
    void goForwardEnabledChanged(bool can_go_forward);
    void pageTitleChanged(const QString& new_title);
    void pageUrlChanged(const QUrl& url);
    void pageIconChanged(const QIcon& icon);
    void linkMouseHighlighted(const QUrl& url);
    void linkMouseClicked(const QUrl& url, LinkNavigationHints hints = LinkNavigationHints::None);
    void loadingStarted();
    void loadingProgress(int progress);
    void loadingFinished(bool success);
    void openUrlInNewTab(bool open_externally, const QUrl& url);
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
    QList<QAction*> diagActions() const;
    WebEnginePage* page() const;

  private:
    WebBrowser* m_browser;
    QString m_html;
    QString m_plainText;
    QScopedPointer<QAction> m_actionPrintToPdf;
    QScopedPointer<QAction> m_actionSaveFullPage;
    QScopedPointer<QAction> m_actionDiagGpu;

    ActionWatcher m_actionWatcherGoBack;
    ActionWatcher m_actionWatcherGoForward;
    ActionWatcher m_actionWatcherReloadPage;
};

#endif // WEBENGINEVIEWER_H
