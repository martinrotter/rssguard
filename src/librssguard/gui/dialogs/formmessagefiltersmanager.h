// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMMESSAGEFILTERSMANAGER_H
#define FORMMESSAGEFILTERSMANAGER_H

#include <QDialog>

#include "services/abstract/serviceroot.h"

#include "ui_formmessagefiltersmanager.h"

class AccountCheckModel;
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
    void addNewFilter();
    void saveSelectedFilter();
    void loadFilter();
    void testFilter();

    // Load feeds/categories tree.
    void loadSelectedAccount();

    // Load checkmarks according to already active assignments.
    void loadFilterFeedAssignments();

    // Display filter title/contents.
    void showFilter(MessageFilter* filter);

  private:
    void loadAccounts();
    void beautifyScript();
    void initializeTestingMessage();
    Message testingMessage() const;

  private:
    Ui::FormMessageFiltersManager m_ui;
    AccountCheckModel* m_feedsModel;
    RootItem* m_rootItem;
    QList<ServiceRoot*> m_accounts;
    FeedReader* m_reader;
    bool m_loadingFilter;
};

#endif // FORMMESSAGEFILTERSMANAGER_H
