// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef WEBVIEWER_H
#define WEBVIEWER_H

#include <QWebEngineView>

#include "core/message.h"
#include "network-web/webpage.h"

class RootItem;

class WebViewer : public QWebEngineView {
  Q_OBJECT

  public:
    explicit WebViewer(QWidget* parent = 0);

    bool canIncreaseZoom();
    bool canDecreaseZoom();

    inline QString messageContents() {
      return m_messageContents;
    }

    WebPage* page() const;
    RootItem* root() const;

  public slots:

    // Page zoom modifiers.
    bool increaseWebPageZoom();
    bool decreaseWebPageZoom();
    bool resetWebPageZoom();

    void displayMessage();
    void loadMessages(const QList<Message>& messages, RootItem* root);
    void clear();

  protected:
    void contextMenuEvent(QContextMenuEvent* event);
    QWebEngineView* createWindow(QWebEnginePage::WebWindowType type);

    void wheelEvent(QWheelEvent* event);

  signals:
    void messageStatusChangeRequested(int message_id, WebPage::MessageStatusChange change);

  private:
    RootItem* m_root;
    QString m_messageContents;
};

#endif // WEBVIEWER_H
