// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef WEBPAGE_H
#define WEBPAGE_H

#include <QWebEnginePage>

class WebViewer;

class WebPage : public QWebEnginePage {
  Q_OBJECT

  public:
    enum MessageStatusChange {
      MarkRead,
      MarkUnread,
      MarkStarred,
      MarkUnstarred
    };

    explicit WebPage(QObject* parent = 0);

    WebViewer* view() const;

  protected:
    void javaScriptAlert(const QUrl& securityOrigin, const QString& msg);
    bool acceptNavigationRequest(const QUrl& url, NavigationType type, bool isMainFrame);

  signals:
    void messageStatusChangeRequested(int message_id, WebPage::MessageStatusChange change);
};

#endif // WEBPAGE_H
