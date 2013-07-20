#include <QMessageBox>
#include <QProcess>
#include <QNetworkProxy>
#include <QColorDialog>

#include "gui/formsettings.h"
#include "gui/themefactory.h"
#include "gui/systemtrayicon.h"
#include "gui/formmain.h"
#include "core/settings.h"
#include "core/defs.h"
#include "core/localization.h"
#include "core/systemfactory.h"
#include "core/dynamicshortcuts.h"
#include "core/webbrowsernetworkaccessmanager.h"


FormSettings::FormSettings(QWidget *parent) : QDialog(parent), m_ui(new Ui::FormSettings) {
  m_ui->setupUi(this);

  // Set flags and attributes.
  setWindowFlags(Qt::MSWindowsFixedSizeDialogHint | Qt::Dialog);
  setWindowIcon(ThemeFactory::fromTheme("preferences-system"));

  // Setup behavior.
  m_ui->m_treeLanguages->setColumnCount(5);
  m_ui->m_treeLanguages->setHeaderHidden(false);
  m_ui->m_treeLanguages->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
  m_ui->m_treeLanguages->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
  m_ui->m_treeLanguages->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
  m_ui->m_treeLanguages->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
  m_ui->m_treeLanguages->header()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
  m_ui->m_treeLanguages->setHeaderLabels(QStringList()
                                         << tr("Language")
                                         << tr("Code")
                                         << tr("Version")
                                         << tr("Author")
                                         << tr("Email"));

  // Establish needed connections.
  connect(this, &FormSettings::accepted, this, &FormSettings::saveSettings);
  connect(m_ui->m_cmbProxyType, static_cast<void (QComboBox::*)(int index)>(&QComboBox::currentIndexChanged),
          this, &FormSettings::onProxyTypeChanged);
  connect(m_ui->m_checkShowPassword, &QCheckBox::stateChanged,
          this, &FormSettings::displayProxyPassword);
  connect(m_ui->m_btnBrowserProgressColor, &QPushButton::clicked,
          this, &FormSettings::changeBrowserProgressColor);

  // Load all settings.
  loadGeneral();
  loadShortcuts();
  loadInterface();
  loadProxy();
  loadLanguage();
}

FormSettings::~FormSettings() {
  delete m_ui;
}

void FormSettings::changeBrowserProgressColor() {
  QColorDialog color_dialog(m_initialSettings.m_webBrowserProgress, this);
  color_dialog.setWindowTitle(tr("Select color for web browser progress bar"));
  color_dialog.setOption(QColorDialog::ShowAlphaChannel);
  color_dialog.exec();

  m_initialSettings.m_webBrowserProgress = color_dialog.selectedColor();
}

void FormSettings::displayProxyPassword(int state) {
  if (state == Qt::Checked) {
    m_ui->m_txtProxyPassword->setEchoMode(QLineEdit::Normal);
  }
  else {
    m_ui->m_txtProxyPassword->setEchoMode(QLineEdit::PasswordEchoOnEdit);
  }
}

void FormSettings::saveSettings() {
  // Save all settings.
  saveGeneral();
  saveShortcuts();
  saveInterface();
  saveProxy();
  saveLanguage();

  Settings::getInstance()->checkSettings();
}

void FormSettings::onProxyTypeChanged(int index) {
  QNetworkProxy::ProxyType selected_type = static_cast<QNetworkProxy::ProxyType>(m_ui->m_cmbProxyType->itemData(index).toInt());
  bool is_proxy_selected = selected_type != QNetworkProxy::NoProxy;

  m_ui->m_txtProxyHost->setEnabled(is_proxy_selected);
  m_ui->m_txtProxyPassword->setEnabled(is_proxy_selected);
  m_ui->m_txtProxyUsername->setEnabled(is_proxy_selected);
  m_ui->m_spinProxyPort->setEnabled(is_proxy_selected);
  m_ui->m_checkShowPassword->setEnabled(is_proxy_selected);
}

void FormSettings::loadProxy() {
  m_ui->m_cmbProxyType->addItem(tr("No proxy"), QNetworkProxy::NoProxy);
  m_ui->m_cmbProxyType->addItem(tr("Socks5"), QNetworkProxy::Socks5Proxy);
  m_ui->m_cmbProxyType->addItem(tr("Http"), QNetworkProxy::HttpProxy);

  // Load the settings.
  QNetworkProxy::ProxyType selected_proxy_type = static_cast<QNetworkProxy::ProxyType>(Settings::getInstance()->value(APP_CFG_PROXY,
                                                                                                                      "proxy_type",
                                                                                                                      QNetworkProxy::NoProxy).toInt());
  m_ui->m_cmbProxyType->setCurrentIndex(m_ui->m_cmbProxyType->findData(selected_proxy_type));
  m_ui->m_txtProxyHost->setText(Settings::getInstance()->value(APP_CFG_PROXY,
                                                               "host").toString());
  m_ui->m_txtProxyUsername->setText(Settings::getInstance()->value(APP_CFG_PROXY,
                                                                   "username").toString());
  m_ui->m_txtProxyPassword->setText(Settings::getInstance()->value(APP_CFG_PROXY,
                                                                   "password").toString());
  m_ui->m_spinProxyPort->setValue(Settings::getInstance()->value(APP_CFG_PROXY,
                                                                 "port", 80).toInt());
}

void FormSettings::saveProxy() {
  Settings::getInstance()->setValue(APP_CFG_PROXY, "proxy_type",
                                    m_ui->m_cmbProxyType->itemData(m_ui->m_cmbProxyType->currentIndex()));
  Settings::getInstance()->setValue(APP_CFG_PROXY, "host",
                                    m_ui->m_txtProxyHost->text());
  Settings::getInstance()->setValue(APP_CFG_PROXY, "username",
                                    m_ui->m_txtProxyUsername->text());
  Settings::getInstance()->setValue(APP_CFG_PROXY, "password",
                                    m_ui->m_txtProxyPassword->text());
  Settings::getInstance()->setValue(APP_CFG_PROXY, "port",
                                    m_ui->m_spinProxyPort->value());

  // Reload settings for all network access managers.
  WebBrowser::globalNetworkManager()->loadSettings();
}

void FormSettings::loadLanguage() {
  QList<Language> languages = Localization::getInstalledLanguages();

  foreach (Language lang, languages) {
    QTreeWidgetItem *item = new QTreeWidgetItem(m_ui->m_treeLanguages);
    item->setText(0, lang.m_name);
    item->setText(1, lang.m_code);
    item->setText(2, lang.m_version);
    item->setText(3, lang.m_author);
    item->setText(4, lang.m_email);
    item->setIcon(0, QIcon(APP_FLAGS_PATH + "/" + lang.m_code + ".png"));
  }

  QList<QTreeWidgetItem*> matching_items = m_ui->m_treeLanguages->findItems(Settings::getInstance()->value(APP_CFG_GEN,
                                                                                                           "language",
                                                                                                           "en").toString(),
                                                                            Qt::MatchExactly,
                                                                            1);
  if (!matching_items.isEmpty()) {
    m_ui->m_treeLanguages->setCurrentItem(matching_items[0]);
  }
}

void FormSettings::saveLanguage() {
  if (m_ui->m_treeLanguages->currentItem() == nullptr) {
    qDebug("No localizations loaded in settings dialog, so no saving for them.");
    return;
  }

  QString actual_lang = Settings::getInstance()->value(APP_CFG_GEN,
                                                       "language",
                                                       "en").toString();
  QString new_lang = m_ui->m_treeLanguages->currentItem()->text(1);

  if (new_lang != actual_lang) {
    Settings::getInstance()->setValue(APP_CFG_GEN, "language", new_lang);

    QMessageBox msg_question(this);
    msg_question.setText(tr("Language of Qonverter was changed. Note that changes will take effect on next Qonverter start."));
    msg_question.setInformativeText(tr("Do you want to restart now?"));
    msg_question.setWindowTitle(tr("Language changed"));
    msg_question.setIcon(QMessageBox::Question);
    msg_question.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msg_question.setDefaultButton(QMessageBox::Yes);

    if (msg_question.exec() == QMessageBox::Yes) {
      if (!QProcess::startDetached(qApp->applicationFilePath())) {
        QMessageBox::warning(this,
                             tr("Problem with RSS Guard restart"),
                             tr("Qonverter couldn't be restarted, please restart it manually for changes to take effect."));
      }
      else {
        qApp->quit();
      }
    }
  }
}

void FormSettings::loadShortcuts() {
  m_ui->m_shortcuts->populate(FormMain::getInstance()->getActions());
}

void FormSettings::saveShortcuts() {
  // Update the actual shortcuts of some actions.
  m_ui->m_shortcuts->updateShortcuts();

  // Save new shortcuts to the settings.
  DynamicShortcuts::save(FormMain::getInstance()->getActions());
}

void FormSettings::loadGeneral() {
  // Load auto-start status.
  SystemFactory::AutoStartStatus autostart_status = SystemFactory::getAutoStartStatus();
  switch (autostart_status) {
    case SystemFactory::Enabled:
      m_ui->m_checkAutostart->setChecked(true);
      break;
    case SystemFactory::Disabled:
      m_ui->m_checkAutostart->setChecked(false);
      break;
    default:
      m_ui->m_checkAutostart->setEnabled(false);
      m_ui->m_checkAutostart->setText(m_ui->m_checkAutostart->text() +
                                      tr(" (not supported on this platform)"));
      break;
  }
}

void FormSettings::saveGeneral() {
  // If auto-start feature is available and user wants
  // to turn it on, then turn it on.
  if (SystemFactory::getAutoStartStatus() != SystemFactory::Unavailable) {
    if (m_ui->m_checkAutostart->isChecked()) {
      SystemFactory::setAutoStartStatus(SystemFactory::Enabled);
    }
    else {
      SystemFactory::setAutoStartStatus(SystemFactory::Disabled);
    }
  }
}

void FormSettings::loadInterface() {
  // Load settings of tray icon.
  if (SystemTrayIcon::isSystemTrayAvailable()) {
    m_ui->m_radioTrayOff->setChecked(!Settings::getInstance()->value(APP_CFG_GUI,
                                                                     "use_tray_icon",
                                                                     true).toBool());
    m_ui->m_cmbTrayClose->setCurrentIndex(Settings::getInstance()->value(APP_CFG_GUI,
                                                                         "close_win_action",
                                                                         0).toInt());
    m_ui->m_checkHidden->setChecked(Settings::getInstance()->value(APP_CFG_GUI,
                                                                   "start_hidden",
                                                                   false).toBool());
  }
  // Tray icon is not supported on this machine.
  else {
    m_ui->m_radioTrayOff->setText(tr("disable (Tray icon is not available.)"));
    m_ui->m_radioTrayOff->setChecked(true);
    m_ui->m_grpTray->setDisabled(true);
  }

  // Load settings of web browser GUI.
  m_initialSettings.m_webBrowserProgress = Settings::getInstance()->value(APP_CFG_GUI,
                                                                          "browser_progress_color",
                                                                          QColor(0, 255, 0, 100)).value<QColor>();
  m_ui->m_checkBrowserProgressColor->setChecked(Settings::getInstance()->value(APP_CFG_GUI,
                                                                               "browser_colored_progress_enabled",
                                                                               true).toBool());

  // Load settings of icon theme.
  QString current_theme = ThemeFactory::getCurrentIconTheme();

  foreach (QString icon_theme_name, ThemeFactory::getInstalledIconThemes()) {
#if defined(Q_OS_LINUX)
    if (icon_theme_name == APP_THEME_SYSTEM) {
      m_ui->m_cmbIconTheme->addItem(tr("system icon theme (default)"),
                                    icon_theme_name);
    }
    else {
#endif
      m_ui->m_cmbIconTheme->addItem(icon_theme_name,
                                    icon_theme_name);
#if defined(Q_OS_LINUX)
    }
    if (current_theme == APP_THEME_SYSTEM) {
      // Because system icon theme lies at the index 0.
      // See ThemeFactory::getInstalledIconThemes() for more info.
      m_ui->m_cmbIconTheme->setCurrentIndex(0);
    }
    else {
#endif
      m_ui->m_cmbIconTheme->setCurrentText(current_theme);
#if defined(Q_OS_LINUX)
    }
#endif
  }
}

void FormSettings::saveInterface() {
  // Save tray icon.
  if (SystemTrayIcon::isSystemTrayAvailable()) {
    Settings::getInstance()->setValue(APP_CFG_GUI, "use_tray_icon",
                                      m_ui->m_radioTrayOn->isChecked());
    Settings::getInstance()->setValue(APP_CFG_GUI, "close_win_action",
                                      m_ui->m_cmbTrayClose->currentIndex());
    Settings::getInstance()->setValue(APP_CFG_GUI, "start_hidden",
                                      m_ui->m_checkHidden->isChecked());
    if (Settings::getInstance()->value(APP_CFG_GUI, "use_tray_icon", true).toBool()) {
      SystemTrayIcon::getInstance()->show();
    }
    else {
      FormMain::getInstance()->display();
      SystemTrayIcon::deleteInstance();
    }
  }

  // Save settings of GUI of web browser.
  Settings::getInstance()->setValue(APP_CFG_GUI,
                                    "browser_progress_color",
                                    m_initialSettings.m_webBrowserProgress);
  Settings::getInstance()->setValue(APP_CFG_GUI,
                                    "browser_colored_progress_enabled",
                                    m_ui->m_checkBrowserProgressColor->isChecked());

  // Save selected icon theme.
  ThemeFactory::setCurrentIconTheme(m_ui->m_cmbIconTheme->itemData(m_ui->m_cmbIconTheme->currentIndex()).toString());
}
