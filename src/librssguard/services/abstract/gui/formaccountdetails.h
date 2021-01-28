// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMACCOUNTDETAILS_H
#define FORMACCOUNTDETAILS_H

#include <QDialog>

#include "ui_formaccountdetails.h"

#include "gui/networkproxydetails.h"
#include "miscellaneous/application.h"
#include "miscellaneous/databasefactory.h"
#include "miscellaneous/databasequeries.h"

class ServiceRoot;

class FormAccountDetails : public QDialog {
  Q_OBJECT

  public:
    explicit FormAccountDetails(const QIcon& icon, QWidget* parent = nullptr);

    template<class T>
    T* addEditAccount(T* account_to_edit = nullptr);

    template<class T>
    T* account() const;

  protected slots:

    // Applies changes.
    // NOTE: This must be reimplemented in subclasses. Also, every
    // subclass must call applyInternal() method as first statement.
    virtual void apply() = 0;

  protected:

    // Performs some common operations and returns true if creating
    // new account or false if editing existing account.
    template<class T>
    bool applyInternal();

    void activateTab(int index);
    void clearTabs();
    void insertCustomTab(QWidget* custom_tab, const QString& title, int index);

    // Sets the account which will be edited.
    // NOTE: This must be reimplemented in subclasses. Also this
    // base implementation must be called first.
    virtual void setEditableAccount(ServiceRoot* editable_account);

  private:
    void createConnections();

  protected:
    Ui::FormAccountDetails m_ui;
    NetworkProxyDetails* m_proxyDetails;
    ServiceRoot* m_account;
};

template<class T>
inline bool FormAccountDetails::applyInternal() {
  if (m_account != nullptr) {
    // Perform last-time operations before account is changed.
    auto* cached_account = dynamic_cast<CacheForServiceRoot*>(m_account);

    if (cached_account != nullptr) {
      qWarningNN << LOGSEC_CORE << "Last-time account cache saving before account gets changed.";
      cached_account->saveAllCachedData(true);
    }
  }

  QSqlDatabase database = qApp->database()->connection(QSL("FormAccountDetails"));
  bool creating = m_account == nullptr;

  if (creating) {
    m_account = new T();
    m_account->setAccountId(DatabaseQueries::createBaseAccount(database, m_account->code()));
  }

  m_account->setNetworkProxy(m_proxyDetails->proxy());

  // NOTE: We edit account common attributes here directly.
  DatabaseQueries::editBaseAccount(database, m_account);
  return creating;
}

template<class T>
inline T* FormAccountDetails::addEditAccount(T* account_to_edit) {
  if (account_to_edit == nullptr) {
    setWindowTitle(tr("Add new account"));
  }
  else {
    setEditableAccount(static_cast<ServiceRoot*>(account_to_edit));
  }

  if (exec() == QDialog::DialogCode::Accepted) {
    return account<T>();
  }
  else {
    return nullptr;
  }
}

template<class T>
inline T* FormAccountDetails::account() const {
  return qobject_cast<T*>(m_account);
}

#endif // FORMACCOUNTDETAILS_H
