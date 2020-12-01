// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMTTRSSFEEDDETAILS_H
#define FORMTTRSSFEEDDETAILS_H

#include "services/abstract/gui/formfeeddetails.h"

class TtRssFeed;
class TtRssFeedDetails;
class AuthenticationDetails;

class FormTtRssFeedDetails : public FormFeedDetails {
  public:
    explicit FormTtRssFeedDetails(ServiceRoot* service_root, QWidget* parent = nullptr);

  public slots:
    int addFeed(RootItem* parent_to_select, const QString& url = QString());

  protected slots:
    virtual void apply();

  private:
    TtRssFeedDetails* m_feedDetails;
    AuthenticationDetails* m_authDetails;
};

#endif // FORMTTRSSFEEDDETAILS_H
