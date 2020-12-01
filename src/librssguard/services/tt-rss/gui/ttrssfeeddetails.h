// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef TTRSSFEEDDETAILS_H
#define TTRSSFEEDDETAILS_H

#include <QWidget>

#include "ui_ttrssfeeddetails.h"

class Category;
class RootItem;

class TtRssFeedDetails : public QWidget {
  Q_OBJECT

  friend class FormTtRssFeedDetails;

  public:
    explicit TtRssFeedDetails(QWidget* parent = nullptr);

  private slots:
    void onUrlChanged(const QString& new_url);

  private:
    void loadCategories(const QList<Category*>& categories, RootItem* root_item, RootItem* parent_to_select = nullptr);

  private:
    Ui::TtRssFeedDetails ui;
};

#endif // TTRSSFEEDDETAILS_H
