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
    void loadFilter();

    // Display filter title/contents.
    void showFilter(MessageFilter* filter);

    // Load feeds/categories of the account, place checkmarks where filter is used.
    void updateFeedAssignments(MessageFilter* filter, ServiceRoot* account);

  private:
    Ui::FormMessageFiltersManager m_ui;
    AccountCheckModel* m_feedsModel;
    RootItem* m_rootItem;
    QList<ServiceRoot*> m_accounts;
    FeedReader* m_reader;
};

#endif // FORMMESSAGEFILTERSMANAGER_H
