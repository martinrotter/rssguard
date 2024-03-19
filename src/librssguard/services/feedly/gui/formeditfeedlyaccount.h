// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMEDITFEEDLYACCOUNT_H
#define FORMEDITFEEDLYACCOUNT_H

#include "services/abstract/gui/formaccountdetails.h"

class FeedlyAccountDetails;
class FeedlyServiceRoot;

class FormEditFeedlyAccount : public FormAccountDetails {
    Q_OBJECT

  public:
    explicit FormEditFeedlyAccount(QWidget* parent = nullptr);

  protected slots:
    virtual void apply();

  protected:
    virtual void loadAccountData();

  private slots:
    void performTest();

  private:
    FeedlyAccountDetails* m_details;
};

#endif // FORMEDITFEEDLYACCOUNT_H
