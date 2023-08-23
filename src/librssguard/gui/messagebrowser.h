// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef MESSAGEBROWSER_H
#define MESSAGEBROWSER_H

#include <QWidget>

#include "core/message.h"

#include "services/abstract/rootitem.h"

#include <QPointer>

class RootItem;
class QVBoxLayout;
class MessageTextBrowser;
class SearchTextWidget;

class MessageBrowser : public QWidget {
  Q_OBJECT

  public:
    explicit MessageBrowser(bool should_resize_to_fit, QWidget* parent = nullptr);

    double verticalScrollBarPosition() const;

  public slots:
    void setVerticalScrollBarPosition(double pos);
    void clear(bool also_hide);
    void reloadFontSettings();
    void loadUrl(const QString& url);
    void loadMessage(const Message& message, RootItem* root);

  protected:
    bool eventFilter(QObject* watched, QEvent* event);

  private slots:
    void onAnchorClicked(const QUrl& url);

  private:
    QString prepareHtmlForMessage(const Message& message);

  private:
    MessageTextBrowser* m_txtBrowser;
    SearchTextWidget* m_searchWidget;
    QVBoxLayout* m_layout;
    QStringList m_pictures;
    QPointer<RootItem> m_root;
};

#endif // MESSAGEBROWSER_H
