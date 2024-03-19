// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMEDITREDDITACCOUNT_H
#define FORMEDITREDDITACCOUNT_H

#include "services/abstract/gui/formaccountdetails.h"
#include "services/reddit/redditnetworkfactory.h"

class RedditServiceRoot;
class RedditAccountDetails;

class FormEditRedditAccount : public FormAccountDetails {
    Q_OBJECT

  public:
    explicit FormEditRedditAccount(QWidget* parent = nullptr);

  protected slots:
    virtual void apply();

  protected:
    virtual void loadAccountData();

  private:
    RedditAccountDetails* m_details;
};

#endif // FORMEDITREDDITACCOUNT_H
