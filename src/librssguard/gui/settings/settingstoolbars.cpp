// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/settings/settingstoolbars.h"

#include "gui/dialogs/formmain.h"
#include "gui/feedmessageviewer.h"
#include "gui/systemtrayicon.h"
#include "gui/tabwidget.h"
#include "gui/toolbars/feedstoolbar.h"
#include "gui/toolbars/messagestoolbar.h"
#include "gui/toolbars/statusbar.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/settings.h"

#include <QDropEvent>
#include <QFontDialog>
#include <QStyleFactory>

SettingsToolbars::SettingsToolbars(Settings* settings, QWidget* parent)
  : SettingsPanel(settings, parent), m_ui(nullptr) {}

SettingsToolbars::~SettingsToolbars() {
  if (m_ui != nullptr) {
    delete m_ui;
  }
}

void SettingsToolbars::loadUi() {
  m_ui = new Ui::SettingsToolbars();
  m_ui->setupUi(this);
  m_ui->m_editorMessagesToolbar->activeItemsWidget()->viewport()->installEventFilter(this);
  m_ui->m_editorFeedsToolbar->activeItemsWidget()->viewport()->installEventFilter(this);
  m_ui->m_editorMessagesToolbar->availableItemsWidget()->viewport()->installEventFilter(this);
  m_ui->m_editorFeedsToolbar->availableItemsWidget()->viewport()->installEventFilter(this);

  connect(m_ui->m_cmbToolbarButtonStyle,
          static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
          this,
          &SettingsToolbars::dirtifySettings);
  connect(m_ui->m_editorFeedsToolbar, &ToolBarEditor::setupChanged, this, &SettingsToolbars::dirtifySettings);
  connect(m_ui->m_editorMessagesToolbar, &ToolBarEditor::setupChanged, this, &SettingsToolbars::dirtifySettings);
  connect(m_ui->m_editorStatusbar, &ToolBarEditor::setupChanged, this, &SettingsToolbars::dirtifySettings);
  connect(m_ui->m_editorStatusbar, &ToolBarEditor::setupChanged, this, &SettingsToolbars::requireRestart);

  connect(m_ui->m_cmbSelectToolBar,
          static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
          m_ui->m_stackedToolbars,
          &QStackedWidget::setCurrentIndex);
  connect(m_ui->m_spinToolbarIconSize,
          QOverload<int>::of(&QSpinBox::valueChanged),
          this,
          &SettingsToolbars::dirtifySettings);

  connect(m_ui->m_spinToolbarIconSize, QOverload<int>::of(&QSpinBox::valueChanged), this, [=](int value) {
    if (value <= 0) {
      m_ui->m_spinToolbarIconSize->setSuffix(tr(" = default icon size"));
    }
    else {
      m_ui->m_spinToolbarIconSize->setSuffix(QSL(" px"));
    }
  });

  SettingsPanel::loadUi();
}

QIcon SettingsToolbars::icon() const {
  return qApp->icons()->fromTheme(QSL("view-list-details"), QSL("draw-freehand"));
}

bool SettingsToolbars::eventFilter(QObject* obj, QEvent* e) {
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

void SettingsToolbars::loadSettings() {
  onBeginLoadSettings();

  // Load toolbar button style.
  m_ui->m_spinToolbarIconSize->setValue(settings()->value(GROUP(GUI), SETTING(GUI::ToolbarIconSize)).toInt());
  m_ui->m_cmbToolbarButtonStyle->addItem(tr("Icon only"), Qt::ToolButtonStyle::ToolButtonIconOnly);
  m_ui->m_cmbToolbarButtonStyle->addItem(tr("Text only"), Qt::ToolButtonStyle::ToolButtonTextOnly);
  m_ui->m_cmbToolbarButtonStyle->addItem(tr("Text beside icon"), Qt::ToolButtonStyle::ToolButtonTextBesideIcon);
  m_ui->m_cmbToolbarButtonStyle->addItem(tr("Text under icon"), Qt::ToolButtonStyle::ToolButtonTextUnderIcon);
  m_ui->m_cmbToolbarButtonStyle->addItem(tr("Follow OS style"), Qt::ToolButtonStyle::ToolButtonFollowStyle);
  m_ui->m_cmbToolbarButtonStyle
    ->setCurrentIndex(m_ui->m_cmbToolbarButtonStyle
                        ->findData(settings()->value(GROUP(GUI), SETTING(GUI::ToolbarStyle)).toInt()));

  // Load toolbars.
  m_ui->m_editorFeedsToolbar->loadFromToolBar(qApp->mainForm()->tabWidget()->feedMessageViewer()->feedsToolBar());
  m_ui->m_editorMessagesToolbar->loadFromToolBar(qApp->mainForm()->tabWidget()->feedMessageViewer()->messagesToolBar());
  m_ui->m_editorStatusbar->loadFromToolBar(qApp->mainForm()->statusBar());

  onEndLoadSettings();
}

void SettingsToolbars::saveSettings() {
  onBeginSaveSettings();

  // Save toolbar.
  settings()->setValue(GROUP(GUI), GUI::ToolbarIconSize, m_ui->m_spinToolbarIconSize->value());
  settings()->setValue(GROUP(GUI),
                       GUI::ToolbarStyle,
                       m_ui->m_cmbToolbarButtonStyle->itemData(m_ui->m_cmbToolbarButtonStyle->currentIndex()));

  m_ui->m_editorFeedsToolbar->saveToolBar();
  m_ui->m_editorMessagesToolbar->saveToolBar();
  m_ui->m_editorStatusbar->saveToolBar();

  qApp->mainForm()->tabWidget()->feedMessageViewer()->normalizeToolbarHeights();
  qApp->mainForm()->tabWidget()->feedMessageViewer()->refreshVisualProperties();

  onEndSaveSettings();
}
