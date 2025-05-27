// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef WEBBROWSER_H
#define WEBBROWSER_H

#include "core/message.h"
#include "gui/tabcontent.h"
#include "services/abstract/rootitem.h"

#include <QPointer>
#include <QToolBar>
#include <QUrl>

class QToolButton;
class QVBoxLayout;
class QHBoxLayout;
class QProgressBar;
class QWidgetAction;
class QMenu;
class QLabel;

class TabWidget;
class WebViewer;
class SearchTextWidget;

class RSSGUARD_DLLSPEC WebBrowser : public TabContent {
    Q_OBJECT

    friend class TextBrowserViewer;

  public:
    explicit WebBrowser(WebViewer* viewer = nullptr, QWidget* parent = nullptr);
    virtual ~WebBrowser();

    virtual WebBrowser* webBrowser() const;

    WebViewer* viewer() const;

    void reloadFontSettings();

    double verticalScrollBarPosition() const;
    void setVerticalScrollBarPosition(double pos);

    void scrollUp();
    void scrollDown();

  public slots:
    void clear(bool also_hide);
    void setHtml(const QString& html, const QUrl& base_url = {});
    void loadMessages(const QList<Message>& messages, RootItem* root);
    void setToolBarVisible(bool visible);

  protected:
    virtual bool eventFilter(QObject* watched, QEvent* event);

  private slots:
    void onZoomFactorChanged();

#if defined(ENABLE_MEDIAPLAYER)
    void playCurrentSiteInMediaPlayer();
#endif

    void openCurrentSiteInSystemBrowser();

    void onLoadingStarted();
    void onLoadingProgress(int progress);
    void onLoadingFinished(bool success);

    void onTitleChanged(const QString& new_title);
    void onIconChanged(const QIcon& icon);

    void onLinkHovered(const QUrl& url);
    void onLinkClicked(const QUrl& url);

  signals:
    void iconChanged(int index, const QIcon& icon);
    void titleChanged(int index, const QString& title);

  private:
    void bindWebView();
    void initializeLayout();
    void createConnections();

  private:
    QVBoxLayout* m_layout;
    QToolBar* m_toolBar;
    WebViewer* m_webView;
    SearchTextWidget* m_searchWidget;
    QProgressBar* m_loadingProgress;
    QAction* m_actionOpenInSystemBrowser;

#if defined(ENABLE_MEDIAPLAYER)
    QAction* m_actionPlayPageInMediaPlayer;
#endif

    QList<Message> m_messages;
    QPointer<RootItem> m_root;
};

inline WebBrowser* WebBrowser::webBrowser() const {
  return const_cast<WebBrowser*>(this);
}

inline WebViewer* WebBrowser::viewer() const {
  return m_webView;
}

inline void WebBrowser::setToolBarVisible(bool visible) {
  m_toolBar->setVisible(visible);
}

#endif // WEBBROWSER_H
