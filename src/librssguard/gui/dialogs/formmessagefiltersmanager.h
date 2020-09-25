// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMMESSAGEFILTERSMANAGER_H
#define FORMMESSAGEFILTERSMANAGER_H

#include <QDialog>

#include "services/abstract/serviceroot.h"

#include "ui_formmessagefiltersmanager.h"

class AccountCheckSortedModel;
class MessageFilter;
class FeedReader;

class FormMessageFiltersManager : public QDialog {
  Q_OBJECT

  public:
    explicit FormMessageFiltersManager(FeedReader* reader, const QList<ServiceRoot*>& accounts, QWidget* parent = nullptr);
    virtual ~FormMessageFiltersManager();

    MessageFilter* selectedFilter() const;
    ServiceRoot* selectedAccount() const;

  private slots:
    void removeSelectedFilter();
    void addNewFilter();
    void saveSelectedFilter();
    void loadFilter();
    void loadFilters();
    void testFilter();

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
    void initializeTestingMessage();
    Message testingMessage() const;

  private:
    Ui::FormMessageFiltersManager m_ui;
    AccountCheckSortedModel* m_feedsModel;
    RootItem* m_rootItem;
    QList<ServiceRoot*> m_accounts;
    FeedReader* m_reader;
    bool m_loadingFilter;
};

#endif // FORMMESSAGEFILTERSMANAGER_H
