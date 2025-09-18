// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMMESSAGEFILTERSMANAGER_H
#define FORMMESSAGEFILTERSMANAGER_H

#include "services/abstract/serviceroot.h"

#include "ui_formmessagefiltersmanager.h"

#include <QDialog>

class QSortFilterProxyModel;
class AccountCheckSortedModel;
class MessageFilter;
class FeedReader;
class MessagesForFiltersModel;
class JsSyntaxHighlighter;

class FormMessageFiltersManager : public QDialog {
    Q_OBJECT

  public:
    explicit FormMessageFiltersManager(FeedReader* reader,
                                       const QList<ServiceRoot*>& accounts,
                                       QWidget* parent = nullptr);
    virtual ~FormMessageFiltersManager();

    MessageFilter* selectedFilter() const;
    ServiceRoot* selectedAccount() const;

  protected:
    virtual bool eventFilter(QObject* watched, QEvent* event);

  private slots:
    void openDocs();
    void displaySelectedMessageDetails(const QModelIndex& current, const QModelIndex& previous);
    void filterMessagesLikeThis(const Message& msg);
    void showMessageContextMenu(QPoint pos);
    void removeSelectedFilter();
    void addNewFilter(const QString& filter_script = QString());
    void saveSelectedFilter();
    void loadFilter();
    void loadFilters();
    void testFilter();
    void displayMessagesOfFeed();
    void processCheckedFeeds();

    // Load feeds/categories tree.
    void loadAccount(ServiceRoot* account);

    // Load checkmarks according to already active assignments.
    void loadFilterFeedAssignments(MessageFilter* filter, ServiceRoot* account);

    void onAccountChanged();
    void onFeedChecked(RootItem* item, Qt::CheckState state);

    // Display filter title/contents.
    void showFilter(MessageFilter* filter);

  private:
    void loadAccounts();
    void beautifyScript();

    RootItem* selectedCategoryFeed() const;

  private:
    Ui::FormMessageFiltersManager m_ui;
    AccountCheckSortedModel* m_feedsModel;
    RootItem* m_rootItem;
    QList<ServiceRoot*> m_accounts;
    FeedReader* m_reader;
    bool m_loadingFilter;
    QSortFilterProxyModel* m_msgProxyModel;
    MessagesForFiltersModel* m_msgModel;
    JsSyntaxHighlighter* m_highlighter;
    QColor m_defaultTextColor;
};

#endif // FORMMESSAGEFILTERSMANAGER_H
