// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMTTRSSFEEDDETAILS_H
#define FORMTTRSSFEEDDETAILS_H

#include "services/abstract/gui/formfeeddetails.h"

class FormTtRssFeedDetails : public FormFeedDetails {
  Q_OBJECT

  public:
    explicit FormTtRssFeedDetails(ServiceRoot* service_root, QWidget* parent = 0);

    // FormFeedDetails interface

  protected slots:
    void apply();

  protected:
    void setEditableFeed(Feed* editable_feed);
};

#endif // FORMTTRSSFEEDDETAILS_H
