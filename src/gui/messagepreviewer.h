// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef MESSAGEPREVIEWER_H
#define MESSAGEPREVIEWER_H

#include <QWidget>

#include "ui_messagepreviewer.h"

#include "core/message.h"
#include "services/abstract/rootitem.h"

#include <QPointer>

namespace Ui {
  class MessagePreviewer;
}

class QToolBar;

class MessagePreviewer : public QWidget {
  Q_OBJECT

  public:
    explicit MessagePreviewer(QWidget* parent = nullptr);

    void reloadFontSettings();

  public slots:
    void clear();
    void hideToolbar();
    void loadMessage(const Message& message, RootItem* root);

  private slots:
    void markMessageAsRead();
    void markMessageAsUnread();
    void markMessageAsReadUnread(RootItem::ReadStatus read);
    void switchMessageImportance(bool checked);

  protected:
    bool eventFilter(QObject* watched, QEvent* event);

  signals:
    void markMessageRead(int id, RootItem::ReadStatus read);
    void markMessageImportant(int id, RootItem::Importance important);
    void requestMessageListReload(bool mark_current_as_read);

  private:
    void createConnections();
    void updateButtons();
    QString prepareHtmlForMessage(const Message& message);

    QToolBar* m_toolBar;

    Ui::MessagePreviewer m_ui;
    Message m_message;
    QStringList m_pictures;

    QPointer<RootItem> m_root;

    QAction* m_actionMarkRead;
    QAction* m_actionMarkUnread;
    QAction* m_actionSwitchImportance;
};

#endif // MESSAGEPREVIEWER_H
