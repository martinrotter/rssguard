// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef QLITEHTMLVIEWER_H
#define QLITEHTMLVIEWER_H

#include "3rd-party/qlitehtml/src/qlitehtmlwidget.h"
#include "gui/webviewers/webviewer.h"

#include <QPointer>
#include <QTimer>

class QKeyEvent;
class SilentNetworkAccessManager;

class RSSGUARD_DLLSPEC QLiteHtmlViewer : public QLiteHtmlWidget, public WebViewer {
    Q_OBJECT
    Q_INTERFACES(WebViewer)

  public:
    explicit QLiteHtmlViewer(QWidget* parent = nullptr);
    virtual ~QLiteHtmlViewer();

    virtual void bindToBrowser(WebBrowser* browser);
    virtual void findText(const QString& text, bool backwards);

    virtual void setHtml(const QString& html, const QUrl& url = {});
    virtual QString html() const;
    virtual QUrl url() const;
    virtual void clear();

    virtual void loadMessage(const Message& message, RootItem* root);
    virtual QString htmlForMessage(const Message& message, RootItem* root) const;

    virtual double verticalScrollBarPosition() const;
    virtual void setVerticalScrollBarPosition(double pos);

    virtual void applyFont(const QFont& fon);
    virtual qreal zoomFactor() const;
    virtual void setZoomFactor(qreal zoom_factor);

  protected:
    virtual ContextMenuData provideContextMenuData(QContextMenuEvent* event) const;

  signals:
    void pageTitleChanged(const QString& new_title);
    void pageUrlChanged(const QUrl& url);
    void pageIconChanged(const QIcon&);
    void linkMouseHighlighted(const QUrl& url);
    void linkClicked(const QUrl& url);
    void loadingStarted();
    void loadingProgress(int progress);
    void loadingFinished(bool success);

  protected:
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void contextMenuEvent(QContextMenuEvent* event);

  private:
    QString prepareLegacyHtmlForMessage(const Message& message, RootItem* selected_item) const;

    QPointer<RootItem> m_root;
    QPointer<SilentNetworkAccessManager> m_network;
};

#endif // QLITEHTMLVIEWER_H
