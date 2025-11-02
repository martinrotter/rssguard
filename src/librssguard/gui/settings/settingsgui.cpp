// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/settings/settingsgui.h"

#include "core/feedsmodel.h"
#include "core/messagesmodel.h"
#include "definitions/globals.h"
#include "gui/dialogs/formmain.h"
#include "gui/reusable/colortoolbutton.h"
#include "gui/reusable/plaintoolbutton.h"
#include "gui/systemtrayicon.h"
#include "gui/tabwidget.h"
#include "gui/toolbars/statusbar.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/settings.h"

#include <QDropEvent>
#include <QFontDialog>
#include <QStyleFactory>

SettingsGui::SettingsGui(Settings* settings, QWidget* parent) : SettingsPanel(settings, parent), m_ui(nullptr) {}

SettingsGui::~SettingsGui() {
  if (m_ui != nullptr) {
    delete m_ui;
  }
}

void SettingsGui::loadUi() {
  m_ui = new Ui::SettingsGui();
  m_ui->setupUi(this);
  m_ui->m_treeSkins->setColumnCount(4);
  m_ui->m_treeSkins->setHeaderHidden(false);
  m_ui->m_treeSkins->setHeaderLabels({tr("Name"), tr("Author"), tr("Forced style"), tr("Forced skin colors")});

  m_ui->m_grpTaskbar->setVisible(
#if (defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)) || defined(Q_OS_WIN)
    true);
#else
    false);
#endif

  m_ui->m_helpSkinColors->setHelpText(tr("Note that skin colors for dialogs/controls only take effect with some "
                                         "styles, for example with the Fusion style."),
                                      false);

  m_ui->m_helpCustomSkinColors->setHelpText(tr("You can override some colors defined by your skin here. "
                                               "Some colors are used dynamically throughout the application."),
                                            false);

  // Setup skins.
  m_ui->m_treeSkins->header()->setSectionResizeMode(0, QHeaderView::ResizeMode::ResizeToContents);
  m_ui->m_treeSkins->header()->setSectionResizeMode(1, QHeaderView::ResizeMode::ResizeToContents);
  m_ui->m_treeSkins->header()->setSectionResizeMode(2, QHeaderView::ResizeMode::ResizeToContents);
  m_ui->m_treeSkins->header()->setSectionResizeMode(3, QHeaderView::ResizeMode::ResizeToContents);

  connect(m_ui->m_checkFontAntialiasing, &QCheckBox::toggled, this, &SettingsGui::requireRestart);
  connect(m_ui->m_checkFontAntialiasing, &QCheckBox::toggled, this, &SettingsGui::dirtifySettings);

  connect(m_ui->m_cmbStyles,
          static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
          this,
          &SettingsGui::updateSkinOptions);

  connect(m_ui->m_checkMonochromeIcons,
          &QCheckBox::toggled,
          m_ui->m_checkColoredIconsWhenArticles,
          &QCheckBox::setEnabled);
  connect(m_ui->m_cmbIconTheme,
          static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
          this,
          &SettingsGui::requireRestart);
  connect(m_ui->m_cmbIconTheme,
          static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
          this,
          &SettingsGui::dirtifySettings);
  connect(m_ui->m_treeSkins, &QTreeWidget::currentItemChanged, this, &SettingsGui::dirtifySettings);
  connect(m_ui->m_treeSkins, &QTreeWidget::currentItemChanged, this, &SettingsGui::updateSkinOptions);
  connect(m_ui->m_grpTray, &QGroupBox::toggled, this, &SettingsGui::dirtifySettings);
  connect(m_ui->m_checkHidden, &QCheckBox::toggled, this, &SettingsGui::dirtifySettings);
  connect(m_ui->m_checkForceAlternativePalette, &QCheckBox::toggled, this, &SettingsGui::dirtifySettings);
  connect(m_ui->m_checkForceAlternativePalette, &QCheckBox::toggled, this, &SettingsGui::requireRestart);
  connect(m_ui->m_checkMonochromeIcons, &QCheckBox::toggled, this, &SettingsGui::dirtifySettings);
  connect(m_ui->m_checkMonochromeIcons, &QCheckBox::toggled, this, &SettingsGui::requireRestart);
  connect(m_ui->m_checkColoredIconsWhenArticles, &QCheckBox::toggled, this, &SettingsGui::dirtifySettings);
  connect(m_ui->m_checkColoredIconsWhenArticles, &QCheckBox::toggled, this, &SettingsGui::requireRestart);
  connect(m_ui->m_checkCountUnreadMessages, &QCheckBox::toggled, this, &SettingsGui::dirtifySettings);
  connect(m_ui->m_checkHideWhenMinimized, &QCheckBox::toggled, this, &SettingsGui::dirtifySettings);
  connect(m_ui->m_checkHideTabBarIfOneTabVisible, &QCheckBox::toggled, this, &SettingsGui::dirtifySettings);
  connect(m_ui->m_checkCloseTabsDoubleClick, &QCheckBox::toggled, this, &SettingsGui::dirtifySettings);
  connect(m_ui->m_checkCloseTabsMiddleClick, &QCheckBox::toggled, this, &SettingsGui::dirtifySettings);
  connect(m_ui->m_checkNewTabDoubleClick, &QCheckBox::toggled, this, &SettingsGui::dirtifySettings);
  connect(m_ui->m_grbCloseTabs, &QGroupBox::toggled, this, &SettingsGui::dirtifySettings);
  connect(m_ui->m_cmbStyles,
          static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
          this,
          &SettingsGui::dirtifySettings);
  connect(m_ui->m_gbCustomSkinColors, &QGroupBox::toggled, this, &SettingsGui::dirtifySettings);
  connect(m_ui->m_displayUnreadMessageCountOnTaskBar, &QCheckBox::toggled, this, &SettingsGui::dirtifySettings);
  connect(m_ui->m_displayUnreadMessageCountOnWindow, &QCheckBox::toggled, this, &SettingsGui::dirtifySettings);
  connect(m_ui->m_gbAppFont, &QGroupBox::toggled, this, &SettingsGui::dirtifySettings);
  connect(m_ui->m_gbAppFont, &QGroupBox::toggled, this, &SettingsGui::requireRestart);
  connect(m_ui->m_btnChangeFont, &QPushButton::clicked, this, [&]() {
    changeFont(*m_ui->m_lblAppFont);
  });

  SettingsPanel::loadUi();
}

void SettingsGui::changeFont(QLabel& lbl) {
  bool ok;
  QFont new_font = QFontDialog::getFont(&ok,
                                        lbl.font(),
                                        this,
                                        tr("Select new font"),
                                        QFontDialog::FontDialogOption::DontUseNativeDialog);

  if (ok) {
    lbl.setFont(new_font);
    dirtifySettings();
    requireRestart();
  }
}

QIcon SettingsGui::icon() const {
  return qApp->icons()->fromTheme(QSL("draw-freehand"), QSL("draw-star"));
}

void SettingsGui::updateSkinOptions() {
  auto* it = m_ui->m_treeSkins->currentItem();

  if (it == nullptr) {
    return;
  }

  const Skin skin = it->data(0, Qt::ItemDataRole::UserRole).value<Skin>();
  const bool skin_has_palette_or_css = skin.hasPalette() || !skin.m_rawData.isEmpty();
  const bool skin_forces_palette = skin.m_forcedSkinColors;
  const bool skin_forces_style = !skin.m_forcedStyles.isEmpty();

  m_ui->m_cmbStyles->setEnabled(!qApp->skins()->styleIsFrozen() && !skin_forces_style);
  m_ui->m_checkForceAlternativePalette->setEnabled(skin_has_palette_or_css && !skin_forces_palette);
}

void SettingsGui::loadSettings() {
  onBeginLoadSettings();

  // Fonts.
  QFont app_fon;

  // Keep in sync with void MessagesModel::setupFonts().
  app_fon.fromString(settings()->value(GROUP(GUI), GUI::AppFont, QApplication::font().toString()).toString());
  m_ui->m_lblAppFont->setFont(app_fon);
  m_ui->m_gbAppFont->setChecked(settings()->value(GROUP(GUI), SETTING(GUI::CustomizeAppFont)).toBool());
  m_ui->m_checkFontAntialiasing->setChecked(settings()->value(GROUP(GUI), SETTING(GUI::FontAntialiasing)).toBool());

  // Load settings of tray icon.
  m_ui->m_grpTray->setChecked(settings()->value(GROUP(GUI), SETTING(GUI::UseTrayIcon)).toBool());

  if (!SystemTrayIcon::isSystemTrayAreaAvailable()) {
    m_ui->m_grpTray->setTitle(m_ui->m_grpTray->title() + QL1C(' ') +
                              tr("(Your OS does not support tray icons at the moment.)"));
    m_ui->m_grpTray->setEnabled(false);
  }

  m_ui->m_checkHidden->setChecked(settings()->value(GROUP(GUI), SETTING(GUI::MainWindowStartsHidden)).toBool());
  m_ui->m_checkHideWhenMinimized
    ->setChecked(settings()->value(GROUP(GUI), SETTING(GUI::HideMainWindowWhenMinimized)).toBool());

  // Load settings of icon theme.
  const QString current_icon_theme = qApp->icons()->currentIconTheme();
  auto icons = qApp->icons()->installedIconThemes();

  for (const QString& icon_theme_name : std::as_const(icons)) {
    if (icon_theme_name == QSL(APP_NO_THEME)) {
      // Add just "no theme" on other systems.
      //: Label for disabling icon theme.
#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
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
  m_ui->m_checkColoredIconsWhenArticles
    ->setChecked(settings()->value(GROUP(GUI), SETTING(GUI::ColoredBusyTrayIcon)).toBool());
  m_ui->m_checkCountUnreadMessages
    ->setChecked(settings()->value(GROUP(GUI), SETTING(GUI::UnreadNumbersInTrayIcon)).toBool());

#if (defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)) || defined(Q_OS_WIN)
  m_ui->m_displayUnreadMessageCountOnTaskBar
    ->setChecked(settings()->value(GROUP(GUI), SETTING(GUI::UnreadNumbersOnTaskBar)).toBool());
#endif

  m_ui->m_displayUnreadMessageCountOnWindow
    ->setChecked(settings()->value(GROUP(GUI), SETTING(GUI::UnreadNumbersOnWindow)).toBool());

  // Mark active icon theme.
  if (current_icon_theme == QL1S(APP_NO_THEME)) {
    // Because "no icon theme" lies at the index 0.
    m_ui->m_cmbIconTheme->setCurrentIndex(0);
  }
  else {
    m_ui->m_cmbIconTheme->setCurrentText(current_icon_theme);
  }

  // Load styles.
  auto styles = QStyleFactory::keys();

  m_ui->m_cmbStyles->addItem(
#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
    tr("system style"),
#else
    tr("default style"),
#endif
    APP_NO_THEME);

  for (const QString& style_name : std::as_const(styles)) {
    m_ui->m_cmbStyles->addItem(style_name, style_name);
  }

  int item_style = m_ui->m_cmbStyles->findData(qApp->skins()->currentStyle(),
                                               Qt::ItemDataRole::UserRole,
                                               Qt::MatchFlag::MatchFixedString);

  if (item_style >= 0) {
    m_ui->m_cmbStyles->setCurrentIndex(item_style);
  }

  m_ui->m_checkForceAlternativePalette
    ->setChecked(settings()->value(GROUP(GUI), SETTING(GUI::ForcedSkinColors)).toBool());

  // Load skin.
  const QString selected_skin = qApp->skins()->selectedSkinName();
  auto skins = qApp->skins()->installedSkins();

  for (const Skin& skin : std::as_const(skins)) {
    QTreeWidgetItem* new_item =
      new QTreeWidgetItem({skin.m_visibleName,
                           skin.m_author,
                           skin.m_forcedStyles.isEmpty() ? QString() : skin.m_forcedStyles.join(QSL(", ")),
                           QString()});

    new_item->setToolTip(0,
                         tr("%1\n\n"
                            "Version: %2\n"
                            "Description: %3")
                           .arg(skin.m_visibleName,
                                skin.m_version,
                                skin.m_description.isEmpty() ? QSL("-") : skin.m_description));

    for (int i = 1; i < m_ui->m_treeSkins->columnCount(); i++) {
      new_item->setToolTip(i, new_item->toolTip(0));
    }

    if (skin.m_forcedStyles.isEmpty()) {
      new_item->setIcon(2, qApp->icons()->fromTheme(QSL("dialog-cancel"), QSL("gtk-cancel")));
    }

    new_item->setIcon(3,
                      skin.m_forcedSkinColors ? qApp->icons()->fromTheme(QSL("dialog-yes"), QSL("dialog-ok"))
                                              : qApp->icons()->fromTheme(QSL("dialog-cancel"), QSL("gtk-cancel")));

    new_item->setData(0, Qt::UserRole, QVariant::fromValue(skin));

    // Add this skin and mark it as active if its active now.
    m_ui->m_treeSkins->addTopLevelItem(new_item);

    if (skin.m_baseName == selected_skin) {
      m_ui->m_treeSkins->setCurrentItem(new_item);
    }
  }

  if (m_ui->m_treeSkins->currentItem() == nullptr && m_ui->m_treeSkins->topLevelItemCount() > 0) {
    // Currently active skin is NOT available, select another one as selected
    // if possible.
    m_ui->m_treeSkins->setCurrentItem(m_ui->m_treeSkins->topLevelItem(0));
  }

  // Load tab settings.
  m_ui->m_checkCloseTabsMiddleClick
    ->setChecked(settings()->value(GROUP(GUI), SETTING(GUI::TabCloseMiddleClick)).toBool());
  m_ui->m_checkCloseTabsDoubleClick
    ->setChecked(settings()->value(GROUP(GUI), SETTING(GUI::TabCloseDoubleClick)).toBool());
  m_ui->m_checkNewTabDoubleClick->setChecked(settings()->value(GROUP(GUI), SETTING(GUI::TabNewDoubleClick)).toBool());
  m_ui->m_checkHideTabBarIfOneTabVisible
    ->setChecked(settings()->value(GROUP(GUI), SETTING(GUI::HideTabBarIfOnlyOneTab)).toBool());

  // Load custom colors.
  m_ui->m_gbCustomSkinColors
    ->setChecked(settings()->value(GROUP(CustomSkinColors), SETTING(CustomSkinColors::Enabled)).toBool());

  auto palette_enums = enumToStrings<SkinEnums::PaletteColors>();
  int row = 0;

  for (const auto& palette_enum : palette_enums) {
    auto* clr_btn = new ColorToolButton(this);
    auto* rst_btn = new PlainToolButton(this);

    rst_btn->setToolTip(tr("Fetch color from activated skin"));
    rst_btn->setIcon(qApp->icons()->fromTheme(QSL("edit-reset")));

    QColor clr = settings()->value(GROUP(CustomSkinColors), palette_enum.second).toString();

    if (!clr.isValid()) {
      clr = qApp->skins()->colorForModel(palette_enum.first).value<QColor>();
    }

    rst_btn->setObjectName(QString::number(int(palette_enum.first)));

    connect(rst_btn, &PlainToolButton::clicked, this, &SettingsGui::resetCustomSkinColor);
    connect(clr_btn, &ColorToolButton::colorChanged, this, &SettingsGui::dirtifySettings);

    clr_btn->setObjectName(QString::number(int(palette_enum.first)));
    clr_btn->setColor(clr);

    auto* lay = new QHBoxLayout();

    lay->addWidget(clr_btn);
    lay->addWidget(rst_btn);

    m_ui->m_layoutCustomColors
      ->setWidget(row,
                  QFormLayout::ItemRole::LabelRole,
                  new QLabel(TextFactory::capitalizeFirstLetter(SkinEnums::palleteColorText(palette_enum.first)),
                             this));
    m_ui->m_layoutCustomColors->setLayout(row, QFormLayout::ItemRole::FieldRole, lay);

    row++;
  }

  onEndLoadSettings();
}

void SettingsGui::resetCustomSkinColor() {
  auto* clr_btn = m_ui->m_gbCustomSkinColors->findChild<ColorToolButton*>(sender()->objectName());
  SkinEnums::PaletteColors pal = SkinEnums::PaletteColors(sender()->objectName().toInt());

  clr_btn->setColor(qApp->skins()->colorForModel(pal, true).value<QColor>());
}

void SettingsGui::saveSettings() {
  onBeginSaveSettings();

  settings()->setValue(GROUP(GUI), GUI::AppFont, m_ui->m_lblAppFont->font().toString());
  settings()->setValue(GROUP(GUI), GUI::CustomizeAppFont, m_ui->m_gbAppFont->isChecked());
  settings()->setValue(GROUP(GUI), GUI::FontAntialiasing, m_ui->m_checkFontAntialiasing->isChecked());

  // Save custom skin colors.
  settings()->setValue(GROUP(CustomSkinColors), CustomSkinColors::Enabled, m_ui->m_gbCustomSkinColors->isChecked());

  auto children = m_ui->m_gbCustomSkinColors->findChildren<ColorToolButton*>();

  for (const ColorToolButton* clr : children) {
    auto pal = SkinEnums::PaletteColors(clr->objectName().toInt());

    settings()->setValue(GROUP(CustomSkinColors), enumToString<SkinEnums::PaletteColors>(pal), clr->color().name());
  }

  // Save tray icon.
  if (SystemTrayIcon::isSystemTrayAreaAvailable()) {
    settings()->setValue(GROUP(GUI), GUI::UseTrayIcon, m_ui->m_grpTray->isChecked());

    if (m_ui->m_grpTray->isChecked()) {
      qApp->showTrayIcon();
    }
    else {
      qApp->deleteTrayIcon();
    }
  }
  settings()->setValue(GROUP(GUI), GUI::MonochromeTrayIcon, m_ui->m_checkMonochromeIcons->isChecked());
  settings()->setValue(GROUP(GUI), GUI::ColoredBusyTrayIcon, m_ui->m_checkColoredIconsWhenArticles->isChecked());
  settings()->setValue(GROUP(GUI), GUI::UnreadNumbersInTrayIcon, m_ui->m_checkCountUnreadMessages->isChecked());
  settings()->setValue(GROUP(GUI), GUI::MainWindowStartsHidden, m_ui->m_checkHidden->isChecked());
  settings()->setValue(GROUP(GUI), GUI::HideMainWindowWhenMinimized, m_ui->m_checkHideWhenMinimized->isChecked());

#if (defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)) || defined(Q_OS_WIN)
  settings()->setValue(GROUP(GUI),
                       GUI::UnreadNumbersOnTaskBar,
                       m_ui->m_displayUnreadMessageCountOnTaskBar->isChecked());
#endif

  settings()->setValue(GROUP(GUI), GUI::UnreadNumbersOnWindow, m_ui->m_displayUnreadMessageCountOnWindow->isChecked());

  // Make sure that number of unread messages is shown in tray icon as requested.
  qApp->feedReader()->feedsModel()->notifyWithCounts();

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
    const Skin active_skin = m_ui->m_treeSkins->currentItem()->data(0, Qt::ItemDataRole::UserRole).value<Skin>();

    if (qApp->skins()->selectedSkinName() != active_skin.m_baseName) {
      qApp->skins()->setCurrentSkinName(active_skin.m_baseName);
      requireRestart();
    }
  }

  // Set new style.
  if (m_ui->m_cmbStyles->currentIndex() >= 0 && m_ui->m_cmbStyles->isEnabled()) {
    const QString new_style = m_ui->m_cmbStyles->currentData().toString();
    const QString old_style = qApp->settings()->value(GROUP(GUI), SETTING(GUI::Style)).toString();

    if (old_style != new_style) {
      requireRestart();
    }

    qApp->settings()->setValue(GROUP(GUI), GUI::Style, new_style);
  }

  if (m_ui->m_checkForceAlternativePalette->isEnabled()) {
    settings()->setValue(GROUP(GUI), GUI::ForcedSkinColors, m_ui->m_checkForceAlternativePalette->isChecked());
  }

  // Save tab settings.
  settings()->setValue(GROUP(GUI), GUI::TabCloseMiddleClick, m_ui->m_checkCloseTabsMiddleClick->isChecked());
  settings()->setValue(GROUP(GUI), GUI::TabCloseDoubleClick, m_ui->m_checkCloseTabsDoubleClick->isChecked());
  settings()->setValue(GROUP(GUI), GUI::TabNewDoubleClick, m_ui->m_checkNewTabDoubleClick->isChecked());
  settings()->setValue(GROUP(GUI), GUI::HideTabBarIfOnlyOneTab, m_ui->m_checkHideTabBarIfOneTabVisible->isChecked());

  qApp->mainForm()->tabWidget()->updateAppearance();
  qApp->mainForm()->tabWidget()->checkCornerButtonVisibility();

  qApp->feedReader()->feedsModel()->reloadWholeLayout();
  qApp->feedReader()->messagesModel()->setupIcons();
  qApp->feedReader()->messagesModel()->reloadWholeLayout();

  onEndSaveSettings();
}
