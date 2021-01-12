// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef INOREADERACCOUNTDETAILS_H
#define INOREADERACCOUNTDETAILS_H

#include <QWidget>

#include "ui_inoreaderaccountdetails.h"

class InoreaderAccountDetails : public QWidget {
  Q_OBJECT

  public:
    explicit InoreaderAccountDetails(QWidget* parent = nullptr);

  private:
    Ui::InoreaderAccountDetails m_ui;
};

#endif // INOREADERACCOUNTDETAILS_H
