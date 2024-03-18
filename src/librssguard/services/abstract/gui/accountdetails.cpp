// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/abstract/gui/accountdetails.h"

#include "ui_accountdetails.h"

AccountDetails::AccountDetails(QWidget* parent) : QWidget(parent), m_ui(new Ui::AccountDetails()) {
  m_ui->setupUi(this);
}

AccountDetails::~AccountDetails() = default;
