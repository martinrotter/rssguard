// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMSSFEEDDETAILS_H
#define FORMSSFEEDDETAILS_H

#include "services/abstract/gui/formfeeddetails.h"

class StandardFeedDetails;

class FormStandardFeedDetails : public FormFeedDetails {
  Q_OBJECT

  public:
    explicit FormStandardFeedDetails(ServiceRoot* service_root, QWidget* parent = nullptr);

  public slots:
    int addEditFeed(Feed* input_feed, RootItem* parent_to_select, const QString& url = QString());

  private slots:
    void onTitleChanged(const QString& new_title);
    void onDescriptionChanged(const QString& new_description);
    void onUrlChanged(const QString& new_url);
    void guessFeed();
    void guessIconOnly();
    void onLoadIconFromFile();
    void onUseDefaultIcon();

    virtual void apply();

  private:
    virtual void setEditableFeed(Feed* editable_feed);

    // Loads categories into the dialog from the model.
    void loadCategories(const QList<Category*>& categories, RootItem* root_item);

  private:
    StandardFeedDetails* m_standardFeedDetails;
    QMenu* m_iconMenu{};
    QAction* m_actionLoadIconFromFile{};
    QAction* m_actionUseDefaultIcon{};
    QAction* m_actionFetchIcon{};
};

#endif // FORMSSFEEDDETAILS_H
