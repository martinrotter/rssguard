// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef STANDARDFEEDDETAILS_H
#define STANDARDFEEDDETAILS_H

#include <QWidget>

#include "ui_standardfeeddetails.h"

class Category;
class RootItem;
class StandardFeed;

class StandardFeedDetails : public QWidget {
  Q_OBJECT

  friend class FormStandardFeedDetails;

  public:
    explicit StandardFeedDetails(QWidget* parent = nullptr);

  private slots:
    void guessIconOnly(const QString& url, const QString& username, const QString& password);
    void guessFeed(const QString& url, const QString& username, const QString& password);

    void onTitleChanged(const QString& new_title);
    void onDescriptionChanged(const QString& new_description);
    void onUrlChanged(const QString& new_url);
    void onLoadIconFromFile();
    void onUseDefaultIcon();

  private:
    void prepareForNewFeed(RootItem* parent_to_select, const QString& url);
    void setExistingFeed(StandardFeed* feed);
    void loadCategories(const QList<Category*>& categories, RootItem* root_item);

  private:
    Ui::StandardFeedDetails ui;
    QMenu* m_iconMenu{};
    QAction* m_actionLoadIconFromFile{};
    QAction* m_actionUseDefaultIcon{};
    QAction* m_actionFetchIcon{};
};

#endif // STANDARDFEEDDETAILS_H
