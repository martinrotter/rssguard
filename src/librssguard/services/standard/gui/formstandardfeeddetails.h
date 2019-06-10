// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMSSFEEDDETAILS_H
#define FORMSSFEEDDETAILS_H

#include "services/abstract/gui/formfeeddetails.h"

class FormStandardFeedDetails : public FormFeedDetails {
  Q_OBJECT

  public:
    explicit FormStandardFeedDetails(ServiceRoot* service_root, QWidget* parent = nullptr);

  protected slots:
    void apply();

  protected:
    void setEditableFeed(Feed* editable_feed);
};

#endif // FORMSSFEEDDETAILS_H
