// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMSSFEEDDETAILS_H
#define FORMSSFEEDDETAILS_H

#include <librssguard/services/abstract/gui/formfeeddetails.h>

class StandardFeedDetails;
class StandardFeedExpDetails;
class StandardServiceRoot;
class AuthenticationDetails;
class StandardFeed;

class FormStandardFeedDetails : public FormFeedDetails {
    Q_OBJECT

  public:
    explicit FormStandardFeedDetails(ServiceRoot* service_root,
                                     RootItem* parent_to_select = nullptr,
                                     const QString& url = QString(),
                                     QWidget* parent = nullptr);

  private slots:
    void guessFeed();
    void guessIconOnly();
    void onTitleChanged(const QString& title);

    virtual void apply();

  private:
    virtual void loadFeedData();

  private:
    StandardFeedDetails* m_standardFeedDetails;
    StandardFeedExpDetails* m_standardFeedExpDetails;
    AuthenticationDetails* m_authDetails;
    RootItem* m_parentToSelect;
    QString m_urlToProcess;
};

#endif // FORMSSFEEDDETAILS_H
