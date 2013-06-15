#include "gui/formsettings.h"
#include "gui/themefactory.h"
#include "core/settings.h"


FormSettings::FormSettings(QWidget *parent) : QDialog(parent), m_ui(new Ui::FormSettings) {
  m_ui->setupUi(this);

  // Set flags.
  setWindowFlags(Qt::MSWindowsFixedSizeDialogHint | Qt::Dialog);

  // Establish needed connections.
  connect(this, &FormSettings::accepted, this, &FormSettings::saveSettings);

  // Load all settings.
  loadInterface();
}

FormSettings::~FormSettings() {
  delete m_ui;
}

void FormSettings::saveSettings() {
  // Save all categories.
  //saveGeneral();
  saveInterface();
  //saveLanguages();

  // Make sure that settings is synced.
  Settings::getInstance().checkSettings();
}

void FormSettings::loadInterface() {
  QString current_theme = ThemeFactory::getCurrentIconTheme();
#if defined(Q_OS_LINUX)
  QString system_theme = ThemeFactory::getSystemIconTheme();
#endif

  foreach (QString icon_theme_name, ThemeFactory::getInstalledIconThemes()) {
#if defined(Q_OS_LINUX)
    if (icon_theme_name == system_theme) {
      m_ui->m_cmbIconTheme->addItem(tr("system icon theme (default)"),
                                    icon_theme_name);
    }
    else {
#endif
      m_ui->m_cmbIconTheme->addItem(icon_theme_name,
                                    icon_theme_name);
#if defined(Q_OS_LINUX)
    }
    if (current_theme == system_theme) {
      // Because system icon theme lies at the index 0.
      // See ThemeFactory::getInstalledIconThemes() for more info.
      m_ui->m_cmbIconTheme->setCurrentIndex(0);
    }
    else {
#endif
      // TODO: Display correct theme on linux.
      m_ui->m_cmbIconTheme->setCurrentText(current_theme);
#if defined(Q_OS_LINUX)
    }
#endif
  }
}

void FormSettings::saveInterface() {
  // Save selected icon theme.
  ThemeFactory::setCurrentIconTheme(m_ui->m_cmbIconTheme->itemData(m_ui->m_cmbIconTheme->currentIndex()).toString());
}
