#include "gui/settings/settingsgeneral.h"


SettingsGeneral::SettingsGeneral(Settings *settings, QWidget *parent)
  : SettingsPanel(settings, parent), m_ui(new Ui::SettingsGeneral) {
  m_ui->setupUi(this);
  m_ui->m_checkAutostart->setText(m_ui->m_checkAutostart->text().arg(APP_NAME));
}

SettingsGeneral::~SettingsGeneral() {
  delete m_ui;
}
