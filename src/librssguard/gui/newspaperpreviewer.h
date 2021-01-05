// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef NEWSPAPERPREVIEWER_H
#define NEWSPAPERPREVIEWER_H

#include <QWidget>

#include "gui/tabcontent.h"

#include "ui_newspaperpreviewer.h"

#include "core/message.h"
#include "services/abstract/rootitem.h"

#include <QPointer>

namespace Ui {
  class NewspaperPreviewer;
}

class RootItem;

#if defined(USE_WEBENGINE)
class WebBrowser;
#endif

class NewspaperPreviewer : public TabContent {
  Q_OBJECT

  public:
    explicit NewspaperPreviewer(int msg_height, RootItem* root, QList<Message> messages, QWidget* parent = nullptr);

#if defined(USE_WEBENGINE)
    virtual WebBrowser* webBrowser() const;
#endif

  public slots:
    void showMoreMessages();

  signals:
    void markMessageRead(int id, RootItem::ReadStatus read);
    void markMessageImportant(int id, RootItem::Importance important);

  private:
    int m_msgHeight;
    QScopedPointer<Ui::NewspaperPreviewer> m_ui;
    QPointer<RootItem> m_root;
    QList<Message> m_messages;
};

#endif // NEWSPAPERPREVIEWER_H
