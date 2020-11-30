// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMSSFEEDDETAILS_H
#define FORMSSFEEDDETAILS_H

#include "services/abstract/gui/formfeeddetails.h"

class StandardFeedDetails;
class AuthenticationDetails;
class StandardFeed;

class FormStandardFeedDetails : public FormFeedDetails {
  Q_OBJECT

  public:
    explicit FormStandardFeedDetails(ServiceRoot* service_root, QWidget* parent = nullptr);

  public slots:
    int addEditFeed(StandardFeed* input_feed, RootItem* parent_to_select, const QString& url = QString());

  private slots:
    void guessFeed();
    void guessIconOnly();

    virtual void apply();

  private:
    virtual void setEditableFeed(Feed* editable_feed);

  private:
    StandardFeedDetails* m_standardFeedDetails;
    AuthenticationDetails* m_authDetails;
};

#endif // FORMSSFEEDDETAILS_H
