// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef WEBVIEWER_H
#define WEBVIEWER_H

#include <QWebEngineView>

#include "core/message.h"
#include "miscellaneous/externaltool.h"
#include "network-web/webpage.h"

class RootItem;

class WebViewer : public QWebEngineView {
  Q_OBJECT

  public:
    explicit WebViewer(QWidget* parent = nullptr);

    bool canIncreaseZoom();
    bool canDecreaseZoom();

    QString messageContents();
    WebPage* page() const;
    RootItem* root() const;

  public slots:
    bool increaseWebPageZoom();
    bool decreaseWebPageZoom();
    bool resetWebPageZoom(bool to_factory_default = false);

    void displayMessage();
    void loadMessages(const QList<Message>& messages, RootItem* root);
    void clear();

  protected:
    virtual QWebEngineView* createWindow(QWebEnginePage::WebWindowType type);
    virtual void contextMenuEvent(QContextMenuEvent* event);
    virtual void wheelEvent(QWheelEvent* event);
    virtual bool event(QEvent* event);
    virtual bool eventFilter(QObject* object, QEvent* event);

  private slots:
    void onLinkHovered(const QString& url);
    void openUrlWithExternalTool(ExternalTool tool, const QString& target_url);

  private:
    RootItem* m_root;
    QString m_messageBaseUrl;
    QString m_messageContents;
};

inline QString WebViewer::messageContents() {
  return m_messageContents;
}

#endif // WEBVIEWER_H
