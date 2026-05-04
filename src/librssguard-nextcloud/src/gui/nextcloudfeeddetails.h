// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef NEXTCLOUDFEEDDETAILS_H
#define NEXTCLOUDFEEDDETAILS_H

#include "ui_nextcloudfeeddetails.h"

#include <QWidget>

class Category;
class RootItem;

class NextcloudFeedDetails : public QWidget {
    Q_OBJECT

    friend class FormNextcloudFeedDetails;

  public:
    explicit NextcloudFeedDetails(QWidget* parent = nullptr);

  private slots:
    void onUrlChanged(const QString& new_url);

  private:
    void loadCategories(const QList<Category*>& categories, RootItem* root_item, RootItem* parent_to_select = nullptr);

  private:
    Ui::NextcloudFeedDetails ui;
};

#endif // NEXTCLOUDFEEDDETAILS_H
