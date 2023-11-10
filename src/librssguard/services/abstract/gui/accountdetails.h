// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef ACCOUNTDETAILS_H
#define ACCOUNTDETAILS_H

#include <QWidget>

#include "ui_accountdetails.h"

namespace Ui {
  class AccountDetails;
}

class AccountDetails : public QWidget {
    Q_OBJECT

    friend class FormAccountDetails;

  public:
    explicit AccountDetails(QWidget* parent = nullptr);

  private:
    Ui::AccountDetails m_ui;
};

#endif // ACCOUNTDETAILS_H
