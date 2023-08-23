// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMEDITOWNCLOUDACCOUNT_H
#define FORMEDITOWNCLOUDACCOUNT_H

#include "services/abstract/gui/formaccountdetails.h"

class OwnCloudAccountDetails;
class OwnCloudServiceRoot;

class FormEditOwnCloudAccount : public FormAccountDetails {
  Q_OBJECT

  public:
    explicit FormEditOwnCloudAccount(QWidget* parent = nullptr);

  protected slots:
    virtual void apply();

  protected:
    virtual void loadAccountData();

  private slots:
    void performTest();

  private:
    OwnCloudAccountDetails* m_details;
};

#endif // FORMEDITOWNCLOUDACCOUNT_H
