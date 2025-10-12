// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef MESSAGEPREVIEWER_H
#define MESSAGEPREVIEWER_H

#include "core/message.h"
#include "gui/tabcontent.h"
#include "services/abstract/label.h"
#include "services/abstract/rootitem.h"

#include <QAction>
#include <QPointer>
#include <QToolButton>
#include <QUrl>

class QGridLayout;
class QStackedLayout;
class QToolBar;

class WebBrowser;
class ItemDetails;

class LabelToolbarAction : public QAction {
    Q_OBJECT

  public:
    explicit LabelToolbarAction(QObject* parent = nullptr);

    Label* label() const;
    void setLabel(Label* label);

  private:
    QPointer<Label> m_label;
};

class MessagePreviewer : public TabContent {
    Q_OBJECT

  public:
    explicit MessagePreviewer(QWidget* parent = nullptr);
    virtual ~MessagePreviewer();

    void reloadFontSettings();

    virtual WebBrowser* webBrowser() const;

  public slots:
    void setToolbarsVisible(bool visible);
    void clear();

    void showItemDetails(RootItem* item);
    void loadMessage(const Message& message, RootItem* root);

  private slots:
    void switchLabel(bool assign);
    void showAllLabels();
    void markMessageAsRead();
    void markMessageAsUnread();
    void markMessageAsReadUnread(RootItem::ReadStatus read);
    void switchMessageImportance(bool checked);

  signals:
    void markMessageRead(int id, RootItem::ReadStatus read);
    void markMessageImportant(int id, RootItem::Importance important);
    void setMessageLabelIds(int id, const QStringList& ids);

  private:
    void createConnections();
    void updateButtons();
    void updateLabels(bool only_clear);

    void ensureItemDetailsVisible();
    void ensureDefaultBrowserVisible();

    QGridLayout* m_mainLayout;
    QStackedLayout* m_viewerLayout;
    QToolBar* m_toolBar;
    WebBrowser* m_msgBrowser;
    Message m_message;
    QPointer<RootItem> m_root;
    QAction* m_actionMarkRead;
    QAction* m_actionMarkUnread;
    QAction* m_actionSwitchImportance;
    QAction* m_actionShowAllLabels;
    QAction* m_separator;
    QList<QAction*> m_btnLabels;
    ItemDetails* m_itemDetails;
    bool m_toolbarVisible;

    static const int INDEX_DEFAULT = 0;
    static const int INDEX_ITEMS = 1;
    static const int INDEX_CUSTOM = 2;
};

#endif // MESSAGEPREVIEWER_H
