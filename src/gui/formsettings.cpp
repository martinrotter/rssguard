#include "gui/formsettings.h"


FormSettings::FormSettings(QWidget *parent) : QDialog(parent), m_ui(new Ui::FormSettings) {
  m_ui->setupUi(this);
}

FormSettings::~FormSettings() {
  delete m_ui;
}
