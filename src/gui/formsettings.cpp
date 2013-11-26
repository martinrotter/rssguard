#include <QMessageBox>
#include <QProcess>
#include <QNetworkProxy>
#include <QColorDialog>

#include "gui/formsettings.h"
#include "gui/iconthemefactory.h"
#include "gui/skinfactory.h"
#include "gui/systemtrayicon.h"
#include "gui/formmain.h"
#include "gui/webbrowser.h"
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
  setWindowIcon(IconThemeFactory::getInstance()->fromTheme("preferences-system"));

  // Setup behavior.
  m_ui->m_treeLanguages->setColumnCount(5);
  m_ui->m_treeLanguages->setHeaderHidden(false);
  m_ui->m_treeLanguages->setHeaderLabels(QStringList()
                                         << tr("Language")
                                         << tr("Code")
                                         << tr("Version")
                                         << tr("Author")
                                         << tr("Email"));

  m_ui->m_treeSkins->setColumnCount(4);
  m_ui->m_treeSkins->setHeaderHidden(false);
  m_ui->m_treeSkins->setHeaderLabels(QStringList()
                                     << tr("Name")
                                     << tr("Version")
                                     << tr("Author")
                                     << tr("Email"));

#if QT_VERSION >= 0x050000
  // Setup languages.
  m_ui->m_treeLanguages->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
  m_ui->m_treeLanguages->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
  m_ui->m_treeLanguages->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
  m_ui->m_treeLanguages->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
  m_ui->m_treeLanguages->header()->setSectionResizeMode(4, QHeaderView::ResizeToContents);

  // Setup skins.
  m_ui->m_treeSkins->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
  m_ui->m_treeSkins->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
  m_ui->m_treeSkins->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
  m_ui->m_treeSkins->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
#else
  // Setup languages.
  m_ui->m_treeLanguages->header()->setResizeMode(0, QHeaderView::ResizeToContents);
  m_ui->m_treeLanguages->header()->setResizeMode(1, QHeaderView::ResizeToContents);
  m_ui->m_treeLanguages->header()->setResizeMode(2, QHeaderView::ResizeToContents);
  m_ui->m_treeLanguages->header()->setResizeMode(3, QHeaderView::ResizeToContents);
  m_ui->m_treeLanguages->header()->setResizeMode(4, QHeaderView::ResizeToContents);

  // Setup skins.
  m_ui->m_treeSkins->header()->setResizeMode(0, QHeaderView::ResizeToContents);
  m_ui->m_treeSkins->header()->setResizeMode(1, QHeaderView::ResizeToContents);
  m_ui->m_treeSkins->header()->setResizeMode(2, QHeaderView::ResizeToContents);
  m_ui->m_treeSkins->header()->setResizeMode(3, QHeaderView::ResizeToContents);
#endif

  // Establish needed connections.
  connect(m_ui->m_buttonBox, SIGNAL(accepted()),
          this, SLOT(saveSettings()));
  connect(m_ui->m_cmbProxyType, SIGNAL(currentIndexChanged(int)),
          this, SLOT(onProxyTypeChanged(int)));
  connect(m_ui->m_checkShowPassword, SIGNAL(stateChanged(int)),
          this, SLOT(displayProxyPassword(int)));
  connect(m_ui->m_btnBrowserProgressColor, SIGNAL(clicked()),
          this, SLOT(changeBrowserProgressColor()));
  connect(m_ui->m_treeSkins, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
          this, SLOT(onSkinSelected(QTreeWidgetItem*,QTreeWidgetItem*)));

  // Load all settings.
  loadGeneral();
  loadShortcuts();
  loadInterface();
  loadProxy();
  loadBrowser();
  loadLanguage();
}

FormSettings::~FormSettings() {
  delete m_ui;
}

void FormSettings::onSkinSelected(QTreeWidgetItem *current,
                                  QTreeWidgetItem *previous) {
  Q_UNUSED(previous);

  if (current != NULL) {
    Skin skin = current->data(0, Qt::UserRole).value<Skin>();
    m_ui->m_lblSelectedContents->setText(skin.m_visibleName);
  }
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

bool FormSettings::doSaveCheck() {
  bool everything_ok = true;
  QString resulting_information;

  everything_ok &= m_ui->m_shortcuts->areShortcutsUnique();

  if (!m_ui->m_shortcuts->areShortcutsUnique()) {
    resulting_information = resulting_information.append(tr("Some keyboard shortcuts are not unique.\n"));
  }

  if (!everything_ok) {
    QMessageBox::warning(this,
                         tr("Cannot save settings"),
                         resulting_information);
  }

  return everything_ok;
}

void FormSettings::saveSettings() {
  // Make sure everything is saveable.
  if (!doSaveCheck()) {
    return;
  }

  // Save all settings.
  saveGeneral();
  saveShortcuts();
  saveInterface();
  saveProxy();
  saveBrowser();
  saveLanguage();

  Settings::getInstance()->checkSettings();

  accept();
}

void FormSettings::onProxyTypeChanged(int index) {
  QNetworkProxy::ProxyType selected_type = static_cast<QNetworkProxy::ProxyType>(m_ui->m_cmbProxyType->itemData(index).toInt());
  bool is_proxy_selected = selected_type != QNetworkProxy::NoProxy;

  m_ui->m_txtProxyHost->setEnabled(is_proxy_selected);
  m_ui->m_txtProxyPassword->setEnabled(is_proxy_selected);
  m_ui->m_txtProxyUsername->setEnabled(is_proxy_selected);
  m_ui->m_spinProxyPort->setEnabled(is_proxy_selected);
  m_ui->m_checkShowPassword->setEnabled(is_proxy_selected);
  m_ui->m_lblProxyHost->setEnabled(is_proxy_selected);
  m_ui->m_lblProxyInfo->setEnabled(is_proxy_selected);
  m_ui->m_lblProxyPassword->setEnabled(is_proxy_selected);
  m_ui->m_lblProxyPort->setEnabled(is_proxy_selected);
  m_ui->m_lblProxyUsername->setEnabled(is_proxy_selected);
}

void FormSettings::loadBrowser() {
  Settings *settings = Settings::getInstance();

  // Load settings of web browser GUI.
  m_initialSettings.m_webBrowserProgress = settings->value(APP_CFG_BROWSER,
                                                           "browser_progress_color",
                                                           QColor(0, 255, 0, 100)).value<QColor>();
  m_ui->m_checkBrowserProgressColor->setChecked(settings->value(APP_CFG_BROWSER,
                                                                "browser_colored_progress_enabled",
                                                                true).toBool());
  m_ui->m_checkMouseGestures->setChecked(settings->value(APP_CFG_BROWSER,
                                                         "gestures_enabled",
                                                         true).toBool());
  m_ui->m_checkQueueTabs->setChecked(settings->value(APP_CFG_BROWSER,
                                                     "queue_tabs",
                                                     true).toBool());
}

void FormSettings::saveBrowser() {
  Settings *settings = Settings::getInstance();

  // Save settings of GUI of web browser.
  settings->setValue(APP_CFG_BROWSER,
                     "browser_progress_color",
                     m_initialSettings.m_webBrowserProgress);
  settings->setValue(APP_CFG_BROWSER,
                     "browser_colored_progress_enabled",
                     m_ui->m_checkBrowserProgressColor->isChecked());
  settings->setValue(APP_CFG_BROWSER,
                     "gestures_enabled",
                     m_ui->m_checkMouseGestures->isChecked());
  settings->setValue(APP_CFG_BROWSER,
                     "queue_tabs",
                     m_ui->m_checkQueueTabs->isChecked());
}

void FormSettings::loadProxy() {
  m_ui->m_cmbProxyType->addItem(tr("No proxy"), QNetworkProxy::NoProxy);
  m_ui->m_cmbProxyType->addItem(tr("Socks5"), QNetworkProxy::Socks5Proxy);
  m_ui->m_cmbProxyType->addItem(tr("Http"), QNetworkProxy::HttpProxy);

  // Load the settings.
  QNetworkProxy::ProxyType selected_proxy_type = static_cast<QNetworkProxy::ProxyType>(Settings::getInstance()->value(APP_CFG_PROXY,
                                                                                                                      "proxy_type",
                                                                                                                      QNetworkProxy::NoProxy).toInt());
  Settings *settings = Settings::getInstance();

  m_ui->m_cmbProxyType->setCurrentIndex(m_ui->m_cmbProxyType->findData(selected_proxy_type));
  m_ui->m_txtProxyHost->setText(settings->value(APP_CFG_PROXY,
                                                "host").toString());
  m_ui->m_txtProxyUsername->setText(settings->value(APP_CFG_PROXY,
                                                    "username").toString());
  m_ui->m_txtProxyPassword->setText(settings->value(APP_CFG_PROXY,
                                                    "password").toString());
  m_ui->m_spinProxyPort->setValue(settings->value(APP_CFG_PROXY,
                                                  "port", 80).toInt());
}

void FormSettings::saveProxy() {
  Settings *settings = Settings::getInstance();

  settings->setValue(APP_CFG_PROXY, "proxy_type",
                     m_ui->m_cmbProxyType->itemData(m_ui->m_cmbProxyType->currentIndex()));
  settings->setValue(APP_CFG_PROXY, "host",
                     m_ui->m_txtProxyHost->text());
  settings->setValue(APP_CFG_PROXY, "username",
                     m_ui->m_txtProxyUsername->text());
  settings->setValue(APP_CFG_PROXY, "password",
                     m_ui->m_txtProxyPassword->text());
  settings->setValue(APP_CFG_PROXY, "port",
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
  if (m_ui->m_treeLanguages->currentItem() == NULL) {
    qDebug("No localizations loaded in settings dialog, so no saving for them.");
    return;
  }

  Settings *settings = Settings::getInstance();
  QString actual_lang = settings->value(APP_CFG_GEN,
                                        "language",
                                        "en").toString();
  QString new_lang = m_ui->m_treeLanguages->currentItem()->text(1);

  if (new_lang != actual_lang) {
    settings->setValue(APP_CFG_GEN, "language", new_lang);

    QMessageBox msg_question(this);
    msg_question.setText(tr("Language of RSS Guard was changed. Note that changes will take effect on next Qonverter start."));
    msg_question.setInformativeText(tr("Do you want to restart now?"));
    msg_question.setWindowTitle(tr("Language changed"));
    msg_question.setIcon(QMessageBox::Question);
    msg_question.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msg_question.setDefaultButton(QMessageBox::Yes);

    if (msg_question.exec() == QMessageBox::Yes) {
      if (!QProcess::startDetached(qApp->applicationFilePath())) {
        QMessageBox::warning(this,
                             tr("Problem with RSS Guard restart"),
                             tr("RSS Guard couldn't be restarted, please restart it manually for changes to take effect."));
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
  Settings *settings = Settings::getInstance();

  // Load settings of tray icon.
  if (SystemTrayIcon::isSystemTrayAvailable()) {
    m_ui->m_radioTrayOff->setChecked(!settings->value(APP_CFG_GUI,
                                                      "use_tray_icon",
                                                      true).toBool());
    m_ui->m_cmbTrayClose->setCurrentIndex(settings->value(APP_CFG_GUI,
                                                          "close_win_action",
                                                          0).toInt());
    m_ui->m_checkHidden->setChecked(settings->value(APP_CFG_GUI,
                                                    "start_hidden",
                                                    false).toBool());
  }
  // Tray icon is not supported on this machine.
  else {
    m_ui->m_radioTrayOff->setText(tr("Disable (Tray icon is not available.)"));
    m_ui->m_radioTrayOff->setChecked(true);
    m_ui->m_grpTray->setDisabled(true);
  }

  // Load settings of icon theme.
  QString current_theme = IconThemeFactory::getInstance()->getCurrentIconTheme();

  foreach (QString icon_theme_name, IconThemeFactory::getInstance()->getInstalledIconThemes()) {
    if (icon_theme_name == APP_THEME_SYSTEM) {
#if defined(Q_OS_LINUX)
      m_ui->m_cmbIconTheme->addItem(tr("system icon theme (default)"),
                                    APP_THEME_SYSTEM);
#else
      m_ui->m_cmbIconTheme->addItem(tr("no icon theme"),
                                    APP_THEME_SYSTEM);
#endif
    }
    else {
      m_ui->m_cmbIconTheme->addItem(icon_theme_name,
                                    icon_theme_name);
    }
  }

  // Mark active theme.
  if (current_theme == APP_THEME_SYSTEM) {
    // Because system icon theme lies at the index 0.
    m_ui->m_cmbIconTheme->setCurrentIndex(0);
  }
  else {
#if QT_VERSION >= 0x050000
    m_ui->m_cmbIconTheme->setCurrentText(current_theme);
#else
    int theme_index = m_ui->m_cmbIconTheme->findText(current_theme);
    if (theme_index >= 0) {
      m_ui->m_cmbIconTheme->setCurrentIndex(theme_index);
    }
#endif
  }

  // Load skin.
  QList<Skin> installed_skins = SkinFactory::getInstance()->getInstalledSkins();
  QString selected_skin = SkinFactory::getInstance()->getSelectedSkinName();

  foreach (Skin skin, installed_skins) {
    QTreeWidgetItem *new_item = new QTreeWidgetItem(QStringList() <<
                                                    skin.m_visibleName <<
                                                    skin.m_version <<
                                                    skin.m_author <<
                                                    skin.m_email);
    new_item->setData(0, Qt::UserRole, QVariant::fromValue(skin));

    // Add this skin and mark it as active if its active now.
    m_ui->m_treeSkins->addTopLevelItem(new_item);

    if (skin.m_baseName == selected_skin) {
      m_ui->m_treeSkins->setCurrentItem(new_item);
      m_ui->m_lblActiveContents->setText(skin.m_visibleName);
    }
  }

  if (m_ui->m_treeSkins->currentItem() == NULL &&
      m_ui->m_treeSkins->topLevelItemCount() > 0) {
    // Currently active skin is NOT available, select another one as selected
    // if possible.
    m_ui->m_treeSkins->setCurrentItem(m_ui->m_treeSkins->topLevelItem(0));
  }

  // Load tab settings.
  m_ui->m_checkCloseTabsMiddleClick->setChecked(settings->value(APP_CFG_GUI,
                                                                "tab_close_mid_button",
                                                                true).toBool());
  m_ui->m_checkCloseTabsDoubleClick->setChecked(settings->value(APP_CFG_GUI,
                                                                "tab_close_double_button",
                                                                true).toBool());
  m_ui->m_checkNewTabDoubleClick->setChecked(settings->value(APP_CFG_GUI,
                                                             "tab_new_double_button",
                                                             true).toBool());
  m_ui->m_hideTabBarIfOneTabVisible->setChecked(settings->value(APP_CFG_GUI,
                                                                "hide_tabbar_one_tab",
                                                                true).toBool());
}

void FormSettings::saveInterface() {
  Settings *settings = Settings::getInstance();

  // Save tray icon.
  if (SystemTrayIcon::isSystemTrayAvailable()) {
    settings->setValue(APP_CFG_GUI, "use_tray_icon",
                       m_ui->m_radioTrayOn->isChecked());
    settings->setValue(APP_CFG_GUI, "close_win_action",
                       m_ui->m_cmbTrayClose->currentIndex());
    settings->setValue(APP_CFG_GUI, "start_hidden",
                       m_ui->m_checkHidden->isChecked());
    if (settings->value(APP_CFG_GUI, "use_tray_icon", true).toBool()) {
      SystemTrayIcon::getInstance()->show();
    }
    else {
      FormMain::getInstance()->display();
      SystemTrayIcon::deleteInstance();
    }
  }

  // Save selected icon theme.
  QString selected_icon_theme = m_ui->m_cmbIconTheme->itemData(m_ui->m_cmbIconTheme->currentIndex()).toString();
  IconThemeFactory::getInstance()->setCurrentIconTheme(selected_icon_theme);

  // Save and activate new skin.
  if (m_ui->m_treeSkins->selectedItems().size() > 0) {
    Skin active_skin = m_ui->m_treeSkins->currentItem()->data(0, Qt::UserRole).value<Skin>();
    SkinFactory::getInstance()->setCurrentSkinName(active_skin.m_baseName);
  }

  // Save tab settings.
  settings->setValue(APP_CFG_GUI, "tab_close_mid_button",
                     m_ui->m_checkCloseTabsMiddleClick->isChecked());
  settings->setValue(APP_CFG_GUI, "tab_close_double_button",
                     m_ui->m_checkCloseTabsDoubleClick->isChecked());
  settings->setValue(APP_CFG_GUI, "tab_new_double_button",
                     m_ui->m_checkNewTabDoubleClick->isChecked());
  settings->setValue(APP_CFG_GUI, "hide_tabbar_one_tab",
                     m_ui->m_hideTabBarIfOneTabVisible->isChecked());
}
