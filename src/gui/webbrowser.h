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

    WebBrowser* webBrowser() const {
      return const_cast<WebBrowser*>(this);
    }

    WebViewer* viewer() const {
      return m_webView;
    }

    void reloadFontSettings();

  public slots:
    void increaseZoom();
    void decreaseZoom();
    void resetZoom();

    void clear();
    void loadUrl(const QString& url);
    void loadUrl(const QUrl& url);
    void loadMessages(const QList<Message>& messages, RootItem* root);
    void loadMessage(const Message& message, RootItem* root);

    // Switches visibility of navigation bar.
    inline void setNavigationBarVisible(bool visible) {
      m_toolBar->setVisible(visible);
    }

  protected:
    bool eventFilter(QObject* watched, QEvent* event);

  private slots:
    void updateUrl(const QUrl& url);

    void onLoadingStarted();
    void onLoadingProgress(int progress);
    void onLoadingFinished(bool success);

    void receiveMessageStatusChangeRequest(int message_id, WebPage::MessageStatusChange change);

    void onTitleChanged(const QString& new_title);
    void onIconChanged(const QIcon& icon);

  signals:
    void closeRequested();
    void iconChanged(int index, const QIcon& icon);
    void titleChanged(int index, const QString& title);

    void markMessageRead(int id, RootItem::ReadStatus read);
    void markMessageImportant(int id, RootItem::Importance important);
    void requestMessageListReload(bool mark_current_as_read);

  private:
    void initializeLayout();
    Message* findMessage(int id);

    void markMessageAsRead(int id, bool read);
    void switchMessageImportance(int id, bool checked);
    void createConnections();

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

    QList<Message> m_messages;
    QPointer<RootItem> m_root;
};

#endif // WEBBROWSER_H
