// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/settings/settingsgui.h"

#include "core/feedsmodel.h"
#include "gui/dialogs/formmain.h"
#include "gui/feedmessageviewer.h"
#include "gui/feedstoolbar.h"
#include "gui/messagestoolbar.h"
#include "gui/statusbar.h"
#include "gui/systemtrayicon.h"
#include "gui/tabwidget.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/settings.h"

#include <QDropEvent>
#include <QStyleFactory>

SettingsGui::SettingsGui(Settings* settings, QWidget* parent) : SettingsPanel(settings, parent), m_ui(new Ui::SettingsGui) {
  m_ui->setupUi(this);
  m_ui->m_editorMessagesToolbar->activeItemsWidget()->viewport()->installEventFilter(this);
  m_ui->m_editorFeedsToolbar->activeItemsWidget()->viewport()->installEventFilter(this);
  m_ui->m_editorMessagesToolbar->availableItemsWidget()->viewport()->installEventFilter(this);
  m_ui->m_editorFeedsToolbar->availableItemsWidget()->viewport()->installEventFilter(this);
  m_ui->m_treeSkins->setColumnCount(4);
  m_ui->m_treeSkins->setHeaderHidden(false);
  m_ui->m_treeSkins->setHeaderLabels(QStringList()
                                     << /*: Skin list name column. */ tr("Name")
                                     << /*: Version column of skin list. */ tr("Version")
                                     << tr("Author")
                                     << tr("E-mail"));

  // Setup skins.
  m_ui->m_treeSkins->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
  m_ui->m_treeSkins->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
  m_ui->m_treeSkins->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
  m_ui->m_treeSkins->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
  connect(m_ui->m_cmbIconTheme, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &SettingsGui::requireRestart);
  connect(m_ui->m_cmbIconTheme, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
          &SettingsGui::dirtifySettings);
  connect(m_ui->m_treeSkins, &QTreeWidget::currentItemChanged, this, &SettingsGui::dirtifySettings);
  connect(m_ui->m_grpTray, &QGroupBox::toggled, this, &SettingsGui::dirtifySettings);
  connect(m_ui->m_checkEnableNotifications, &QCheckBox::toggled, this, &SettingsGui::dirtifySettings);
  connect(m_ui->m_checkHidden, &QCheckBox::toggled, this, &SettingsGui::dirtifySettings);
  connect(m_ui->m_checkMonochromeIcons, &QCheckBox::toggled, this, &SettingsGui::dirtifySettings);
  connect(m_ui->m_checkCountUnreadMessages, &QCheckBox::toggled, this, &SettingsGui::dirtifySettings);
  connect(m_ui->m_checkHideWhenMinimized, &QCheckBox::toggled, this, &SettingsGui::dirtifySettings);
  connect(m_ui->m_checkHideTabBarIfOneTabVisible, &QCheckBox::toggled, this, &SettingsGui::dirtifySettings);
  connect(m_ui->m_checkCloseTabsDoubleClick, &QCheckBox::toggled, this, &SettingsGui::dirtifySettings);
  connect(m_ui->m_checkCloseTabsMiddleClick, &QCheckBox::toggled, this, &SettingsGui::dirtifySettings);
  connect(m_ui->m_checkNewTabDoubleClick, &QCheckBox::toggled, this, &SettingsGui::dirtifySettings);
  connect(m_ui->m_grbCloseTabs, &QGroupBox::toggled, this, &SettingsGui::dirtifySettings);
  connect(m_ui->m_cmbToolbarButtonStyle, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
          &SettingsGui::dirtifySettings);
  connect(m_ui->m_editorFeedsToolbar, &ToolBarEditor::setupChanged, this, &SettingsGui::dirtifySettings);
  connect(m_ui->m_editorMessagesToolbar, &ToolBarEditor::setupChanged, this, &SettingsGui::dirtifySettings);
  connect(m_ui->m_editorStatusbar, &ToolBarEditor::setupChanged, this, &SettingsGui::dirtifySettings);
  connect(m_ui->m_editorStatusbar, &ToolBarEditor::setupChanged, this, &SettingsGui::requireRestart);
  connect(m_ui->m_cmbStyles, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &SettingsGui::dirtifySettings);
  connect(m_ui->m_cmbSelectToolBar, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), m_ui->m_stackedToolbars,
          &QStackedWidget::setCurrentIndex);
}

SettingsGui::~SettingsGui() {
  delete m_ui;
}

bool SettingsGui::eventFilter(QObject* obj, QEvent* e) {
  Q_UNUSED(obj)

  if (e->type() == QEvent::Type::Drop) {
    auto* drop_event = static_cast<QDropEvent*>(e);

    if (drop_event->keyboardModifiers() != Qt::NoModifier) {
      drop_event->setDropAction(Qt::MoveAction);
    }
  }

  return false;
}

void SettingsGui::loadSettings() {
  onBeginLoadSettings();

  // Load settings of tray icon.
  if (SystemTrayIcon::isSystemTrayAvailable()) {
    m_ui->m_grpTray->setChecked(settings()->value(GROUP(GUI), SETTING(GUI::UseTrayIcon)).toBool());
  }

  // Tray icon is not supported on this machine.
  else {
    m_ui->m_grpTray->setTitle(m_ui->m_grpTray->title() + QL1C(' ') + tr("(Tray icon is not available.)"));
    m_ui->m_grpTray->setChecked(false);
  }

  m_ui->m_checkHidden->setChecked(settings()->value(GROUP(GUI), SETTING(GUI::MainWindowStartsHidden)).toBool());
  m_ui->m_checkHideWhenMinimized->setChecked(settings()->value(GROUP(GUI), SETTING(GUI::HideMainWindowWhenMinimized)).toBool());

  // Load fancy notification settings.
  m_ui->m_checkEnableNotifications->setChecked(settings()->value(GROUP(GUI), SETTING(GUI::EnableNotifications)).toBool());

  // Load settings of icon theme.
  const QString current_theme = qApp->icons()->currentIconTheme();

  for (const QString& icon_theme_name : qApp->icons()->installedIconThemes()) {
    if (icon_theme_name == APP_NO_THEME) {
      // Add just "no theme" on other systems.
      //: Label for disabling icon theme.
#if defined(Q_OS_LINUX)
      m_ui->m_cmbIconTheme->addItem(tr("system icon theme"), APP_NO_THEME);
#else
      m_ui->m_cmbIconTheme->addItem(tr("no icon theme"), APP_NO_THEME);
#endif
    }
    else {
      m_ui->m_cmbIconTheme->addItem(icon_theme_name, icon_theme_name);
    }
  }

  m_ui->m_checkMonochromeIcons->setChecked(settings()->value(GROUP(GUI), SETTING(GUI::MonochromeTrayIcon)).toBool());
  m_ui->m_checkCountUnreadMessages->setChecked(settings()->value(GROUP(GUI), SETTING(GUI::UnreadNumbersInTrayIcon)).toBool());

  // Mark active theme.
  if (current_theme == QL1S(APP_NO_THEME)) {
    // Because "no icon theme" lies at the index 0.
    m_ui->m_cmbIconTheme->setCurrentIndex(0);
  }
  else {
    m_ui->m_cmbIconTheme->setCurrentText(current_theme);
  }

  // Load skin.
  const QString selected_skin = qApp->skins()->selectedSkinName();

  for (const Skin& skin : qApp->skins()->installedSkins()) {
    QTreeWidgetItem* new_item = new QTreeWidgetItem(QStringList() <<
                                                    skin.m_visibleName <<
                                                    skin.m_version <<
                                                    skin.m_author <<
                                                    skin.m_email);

    new_item->setData(0, Qt::UserRole, QVariant::fromValue(skin));

    // Add this skin and mark it as active if its active now.
    m_ui->m_treeSkins->addTopLevelItem(new_item);

    if (skin.m_baseName == selected_skin) {
      m_ui->m_treeSkins->setCurrentItem(new_item);
    }
  }

  if (m_ui->m_treeSkins->currentItem() == nullptr &&
      m_ui->m_treeSkins->topLevelItemCount() > 0) {
    // Currently active skin is NOT available, select another one as selected
    // if possible.
    m_ui->m_treeSkins->setCurrentItem(m_ui->m_treeSkins->topLevelItem(0));
  }

  // Load styles.
  for (const QString& style_name : QStyleFactory::keys()) {
    m_ui->m_cmbStyles->addItem(style_name);
  }

  int item_style = m_ui->m_cmbStyles->findText(settings()->value(GROUP(GUI), SETTING(GUI::Style)).toString(),
                                               Qt::MatchFlag::MatchFixedString);

  if (item_style >= 0) {
    m_ui->m_cmbStyles->setCurrentIndex(item_style);
  }

  // Load tab settings.
  m_ui->m_checkCloseTabsMiddleClick->setChecked(settings()->value(GROUP(GUI), SETTING(GUI::TabCloseMiddleClick)).toBool());
  m_ui->m_checkCloseTabsDoubleClick->setChecked(settings()->value(GROUP(GUI), SETTING(GUI::TabCloseDoubleClick)).toBool());
  m_ui->m_checkNewTabDoubleClick->setChecked(settings()->value(GROUP(GUI), SETTING(GUI::TabNewDoubleClick)).toBool());
  m_ui->m_checkHideTabBarIfOneTabVisible->setChecked(settings()->value(GROUP(GUI), SETTING(GUI::HideTabBarIfOnlyOneTab)).toBool());

  // Load toolbar button style.
  m_ui->m_cmbToolbarButtonStyle->addItem(tr("Icon only"), Qt::ToolButtonIconOnly);
  m_ui->m_cmbToolbarButtonStyle->addItem(tr("Text only"), Qt::ToolButtonTextOnly);
  m_ui->m_cmbToolbarButtonStyle->addItem(tr("Text beside icon"), Qt::ToolButtonTextBesideIcon);
  m_ui->m_cmbToolbarButtonStyle->addItem(tr("Text under icon"), Qt::ToolButtonTextUnderIcon);
  m_ui->m_cmbToolbarButtonStyle->addItem(tr("Follow OS style"), Qt::ToolButtonFollowStyle);
  m_ui->m_cmbToolbarButtonStyle->setCurrentIndex(m_ui->m_cmbToolbarButtonStyle->findData(settings()->value(GROUP(GUI),
                                                                                                           SETTING(
                                                                                                             GUI::ToolbarStyle)).toInt()));

  // Load toolbars.
  m_ui->m_editorFeedsToolbar->loadFromToolBar(qApp->mainForm()->tabWidget()->feedMessageViewer()->feedsToolBar());
  m_ui->m_editorMessagesToolbar->loadFromToolBar(qApp->mainForm()->tabWidget()->feedMessageViewer()->messagesToolBar());
  m_ui->m_editorStatusbar->loadFromToolBar(qApp->mainForm()->statusBar());
  onEndLoadSettings();
}

void SettingsGui::saveSettings() {
  onBeginSaveSettings();

  // Save toolbar.
  settings()->setValue(GROUP(GUI), GUI::ToolbarStyle,
                       m_ui->m_cmbToolbarButtonStyle->itemData(m_ui->m_cmbToolbarButtonStyle->currentIndex()));

  // Save tray icon.
  if (SystemTrayIcon::isSystemTrayAvailable()) {
    settings()->setValue(GROUP(GUI), GUI::UseTrayIcon, m_ui->m_grpTray->isChecked());

    if (m_ui->m_grpTray->isChecked()) {
      qApp->showTrayIcon();
    }
    else {
      qApp->deleteTrayIcon();
    }
  }

  auto old_monochrome = settings()->value(GROUP(GUI), SETTING(GUI::MonochromeTrayIcon)).toBool();

  if (old_monochrome != m_ui->m_checkMonochromeIcons->isChecked()) {
    requireRestart();
    settings()->setValue(GROUP(GUI), GUI::MonochromeTrayIcon, m_ui->m_checkMonochromeIcons->isChecked());
  }

  settings()->setValue(GROUP(GUI), GUI::UnreadNumbersInTrayIcon, m_ui->m_checkCountUnreadMessages->isChecked());
  settings()->setValue(GROUP(GUI), GUI::MainWindowStartsHidden, m_ui->m_checkHidden->isChecked());
  settings()->setValue(GROUP(GUI), GUI::HideMainWindowWhenMinimized, m_ui->m_checkHideWhenMinimized->isChecked());

  // Make sure that number of unread messages is shown in tray icon as requested.
  qApp->feedReader()->feedsModel()->notifyWithCounts();

  // Save notifications.
  settings()->setValue(GROUP(GUI), GUI::EnableNotifications, m_ui->m_checkEnableNotifications->isChecked());

  // Save selected icon theme.
  QString selected_icon_theme = m_ui->m_cmbIconTheme->itemData(m_ui->m_cmbIconTheme->currentIndex()).toString();
  QString original_icon_theme = qApp->icons()->currentIconTheme();

  qApp->icons()->setCurrentIconTheme(selected_icon_theme);

  // Check if icon theme was changed.
  if (selected_icon_theme != original_icon_theme) {
    requireRestart();
  }

  // Save and activate new skin.
  if (!m_ui->m_treeSkins->selectedItems().isEmpty()) {
    const Skin active_skin = m_ui->m_treeSkins->currentItem()->data(0, Qt::UserRole).value<Skin>();

    if (qApp->skins()->selectedSkinName() != active_skin.m_baseName) {
      qApp->skins()->setCurrentSkinName(active_skin.m_baseName);
      requireRestart();
    }
  }

  // Set new style.
  if (m_ui->m_cmbStyles->currentIndex() >= 0) {
    const QString new_style = m_ui->m_cmbStyles->currentText();
    const QString old_style = qApp->settings()->value(GROUP(GUI), SETTING(GUI::Style)).toString();

    if (old_style != new_style) {
      requireRestart();
    }

    qApp->settings()->setValue(GROUP(GUI), GUI::Style, new_style);
  }

  // Save tab settings.
  settings()->setValue(GROUP(GUI), GUI::TabCloseMiddleClick, m_ui->m_checkCloseTabsMiddleClick->isChecked());
  settings()->setValue(GROUP(GUI), GUI::TabCloseDoubleClick, m_ui->m_checkCloseTabsDoubleClick->isChecked());
  settings()->setValue(GROUP(GUI), GUI::TabNewDoubleClick, m_ui->m_checkNewTabDoubleClick->isChecked());
  settings()->setValue(GROUP(GUI), GUI::HideTabBarIfOnlyOneTab, m_ui->m_checkHideTabBarIfOneTabVisible->isChecked());
  m_ui->m_editorFeedsToolbar->saveToolBar();
  m_ui->m_editorMessagesToolbar->saveToolBar();
  m_ui->m_editorStatusbar->saveToolBar();
  qApp->mainForm()->tabWidget()->checkTabBarVisibility();
  qApp->mainForm()->tabWidget()->feedMessageViewer()->refreshVisualProperties();
  onEndSaveSettings();
}
