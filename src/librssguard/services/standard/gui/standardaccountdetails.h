// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef STANDARDACCOUNTDETAILS_H
#define STANDARDACCOUNTDETAILS_H

#include <QWidget>

#include "ui_standardaccountdetails.h"

namespace Ui {
  class StandardAccountDetails;
}

class StandardAccountDetails : public QWidget {
    Q_OBJECT

    friend class FormEditStandardAccount;

  public:
    explicit StandardAccountDetails(QWidget* parent = nullptr);

  private slots:
    void onLoadIconFromFile();
    void onUseDefaultIcon();

  private:
    Ui::StandardAccountDetails m_ui;
};

#endif // STANDARDACCOUNTDETAILS_H
