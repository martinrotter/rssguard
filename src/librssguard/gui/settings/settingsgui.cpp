// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/settings/settingsgui.h"

#include "core/feedsmodel.h"
#include "gui/dialogs/formmain.h"
#include "gui/feedmessageviewer.h"
#include "gui/reusable/colortoolbutton.h"
#include "gui/reusable/plaintoolbutton.h"
#include "gui/systemtrayicon.h"
#include "gui/tabwidget.h"
#include "gui/toolbars/feedstoolbar.h"
#include "gui/toolbars/messagestoolbar.h"
#include "gui/toolbars/statusbar.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/settings.h"

#include <QDropEvent>
#include <QMetaEnum>
#include <QMetaObject>
#include <QStyleFactory>

SettingsGui::SettingsGui(Settings* settings, QWidget* parent) : SettingsPanel(settings, parent), m_ui(new Ui::SettingsGui) {
  m_ui->setupUi(this);
  m_ui->m_editorMessagesToolbar->activeItemsWidget()->viewport()->installEventFilter(this);
  m_ui->m_editorFeedsToolbar->activeItemsWidget()->viewport()->installEventFilter(this);
  m_ui->m_editorMessagesToolbar->availableItemsWidget()->viewport()->installEventFilter(this);
  m_ui->m_editorFeedsToolbar->availableItemsWidget()->viewport()->installEventFilter(this);
  m_ui->m_treeSkins->setColumnCount(3);
  m_ui->m_treeSkins->setHeaderHidden(false);
  m_ui->m_treeSkins->setHeaderLabels(QStringList()
                                     << /*: Skin list name column. */ tr("Name")
                                     << /*: Version column of skin list. */ tr("Version")
                                     << tr("Author"));

  m_ui->m_helpCustomSkinColors->setHelpText(tr("You can override some colors defined by your skin here. "
                                               "Some colors are used dynamically throughout the application."), false);

  // Setup skins.
  m_ui->m_treeSkins->header()->setSectionResizeMode(0, QHeaderView::ResizeMode::ResizeToContents);
  m_ui->m_treeSkins->header()->setSectionResizeMode(1, QHeaderView::ResizeMode::ResizeToContents);
  m_ui->m_treeSkins->header()->setSectionResizeMode(2, QHeaderView::ResizeMode::ResizeToContents);

  connect(m_ui->m_cmbStyles, &QComboBox::currentTextChanged, this, [this](const QString& txt) {
    m_ui->m_checkForceDarkFusion->setVisible(qApp->skins()->isStyleGoodForDarkVariant(txt));
  });

  connect(m_ui->m_cmbIconTheme, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &SettingsGui::requireRestart);
  connect(m_ui->m_cmbIconTheme, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
          &SettingsGui::dirtifySettings);
  connect(m_ui->m_treeSkins, &QTreeWidget::currentItemChanged, this, &SettingsGui::dirtifySettings);
  connect(m_ui->m_grpTray, &QGroupBox::toggled, this, &SettingsGui::dirtifySettings);
  connect(m_ui->m_checkHidden, &QCheckBox::toggled, this, &SettingsGui::dirtifySettings);
  connect(m_ui->m_checkForceDarkFusion, &QCheckBox::toggled, this, &SettingsGui::dirtifySettings);
  connect(m_ui->m_checkForceDarkFusion, &QCheckBox::toggled, this, &SettingsGui::requireRestart);
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
  connect(m_ui->m_gbCustomSkinColors, &QGroupBox::toggled, this, &SettingsGui::dirtifySettings);
  connect(m_ui->m_spinToolbarIconSize, QOverload<int>::of(&QSpinBox::valueChanged), this, &SettingsGui::dirtifySettings);

  connect(m_ui->m_spinToolbarIconSize, QOverload<int>::of(&QSpinBox::valueChanged), this, [=](int value) {
    if (value <= 0) {
      m_ui->m_spinToolbarIconSize->setSuffix(tr(" = default icon size"));
    }
    else {
      m_ui->m_spinToolbarIconSize->setSuffix(QSL(" px"));
    }
  });
}

SettingsGui::~SettingsGui() {
  delete m_ui;
}

bool SettingsGui::eventFilter(QObject* obj, QEvent* e) {
  Q_UNUSED(obj)

  if (e->type() == QEvent::Type::Drop) {
    auto* drop_event = static_cast<QDropEvent*>(e);

#if QT_VERSION_MAJOR == 6
    if (drop_event->modifiers() !=
#else
    if (drop_event->keyboardModifiers() !=
#endif
        Qt::KeyboardModifier::NoModifier) {
      drop_event->setDropAction(Qt::DropAction::MoveAction);
    }
  }

  return false;
}

void SettingsGui::loadSettings() {
  onBeginLoadSettings();

  // Load settings of tray icon.
  m_ui->m_grpTray->setChecked(settings()->value(GROUP(GUI), SETTING(GUI::UseTrayIcon)).toBool());

  if (!SystemTrayIcon::isSystemTrayAreaAvailable()) {
    m_ui->m_grpTray->setTitle(m_ui->m_grpTray->title() + QL1C(' ') + tr("(Your OS does not support tray icons at the moment.)"));
    m_ui->m_grpTray->setEnabled(false);
  }

  m_ui->m_checkHidden->setChecked(settings()->value(GROUP(GUI), SETTING(GUI::MainWindowStartsHidden)).toBool());
  m_ui->m_checkHideWhenMinimized->setChecked(settings()->value(GROUP(GUI), SETTING(GUI::HideMainWindowWhenMinimized)).toBool());

  m_ui->m_checkForceDarkFusion->setChecked(settings()->value(GROUP(GUI),
                                                             SETTING(GUI::ForceDarkFusion)).toBool());

  // Load settings of icon theme.
  const QString current_theme = qApp->icons()->currentIconTheme();
  auto icons = qApp->icons()->installedIconThemes();

  for (const QString& icon_theme_name : qAsConst(icons)) {
    if (icon_theme_name == QSL(APP_NO_THEME)) {
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
  auto skins = qApp->skins()->installedSkins();

  for (const Skin& skin : qAsConst(skins)) {
    QTreeWidgetItem* new_item = new QTreeWidgetItem(QStringList() <<
                                                    skin.m_visibleName <<
                                                    skin.m_version <<
                                                    skin.m_author);

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
  auto styles = QStyleFactory::keys();

  for (const QString& style_name : qAsConst(styles)) {
    m_ui->m_cmbStyles->addItem(style_name);
  }

  int item_style = m_ui->m_cmbStyles->findText(settings()->value(GROUP(GUI), SETTING(GUI::Style)).toString(),
                                               Qt::MatchFlag::MatchFixedString);

  if (item_style >= 0) {
    m_ui->m_cmbStyles->setCurrentIndex(item_style);
  }

  if (qApp->skins()->styleIsFrozen()) {
    m_ui->m_cmbStyles->setEnabled(false);
    m_ui->m_cmbStyles->setToolTip(tr("You cannot change style because it was explicitly selected in your OS settings.\n"
                                     "Perhaps it is set with 'QT_STYLE_OVERRIDE' environment variable?"));
  }

  // Load tab settings.
  m_ui->m_checkCloseTabsMiddleClick->setChecked(settings()->value(GROUP(GUI), SETTING(GUI::TabCloseMiddleClick)).toBool());
  m_ui->m_checkCloseTabsDoubleClick->setChecked(settings()->value(GROUP(GUI), SETTING(GUI::TabCloseDoubleClick)).toBool());
  m_ui->m_checkNewTabDoubleClick->setChecked(settings()->value(GROUP(GUI), SETTING(GUI::TabNewDoubleClick)).toBool());
  m_ui->m_checkHideTabBarIfOneTabVisible->setChecked(settings()->value(GROUP(GUI), SETTING(GUI::HideTabBarIfOnlyOneTab)).toBool());

  // Load toolbar button style.
  m_ui->m_spinToolbarIconSize->setValue(settings()->value(GROUP(GUI), SETTING(GUI::ToolbarIconSize)).toInt());
  m_ui->m_cmbToolbarButtonStyle->addItem(tr("Icon only"), Qt::ToolButtonStyle::ToolButtonIconOnly);
  m_ui->m_cmbToolbarButtonStyle->addItem(tr("Text only"), Qt::ToolButtonStyle::ToolButtonTextOnly);
  m_ui->m_cmbToolbarButtonStyle->addItem(tr("Text beside icon"), Qt::ToolButtonStyle::ToolButtonTextBesideIcon);
  m_ui->m_cmbToolbarButtonStyle->addItem(tr("Text under icon"), Qt::ToolButtonStyle::ToolButtonTextUnderIcon);
  m_ui->m_cmbToolbarButtonStyle->addItem(tr("Follow OS style"), Qt::ToolButtonStyle::ToolButtonFollowStyle);
  m_ui->m_cmbToolbarButtonStyle->setCurrentIndex(m_ui->m_cmbToolbarButtonStyle->findData(settings()->value(GROUP(GUI),
                                                                                                           SETTING(
                                                                                                             GUI::ToolbarStyle)).toInt()));

  // Load toolbars.
  m_ui->m_editorFeedsToolbar->loadFromToolBar(qApp->mainForm()->tabWidget()->feedMessageViewer()->feedsToolBar());
  m_ui->m_editorMessagesToolbar->loadFromToolBar(qApp->mainForm()->tabWidget()->feedMessageViewer()->messagesToolBar());
  m_ui->m_editorStatusbar->loadFromToolBar(qApp->mainForm()->statusBar());

  // Load custom colors.
  m_ui->m_gbCustomSkinColors->setChecked(settings()->value(GROUP(CustomSkinColors),
                                                           SETTING(CustomSkinColors::Enabled)).toBool());

  const QMetaObject& mo = SkinEnums::staticMetaObject;
  QMetaEnum enumer = mo.enumerator(mo.indexOfEnumerator(QSL("PaletteColors").toLocal8Bit().constData()));

  for (int i = 0, row = 0; i < enumer.keyCount(); i++, row++) {
    SkinEnums::PaletteColors pal = SkinEnums::PaletteColors(enumer.value(i));

    auto* clr_btn = new ColorToolButton(this);
    auto* rst_btn = new PlainToolButton(this);

    rst_btn->setToolTip(tr("Fetch color from activated skin"));
    rst_btn->setIcon(qApp->icons()->fromTheme(QSL("edit-reset")));

    QColor clr = settings()->value(GROUP(CustomSkinColors), enumer.key(i)).toString();

    if (!clr.isValid()) {
      clr = qApp->skins()->currentSkin().colorForModel(pal).value<QColor>();
    }

    rst_btn->setObjectName(QString::number(enumer.value(i)));

    connect(rst_btn, &PlainToolButton::clicked, this, &SettingsGui::resetCustomSkinColor);
    connect(clr_btn, &ColorToolButton::colorChanged, this, &SettingsGui::dirtifySettings);

    clr_btn->setObjectName(QString::number(enumer.value(i)));
    clr_btn->setColor(clr);

    auto* lay = new QHBoxLayout();

    lay->addWidget(clr_btn);
    lay->addWidget(rst_btn);

    m_ui->m_layoutCustomColors->setWidget(row,
                                          QFormLayout::ItemRole::LabelRole,
                                          new QLabel(
                                            TextFactory::capitalizeFirstLetter(SkinEnums::palleteColorText(SkinEnums::PaletteColors(enumer.value(i)))),
                                            this));
    m_ui->m_layoutCustomColors->setLayout(row,
                                          QFormLayout::ItemRole::FieldRole,
                                          lay);
  }

  onEndLoadSettings();
}

void SettingsGui::resetCustomSkinColor() {
  auto* clr_btn = m_ui->m_gbCustomSkinColors->findChild<ColorToolButton*>(sender()->objectName());
  SkinEnums::PaletteColors pal = SkinEnums::PaletteColors(sender()->objectName().toInt());

  clr_btn->setColor(qApp->skins()->currentSkin().colorForModel(pal, true).value<QColor>());
}

void SettingsGui::saveSettings() {
  onBeginSaveSettings();

  // Save custom skin colors.
  settings()->setValue(GROUP(CustomSkinColors), CustomSkinColors::Enabled, m_ui->m_gbCustomSkinColors->isChecked());

  const QMetaObject& mo = SkinEnums::staticMetaObject;
  QMetaEnum enumer = mo.enumerator(mo.indexOfEnumerator(QSL("PaletteColors").toLocal8Bit().constData()));
  auto children = m_ui->m_gbCustomSkinColors->findChildren<ColorToolButton*>();

  for (const ColorToolButton* clr : children) {
    settings()->setValue(GROUP(CustomSkinColors),
                         enumer.valueToKey(clr->objectName().toInt()),
                         clr->color().name());
  }

  // Save toolbar.
  settings()->setValue(GROUP(GUI), GUI::ToolbarIconSize, m_ui->m_spinToolbarIconSize->value());

  settings()->setValue(GROUP(GUI), GUI::ToolbarStyle,
                       m_ui->m_cmbToolbarButtonStyle->itemData(m_ui->m_cmbToolbarButtonStyle->currentIndex()));

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

  auto old_monochrome = settings()->value(GROUP(GUI), SETTING(GUI::MonochromeTrayIcon)).toBool();

  if (old_monochrome != m_ui->m_checkMonochromeIcons->isChecked()) {
    requireRestart();
    settings()->setValue(GROUP(GUI), GUI::MonochromeTrayIcon, m_ui->m_checkMonochromeIcons->isChecked());
  }

  settings()->setValue(GROUP(GUI), GUI::UnreadNumbersInTrayIcon, m_ui->m_checkCountUnreadMessages->isChecked());
  settings()->setValue(GROUP(GUI), GUI::MainWindowStartsHidden, m_ui->m_checkHidden->isChecked());
  settings()->setValue(GROUP(GUI), GUI::HideMainWindowWhenMinimized, m_ui->m_checkHideWhenMinimized->isChecked());

  settings()->setValue(GROUP(GUI), GUI::ForceDarkFusion, m_ui->m_checkForceDarkFusion->isChecked());

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

  qApp->feedReader()->feedsModel()->reloadWholeLayout();
  qApp->feedReader()->messagesModel()->reloadWholeLayout();

  onEndSaveSettings();
}
