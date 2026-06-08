// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/dialogs/formsettings.h"

#include "definitions/definitions.h"
#include "gui/guiutilities.h"
#include "gui/messagebox.h"
#include "gui/settings/settingsbrowsermail.h"
#include "gui/settings/settingsdatabase.h"
#include "gui/settings/settingsfeedsmessages.h"
#include "gui/settings/settingsgeneral.h"
#include "gui/settings/settingsgui.h"
#include "gui/settings/settingslocalization.h"
#include "gui/settings/settingsmediaplayer.h"
#include "gui/settings/settingsnetwork.h"
#include "gui/settings/settingsnotifications.h"
#include "gui/settings/settingsshortcuts.h"
#include "gui/settings/settingstoolbars.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/settings.h"

#include <algorithm>

#include <QAbstractButton>
#include <QComboBox>
#include <QEvent>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPointer>
#include <QRegularExpression>
#include <QScrollArea>
#include <QScrollBar>
#include <QSet>
#include <QTabWidget>
#include <QTimer>
#include <QVariant>

namespace {
  class SettingsSearchHighlight : public QWidget {
    public:
      explicit SettingsSearchHighlight(QWidget* target)
        : QWidget(target != nullptr && target->parentWidget() != nullptr ? target->parentWidget() : target),
          m_target(target) {
        setAttribute(Qt::WidgetAttribute::WA_TransparentForMouseEvents);
        setAttribute(Qt::WidgetAttribute::WA_NoSystemBackground);
        setAttribute(Qt::WidgetAttribute::WA_TranslucentBackground);

        updateGeometryFromTarget();
      }

      virtual bool eventFilter(QObject* watched, QEvent* event) {
        if ((watched == m_target.data() || watched == parentWidget()) &&
            (event->type() == QEvent::Type::Resize || event->type() == QEvent::Type::Move ||
             event->type() == QEvent::Type::Show)) {
          updateGeometryFromTarget();
          update();
        }

        return QWidget::eventFilter(watched, event);
      }

      void updateGeometryFromTarget() {
        if (m_target == nullptr) {
          return;
        }

        if (parentWidget() == m_target) {
          setGeometry(m_target->rect());
        }
        else {
          setGeometry(m_target->geometry().adjusted(-3, -2, 3, 2));
        }
      }

    protected:
      virtual void paintEvent(QPaintEvent* event) {
        Q_UNUSED(event)

        QPainter painter(this);
        painter.setRenderHint(QPainter::RenderHint::Antialiasing);

        QColor fill = palette().highlight().color();
        QColor stroke = fill;

        fill.setAlpha(42);
        stroke.setAlpha(170);

        painter.setBrush(fill);
        painter.setPen(QPen(stroke, 1));

        QRect rect = this->rect().adjusted(1, 1, -1, -1);
        painter.drawRoundedRect(rect, 2, 2);
      }

    private:
      QPointer<QWidget> m_target;
  };

  SettingsSearchHighlight* searchHighlightFor(QWidget* widget) {
    if (widget == nullptr) {
      return nullptr;
    }

    auto* highlight =
      dynamic_cast<SettingsSearchHighlight*>(widget->property("settingsSearchHighlight").value<QObject*>());

    if (highlight == nullptr) {
      highlight = new SettingsSearchHighlight(widget);
      highlight->setObjectName(QSL("settingsSearchHighlight"));
      widget->setProperty("settingsSearchHighlight", QVariant::fromValue<QObject*>(highlight));
      widget->installEventFilter(highlight);

      if (widget->parentWidget() != nullptr) {
        widget->parentWidget()->installEventFilter(highlight);
      }
    }

    highlight->updateGeometryFromTarget();
    highlight->raise();
    highlight->show();
    highlight->update();

    return highlight;
  }

  void clearSearchHighlight(QWidget* widget) {
    if (widget == nullptr) {
      return;
    }

    auto* highlight =
      dynamic_cast<SettingsSearchHighlight*>(widget->property("settingsSearchHighlight").value<QObject*>());

    if (highlight != nullptr) {
      widget->removeEventFilter(highlight);

      if (widget->parentWidget() != nullptr) {
        widget->parentWidget()->removeEventFilter(highlight);
      }

      widget->setProperty("settingsSearchHighlight", QVariant());
      highlight->deleteLater();
    }
  }
} // namespace

FormSettings::FormSettings(QWidget& parent)
  : QDialog(&parent), m_searchIndexBuilt(false), m_settings(*qApp->settings()) {
  m_ui.setupUi(this);

  // Set flags and attributes.
  GuiUtilities::applyDialogProperties(*this,
                                      qApp->icons()->fromTheme(QSL("emblem-system"), QSL("applications-system")));

  m_btnApply = m_ui.m_buttonBox->button(QDialogButtonBox::StandardButton::Apply);

  m_btnApply->setEnabled(false);

  // Establish needed connections.
  connect(m_ui.m_buttonBox, &QDialogButtonBox::accepted, this, &FormSettings::saveSettings);
  connect(m_ui.m_buttonBox, &QDialogButtonBox::rejected, this, &FormSettings::cancelSettings);

  connect(m_btnApply, &QPushButton::clicked, this, &FormSettings::applySettings);
  connect(m_ui.m_listSettings, &QListWidget::currentRowChanged, this, &FormSettings::openSettingsCategory);
  connect(m_ui.m_txtSearchSettings, &QLineEdit::textChanged, this, &FormSettings::searchSettings);

  addSettingsPanel(new SettingsGeneral(&m_settings, this));
  addSettingsPanel(new SettingsLocalization(&m_settings, this));
  addSettingsPanel(new SettingsDatabase(&m_settings, this));
  addSettingsPanel(new SettingsNetwork(&m_settings, this));
  addSettingsPanel(new SettingsFeedsMessages(&m_settings, this));
  addSettingsPanel(new SettingsNotifications(&m_settings, this));
  addSettingsPanel(new SettingsGui(&m_settings, this));
  addSettingsPanel(new SettingsToolbars(&m_settings, this));
  addSettingsPanel(new SettingsShortcuts(&m_settings, this));
  addSettingsPanel(new SettingsBrowserMail(&m_settings, this));
  addSettingsPanel(new SettingsMediaPlayer(&m_settings, this));

  m_ui.m_listSettings->setMaximumWidth(m_ui.m_listSettings->sizeHintForColumn(0) +
                                       6 * m_ui.m_listSettings->frameWidth());
  m_ui.m_listSettings->setCurrentRow(0);
}

FormSettings::~FormSettings() {
  qDebugNN << LOGSEC_GUI << "Destroying FormSettings distance.";
}

void FormSettings::reject() {
  m_ui.m_buttonBox->button(QDialogButtonBox::StandardButton::Cancel)->click();
}

void FormSettings::openSettingsCategory(int category) {
  if (category >= 0 && category < m_panels.size()) {
    if (!m_panels.at(category)->uiLoaded()) {
      m_panels.at(category)->loadUi();
    }

    if (!m_panels.at(category)->isLoaded()) {
      m_panels.at(category)->loadSettings();
    }
  }

  m_ui.m_stackedSettings->setCurrentIndex(category);
  updateSearchHighlights();
}

void FormSettings::searchSettings(const QString& phrase) {
  const QString normalized_phrase = normalizedSearchText(phrase);

  if (normalized_phrase.isEmpty()) {
    clearSearch();
    return;
  }

  ensureSearchIndexBuilt();

  const QStringList terms = normalized_phrase.split(QL1C(' '), SPLIT_BEHAVIOR::SkipEmptyParts);
  QSet<SettingsPanel*> matched_panels;

  m_searchResults.clear();

  for (int i = 0; i < m_searchIndex.size(); ++i) {
    const SettingsSearchEntry& entry = m_searchIndex.at(i);

    if (entry.m_panel.isNull() || entry.m_widget.isNull()) {
      continue;
    }

    const bool matches = std::all_of(terms.cbegin(), terms.cend(), [&entry](const QString& term) {
      return entry.m_text.contains(term);
    });

    if (matches) {
      m_searchResults.append(i);
      matched_panels.insert(entry.m_panel.data());
    }
  }

  for (int i = 0; i < m_panels.size(); ++i) {
    m_ui.m_listSettings->item(i)->setHidden(!matched_panels.contains(m_panels.at(i)));
  }

  applyFirstSearchResult();
  updateSearchHighlights();
}

void FormSettings::saveSettings() {
  applySettings();
  accept();
}

void FormSettings::applySettings() {
  // Save all settings.
  m_settings.checkSettings();
  QStringList panels_for_restart;

  for (SettingsPanel* panel : std::as_const(m_panels)) {
    if (panel->isDirty() && panel->isLoaded() && panel->uiLoaded()) {
      panel->saveSettings();
    }

    if (panel->requiresRestart()) {
      panels_for_restart.append(panel->title().toLower());
      panel->setRequiresRestart(false);
    }
  }

  if (!panels_for_restart.isEmpty()) {
    const QStringList changed_settings_description =
      panels_for_restart.replaceInStrings(QRegularExpression(QSL("^")), QSL(" \u2022 "));
    MsgBox::show(this,
                 QMessageBox::Icon::Question,
                 tr("Critical settings were changed"),
                 tr("Some critical settings were changed and will be applied after the application gets restarted. "
                    "\n\nYou have to restart manually."),
                 {},
                 tr("Changed categories of settings:\n%1.").arg(changed_settings_description.join(QSL(",\n"))));
  }

  m_btnApply->setEnabled(false);
}

void FormSettings::cancelSettings() {
  QStringList changed_panels;

  for (SettingsPanel* panel : std::as_const(m_panels)) {
    if (panel->isLoaded() && panel->isDirty()) {
      changed_panels.append(panel->title().toLower());
    }
  }

  if (changed_panels.isEmpty()) {
    done(QDialog::DialogCode::Rejected);
  }
  else {
    const QStringList changed_settings_description =
      changed_panels.replaceInStrings(QRegularExpression(QSL("^")), QSL(" \u2022 "));

    if (MsgBox::show(this,
                     QMessageBox::Icon::Critical,
                     tr("Some settings are changed and will be lost"),
                     tr("Some settings were changed and by cancelling this dialog, you would lose these changes."),
                     tr("Do you really want to close this dialog without saving any settings?"),
                     tr("Changed categories of settings:\n%1.").arg(changed_settings_description.join(QSL(",\n"))),
                     QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No,
                     QMessageBox::StandardButton::Yes) == QMessageBox::StandardButton::Yes) {
      done(QDialog::DialogCode::Rejected);
    }
  }
}

void FormSettings::addSettingsPanel(SettingsPanel* panel) {
  QListWidgetItem* itm = new QListWidgetItem(m_ui.m_listSettings);

  itm->setText(panel->title());
  itm->setIcon(panel->icon());

  m_panels.append(panel);

  QScrollArea* scr = new QScrollArea(m_ui.m_stackedSettings);

  scr->setWidgetResizable(true);
  scr->setFrameShape(QFrame::Shape::StyledPanel);
  scr->setWidget(panel);

  m_ui.m_stackedSettings->addWidget(scr);

  connect(scr->verticalScrollBar(), &QScrollBar::valueChanged, this, [this]() {
    updateSearchHighlights();
  });
  connect(scr->horizontalScrollBar(), &QScrollBar::valueChanged, this, [this]() {
    updateSearchHighlights();
  });

  connect(panel, &SettingsPanel::settingsChanged, this, [this]() {
    m_btnApply->setEnabled(true);
  });
}

void FormSettings::ensureSearchIndexBuilt() {
  if (m_searchIndexBuilt) {
    return;
  }

  for (SettingsPanel* panel : std::as_const(m_panels)) {
    if (!panel->uiLoaded()) {
      panel->loadUi();
    }

    if (!panel->isLoaded()) {
      panel->loadSettings();
    }

    indexSettingsPanel(panel);
  }

  m_searchIndexBuilt = true;
}

void FormSettings::indexSettingsPanel(SettingsPanel* panel) {
  if (panel == nullptr) {
    return;
  }

  SettingsSearchEntry panel_entry;
  panel_entry.m_text = normalizedSearchText(panel->title());
  panel_entry.m_panel = panel;
  panel_entry.m_widget = panel;

  if (!panel_entry.m_text.isEmpty()) {
    m_searchIndex.append(panel_entry);
  }

  indexWidget(panel, panel, {});
}

void FormSettings::indexWidget(QWidget* widget, SettingsPanel* panel, QList<SettingsSearchTab> tab_path) {
  if (widget == nullptr || panel == nullptr) {
    return;
  }

  if (auto* tab_widget = qobject_cast<QTabWidget*>(widget); tab_widget != nullptr) {
    if (!tab_widget->property("settingsSearchConnected").toBool()) {
      tab_widget->setProperty("settingsSearchConnected", true);
      connect(tab_widget, &QTabWidget::currentChanged, this, [this]() {
        updateSearchHighlights();
      });
    }

    for (int i = 0; i < tab_widget->count(); ++i) {
      SettingsSearchEntry tab_entry;
      tab_entry.m_text = normalizedSearchText(tab_widget->tabText(i) + QL1C(' ') + tab_widget->tabToolTip(i));
      tab_entry.m_panel = panel;
      tab_entry.m_widget = tab_widget->widget(i);
      tab_entry.m_tabPath = tab_path;
      tab_entry.m_tabPath.append({tab_widget, i});

      if (!tab_entry.m_text.isEmpty()) {
        m_searchIndex.append(tab_entry);
      }
    }
  }

  const QString widget_text = normalizedSearchText(searchableWidgetText(widget));

  if (!widget_text.isEmpty()) {
    SettingsSearchEntry entry;
    entry.m_text = widget_text;
    entry.m_panel = panel;
    entry.m_widget = widget;
    entry.m_tabPath = tab_path;
    m_searchIndex.append(entry);
  }

  if (auto* combo = qobject_cast<QComboBox*>(widget); combo != nullptr) {
    QStringList items;

    for (int i = 0; i < combo->count(); ++i) {
      items.append(combo->itemText(i));
    }

    const QString combo_items = normalizedSearchText(items.join(QL1C(' ')));

    if (!combo_items.isEmpty()) {
      SettingsSearchEntry entry;
      entry.m_text = combo_items;
      entry.m_panel = panel;
      entry.m_widget = combo;
      entry.m_tabPath = tab_path;
      m_searchIndex.append(entry);
    }
  }

  const auto children = widget->findChildren<QWidget*>(QString(), Qt::FindChildOption::FindDirectChildrenOnly);

  for (QWidget* child : children) {
    QList<SettingsSearchTab> child_tab_path = tab_path;

    if (auto* tab_widget = qobject_cast<QTabWidget*>(widget); tab_widget != nullptr) {
      const int tab_index = tab_widget->indexOf(child);

      if (tab_index >= 0) {
        child_tab_path.append({tab_widget, tab_index});
      }
    }

    indexWidget(child, panel, child_tab_path);
  }
}

void FormSettings::applyFirstSearchResult() {
  if (m_searchResults.isEmpty()) {
    return;
  }

  const SettingsSearchEntry& entry = m_searchIndex.at(m_searchResults.first());

  if (entry.m_panel.isNull() || entry.m_widget.isNull()) {
    return;
  }

  const int panel_index = m_panels.indexOf(entry.m_panel.data());

  if (panel_index < 0) {
    return;
  }

  if (m_ui.m_listSettings->currentRow() != panel_index) {
    m_ui.m_listSettings->setCurrentRow(panel_index);
  }

  m_ui.m_stackedSettings->setCurrentIndex(panel_index);

  for (const SettingsSearchTab& tab : entry.m_tabPath) {
    if (!tab.m_tabWidget.isNull() && tab.m_tabIndex >= 0 && tab.m_tabIndex < tab.m_tabWidget->count()) {
      tab.m_tabWidget->setCurrentIndex(tab.m_tabIndex);
    }
  }

  auto* scroll_area = qobject_cast<QScrollArea*>(m_ui.m_stackedSettings->currentWidget());

  if (scroll_area != nullptr) {
    QTimer::singleShot(0, this, [scroll_area, widget = entry.m_widget]() {
      if (!widget.isNull()) {
        scroll_area->ensureWidgetVisible(widget.data(), 20, 20);
      }
    });
  }
}

void FormSettings::updateSearchHighlights() {
  for (const QPointer<QWidget>& widget : std::as_const(m_highlightedWidgets)) {
    if (!widget.isNull()) {
      clearSearchHighlight(widget.data());
    }
  }

  m_highlightedWidgets.clear();

  if (m_ui.m_txtSearchSettings->text().simplified().isEmpty() || m_searchResults.isEmpty()) {
    return;
  }

  auto* scroll_area = qobject_cast<QScrollArea*>(m_ui.m_stackedSettings->currentWidget());

  if (scroll_area == nullptr) {
    return;
  }

  auto* current_panel = qobject_cast<SettingsPanel*>(scroll_area->widget());

  for (int result : std::as_const(m_searchResults)) {
    const SettingsSearchEntry& entry = m_searchIndex.at(result);

    if (!entry.m_panel.isNull() && entry.m_panel.data() == current_panel && !entry.m_widget.isNull() &&
        entry.m_widget.data() != current_panel) {
      bool is_on_current_tab_path = true;

      for (const SettingsSearchTab& tab : entry.m_tabPath) {
        if (tab.m_tabWidget.isNull() || tab.m_tabWidget->currentIndex() != tab.m_tabIndex) {
          is_on_current_tab_path = false;
          break;
        }
      }

      if (is_on_current_tab_path) {
        searchHighlightFor(entry.m_widget.data());
        m_highlightedWidgets.append(entry.m_widget);
      }
    }
  }
}

void FormSettings::clearSearch() {
  m_searchResults.clear();

  for (int i = 0; i < m_ui.m_listSettings->count(); ++i) {
    m_ui.m_listSettings->item(i)->setHidden(false);
  }

  updateSearchHighlights();

  if (m_ui.m_stackedSettings->currentIndex() < 0 && !m_panels.isEmpty()) {
    m_ui.m_listSettings->setCurrentRow(0);
  }
}

QString FormSettings::normalizedSearchText(const QString& text) {
  QString result = text;

  result.remove(QRegularExpression(QSL("<[^>]*>")));
  result.replace(QL1C('&'), QL1S(""));
  result = result.toLower().simplified();

  return result;
}

QString FormSettings::searchableWidgetText(QWidget* widget) {
  QStringList texts;

  if (auto* label = qobject_cast<QLabel*>(widget); label != nullptr) {
    texts.append(label->text());
  }
  else if (auto* button = qobject_cast<QAbstractButton*>(widget); button != nullptr) {
    texts.append(button->text());
  }
  else if (auto* group_box = qobject_cast<QGroupBox*>(widget); group_box != nullptr) {
    texts.append(group_box->title());
  }
  else if (auto* line_edit = qobject_cast<QLineEdit*>(widget); line_edit != nullptr) {
    texts.append(line_edit->placeholderText());
  }

  texts.append(widget->toolTip());
  texts.append(widget->whatsThis());
  texts.append(widget->statusTip());

  return texts.join(QL1C(' '));
}
