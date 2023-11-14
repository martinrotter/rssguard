// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef GREADERFEEDDETAILS_H
#define GREADERFEEDDETAILS_H

#include <QWidget>

#include "ui_greaderfeeddetails.h"

class Category;
class RootItem;

class GreaderFeedDetails : public QWidget {
    Q_OBJECT

    friend class FormGreaderFeedDetails;

  public:
    explicit GreaderFeedDetails(QWidget* parent = nullptr);

  private slots:
    void onUrlChanged(const QString& new_url);
    void onTitleChanged(const QString& new_title);

  private:
    void loadCategories(const QList<Category*>& categories, RootItem* root_item, RootItem* parent_to_select = nullptr);

  private:
    Ui::GreaderFeedDetails ui;
};

#endif // GREADERFEEDDETAILS_H
