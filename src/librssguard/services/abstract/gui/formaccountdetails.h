// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMACCOUNTDETAILS_H
#define FORMACCOUNTDETAILS_H

#include "database/databasefactory.h"
#include "database/databasequeries.h"
#include "gui/reusable/networkproxydetails.h"
#include "miscellaneous/application.h"
#include "services/abstract/gui/accountdetails.h"

#include <QDialog>

namespace Ui {
  class FormAccountDetails;
}

class ServiceRoot;

class RSSGUARD_DLLSPEC FormAccountDetails : public QDialog {
    Q_OBJECT

  public:
    explicit FormAccountDetails(const QIcon& icon, QWidget* parent = nullptr);
    virtual ~FormAccountDetails();

    template <class T>
    T* addEditAccount(T* account_to_edit = nullptr);

    template <class T>
    T* account() const;

  protected slots:

    // Applies changes.
    // NOTE: This must be reimplemented in subclasses. Also this base
    // implementation must be called first.
    virtual void apply();

  protected:
    void activateTab(int index);
    void clearTabs();
    void insertCustomTab(QWidget* custom_tab, const QString& title, int index);

    // Sets the account which will be edited.
    // NOTE: This must be reimplemented in subclasses. Also this
    // base implementation must be called first.
    virtual void loadAccountData();

  private:
    void createConnections();

  protected:
    QScopedPointer<Ui::FormAccountDetails> m_ui;
    NetworkProxyDetails* m_proxyDetails;
    AccountDetails* m_accountDetails;
    ServiceRoot* m_account;
    bool m_creatingNew;
};

template <class T>
inline T* FormAccountDetails::addEditAccount(T* account_to_edit) {
  m_creatingNew = account_to_edit == nullptr;

  if (m_creatingNew) {
    m_account = new T();
  }
  else {
    m_account = account_to_edit;
  }

  // Load custom logic for account data loading.
  loadAccountData();

  if (exec() == QDialog::DialogCode::Accepted) {
    return account<T>();
  }
  else {
    return nullptr;
  }
}

template <class T>
inline T* FormAccountDetails::account() const {
  return qobject_cast<T*>(m_account);
}

#endif // FORMACCOUNTDETAILS_H
