// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef WEBBROWSER_H
#define WEBBROWSER_H

#include "gui/tabcontent.h"

#include "core/message.h"
#include "network-web/webpage.h"
#include "services/abstract/rootitem.h"

#include <QPointer>
#include <QToolBar>

class QToolButton;
class QVBoxLayout;
class QHBoxLayout;
class QProgressBar;
class QMenu;
class QLabel;
class TabWidget;
class WebViewer;
class LocationLineEdit;
class DiscoverFeedsButton;
class SearchTextWidget;

class WebBrowser : public TabContent {
  Q_OBJECT

  public:
    explicit WebBrowser(QWidget* parent = nullptr);
    virtual ~WebBrowser();

    virtual WebBrowser* webBrowser() const;

    WebViewer* viewer() const;

    double verticalScrollBarPosition() const;
    void setVerticalScrollBarPosition(double pos);

  public slots:
    void reloadFontSettings();
    void increaseZoom();
    void decreaseZoom();
    void resetZoom();

    void clear(bool also_hide);
    void loadUrl(const QString& url);
    void loadUrl(const QUrl& url);
    void loadMessages(const QList<Message>& messages, RootItem* root);
    void loadMessage(const Message& message, RootItem* root);
    void setNavigationBarVisible(bool visible);

  protected:
    virtual bool eventFilter(QObject* watched, QEvent* event);

  private slots:
    void readabilePage();
    void openCurrentSiteInSystemBrowser();
    void updateUrl(const QUrl& url);
    void onLoadingStarted();
    void onLoadingProgress(int progress);
    void onLoadingFinished(bool success);
    void onTitleChanged(const QString& new_title);
    void onIconChanged(const QIcon& icon);
    void setReadabledHtml(const QString& better_html);
    void readabilityFailed(const QString& error);

  signals:
    void closeRequested();
    void iconChanged(int index, const QIcon& icon);
    void titleChanged(int index, const QString& title);

  private:
    void initializeLayout();
    void createConnections();

    Message* findMessage(int id);

  private:
    QVBoxLayout* m_layout;
    QToolBar* m_toolBar;
    WebViewer* m_webView;
    SearchTextWidget* m_searchWidget;
    LocationLineEdit* m_txtLocation;
    DiscoverFeedsButton* m_btnDiscoverFeeds;
    QProgressBar* m_loadingProgress;
    QAction* m_actionBack;
    QAction* m_actionForward;
    QAction* m_actionReload;
    QAction* m_actionStop;
    QAction* m_actionOpenInSystemBrowser;
    QAction* m_actionReadabilePage;
    QList<Message> m_messages;
    QPointer<RootItem> m_root;
};

inline WebBrowser* WebBrowser::webBrowser() const {
  return const_cast<WebBrowser*>(this);
}

inline WebViewer* WebBrowser::viewer() const {
  return m_webView;
}

inline void WebBrowser::setNavigationBarVisible(bool visible) {
  m_toolBar->setVisible(visible);
}

#endif // WEBBROWSER_H
