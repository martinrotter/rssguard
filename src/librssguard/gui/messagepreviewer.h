// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef MESSAGEPREVIEWER_H
#define MESSAGEPREVIEWER_H

#include <QToolButton>

#include "core/message.h"
#include "services/abstract/label.h"
#include "services/abstract/rootitem.h"

#include <QPointer>

class QGridLayout;
class QToolBar;

#if defined(USE_WEBENGINE)
class WebBrowser;
#else
class MessageBrowser;
#endif

class LabelButton : public QToolButton {
  Q_OBJECT

  public:
    explicit LabelButton(QWidget* parent = nullptr);

    Label* label() const;
    void setLabel(Label* label);

  private:
    QPointer<Label> m_label;
};

class MessagePreviewer : public QWidget {
  Q_OBJECT

  public:
    explicit MessagePreviewer(bool should_resize_to_fit, QWidget* parent = nullptr);

    void reloadFontSettings();

#if defined(USE_WEBENGINE)
    WebBrowser* webBrowser() const;
#endif

  public slots:
    void setToolbarsVisible(bool visible);
    void clear();
    void hideToolbar();
    void loadUrl(const QString& url);
    void loadMessage(const Message& message, RootItem* root);

  private slots:
    void switchLabel(bool assign);
    void markMessageAsRead();
    void markMessageAsUnread();
    void markMessageAsReadUnread(RootItem::ReadStatus read);
    void switchMessageImportance(bool checked);

  protected:

  signals:
    void markMessageRead(int id, RootItem::ReadStatus read);
    void markMessageImportant(int id, RootItem::Importance important);

  private:
    void createConnections();
    void updateButtons();
    void updateLabels(bool only_clear);

    QGridLayout* m_layout;
    QToolBar* m_toolBar;

#if defined(USE_WEBENGINE)
    WebBrowser* m_txtMessage;
#else
    MessageBrowser* m_txtMessage;
#endif

    Message m_message;
    QPointer<RootItem> m_root;
    QAction* m_actionMarkRead;
    QAction* m_actionMarkUnread;
    QAction* m_actionSwitchImportance;
    QAction* m_separator;
    QList<QPair<LabelButton*, QAction*>> m_btnLabels;
};

#endif // MESSAGEPREVIEWER_H
