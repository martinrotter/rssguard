// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef QLITEHTMLARTICLEVIEWER_H
#define QLITEHTMLARTICLEVIEWER_H

#include "gui/webviewers/qlitehtml/qlitehtmlwidget.h"
#include "gui/webviewers/webviewer.h"

#include <QPointer>
#include <QTimer>

class QKeyEvent;
class SilentNetworkAccessManager;

class RSSGUARD_DLLSPEC QLiteHtmlArticleViewer : public QLiteHtmlWidget, public WebViewer {
    Q_OBJECT
    Q_INTERFACES(WebViewer)

  public:
    explicit QLiteHtmlArticleViewer(QWidget* parent = nullptr);
    virtual ~QLiteHtmlArticleViewer();

    virtual void bindToBrowser(WebBrowser* browser);
    virtual void findText(const QString& text, bool backwards);

    virtual void reloadNetworkSettings();

    virtual void setHtml(const QString& html, const QUrl& url = {}, RootItem* root = nullptr);
    virtual QString html() const;
    virtual QUrl url() const;
    virtual void clear();

    virtual void loadMessage(const Message& message, RootItem* root);
    virtual QString htmlForMessage(const Message& message, RootItem* root) const;

    virtual bool loadExternalResources() const;
    virtual void setLoadExternalResources(bool load_resources);

    virtual double verticalScrollBarPosition() const;
    virtual void setVerticalScrollBarPosition(double pos);

    virtual void applyFont(const QFont& fon);
    virtual qreal zoomFactor() const;
    virtual void setZoomFactor(qreal zoom_factor);

  protected:
    virtual ContextMenuData provideContextMenuData(QContextMenuEvent* event);

  signals:
    void pageTitleChanged(const QString& new_title);
    void pageUrlChanged(const QUrl& url);
    void pageIconChanged(const QIcon&);
    void linkMouseHighlighted(const QUrl& url);
    void linkMouseClicked(const QUrl& url);
    void loadingStarted();
    void loadingProgress(int progress);
    void loadingFinished(bool success);

  protected:
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void contextMenuEvent(QContextMenuEvent* event);

  private:
    QPointer<RootItem> m_root;
};

#endif // QLITEHTMLARTICLEVIEWER_H
