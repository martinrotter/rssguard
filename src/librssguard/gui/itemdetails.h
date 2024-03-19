// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef ITEMDETAILS_H
#define ITEMDETAILS_H

#include "ui_itemdetails.h"

#include <QWidget>

class RootItem;

class ItemDetails : public QWidget {
    Q_OBJECT

  public:
    explicit ItemDetails(QWidget* parent = nullptr);
    virtual ~ItemDetails();

    void loadItemDetails(RootItem* item);

  private:
    Ui::ItemDetails m_ui;
};

#endif // ITEMDETAILS_H
