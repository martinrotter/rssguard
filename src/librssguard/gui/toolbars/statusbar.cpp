// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/toolbars/statusbar.h"

#include "gui/dialogs/formmain.h"
#include "gui/reusable/plaintoolbutton.h"
#include "gui/reusable/progressbarwithtext.h"
#include "gui/tabwidget.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/mutex.h"

#include <QLabel>
#include <QToolButton>

StatusBar::StatusBar(QWidget* parent) : QStatusBar(parent) {
  setSizeGripEnabled(false);
  setContentsMargins(2, 0, 2, 2);

  m_barProgressFeeds = new ProgressBarWithText(this);
  m_barProgressFeeds->setTextVisible(true);
  m_barProgressFeeds->setFixedWidth(250);
  m_barProgressFeeds->setVisible(false);
  m_barProgressFeeds->setObjectName(QSL("m_barProgressFeeds"));

  m_barProgressFeedsAction = new QAction(qApp->icons()->fromTheme(QSL("application-rss+xml")), tr("Feed update progress bar"), this);
  m_barProgressFeedsAction->setObjectName(QSL("m_barProgressFeedsAction"));

  m_barProgressDownload = new ProgressBarWithText(this);
  m_barProgressDownload->setTextVisible(true);
  m_barProgressDownload->setFixedWidth(230);
  m_barProgressDownload->setVisible(false);
  m_barProgressDownload->setObjectName(QSL("m_barProgressDownload"));

  m_barProgressDownloadAction = new QAction(qApp->icons()->fromTheme(QSL("emblem-downloads")), tr("File download progress bar"), this);
  m_barProgressDownloadAction->setObjectName(QSL("m_barProgressDownloadAction"));

  m_barProgressDownload->installEventFilter(this);
}

StatusBar::~StatusBar() {
  clear();
  qDebugNN << LOGSEC_GUI "Destroying StatusBar instance.";
}

QList<QAction*> StatusBar::availableActions() const {
  QList<QAction*> actions = qApp->userActions();

  // Now, add placeholder actions for custom stuff.
  actions << m_barProgressDownloadAction
          << m_barProgressFeedsAction;

  return actions;
}

QList<QAction*> StatusBar::activatedActions() const {
  return actions();
}

void StatusBar::saveAndSetActions(const QStringList& actions) {
  qApp->settings()->setValue(GROUP(GUI), GUI::StatusbarActions, actions.join(QSL(",")));
  loadSpecificActions(convertActions(actions));
}

QStringList StatusBar::defaultActions() const {
  return QString(GUI::StatusbarActionsDef).split(',',
#if QT_VERSION >= 0x050F00 // Qt >= 5.15.0
                                                 Qt::SplitBehaviorFlags::SkipEmptyParts);
#else
                                                 QString::SplitBehavior::SkipEmptyParts);
#endif
}

QStringList StatusBar::savedActions() const {
  return qApp->settings()->value(GROUP(GUI),
                                 SETTING(GUI::StatusbarActions)).toString().split(',',
#if QT_VERSION >= 0x050F00 // Qt >= 5.15.0
                                                                                  Qt::SplitBehaviorFlags::SkipEmptyParts);
#else
                                                                                  QString::SplitBehavior::SkipEmptyParts);
#endif
}

QList<QAction*> StatusBar::convertActions(const QStringList& actions) {
  bool progress_visible = this->actions().contains(m_barProgressFeedsAction) && m_barProgressFeeds->isVisible();
  QList<QAction*> available_actions = availableActions();
  QList<QAction*> spec_actions;

  // Iterate action names and add respectable
  // actions into the toolbar.
  for (const QString& action_name : actions) {
    QAction* matching_action = findMatchingAction(action_name, available_actions);
    QAction* action_to_add;
    QWidget* widget_to_add;

    if (matching_action == m_barProgressDownloadAction) {
      widget_to_add = m_barProgressDownload;
      action_to_add = m_barProgressDownloadAction;
      widget_to_add->setVisible(false);
    }
    else if (matching_action == m_barProgressFeedsAction) {
      widget_to_add = m_barProgressFeeds;
      action_to_add = m_barProgressFeedsAction;
      widget_to_add->setVisible(progress_visible);
    }
    else {
      if (action_name == QSL(SEPARATOR_ACTION_NAME)) {
        QLabel* lbl = new QLabel(QSL("•"), this);

        widget_to_add = lbl;
        action_to_add = new QAction(this);
        action_to_add->setSeparator(true);
      }
      else if (action_name == QSL(SPACER_ACTION_NAME)) {
        QLabel* lbl = new QLabel(QSL("\t\t"), this);

        widget_to_add = lbl;
        action_to_add = new QAction(this);
        action_to_add->setIcon(qApp->icons()->fromTheme(QSL("system-search")));
        action_to_add->setProperty("type", SPACER_ACTION_NAME);
        action_to_add->setProperty("name", tr("Toolbar spacer"));
      }
      else if (matching_action != nullptr) {
        // Add originally toolbar action.
        auto* tool_button = new PlainToolButton(this);

        tool_button->reactOnActionChange(matching_action);
        widget_to_add = tool_button;
        action_to_add = matching_action;
        connect(tool_button, &PlainToolButton::clicked, matching_action, &QAction::trigger);
        connect(matching_action, &QAction::changed, tool_button, &PlainToolButton::reactOnSenderActionChange);
      }
      else {
        action_to_add = nullptr;
        widget_to_add = nullptr;
      }
    }

    if (action_to_add != nullptr && widget_to_add != nullptr) {
      action_to_add->setProperty("widget", QVariant::fromValue(widget_to_add));
      spec_actions.append(action_to_add);
    }
  }

  return spec_actions;
}

void StatusBar::loadSpecificActions(const QList<QAction*>& actions, bool initial_load) {
  if (initial_load) {
    clear();

    for (QAction* act : actions) {
      QWidget* widget = act->property("widget").isValid() ? qvariant_cast<QWidget*>(act->property("widget")) : nullptr;

      addAction(act);

      // And also add widget.
      if (widget != nullptr) {
        addPermanentWidget(widget);
      }
    }
  }
}

bool StatusBar::eventFilter(QObject* watched, QEvent* event) {
  if (watched == m_barProgressDownload) {
    if (event->type() == QEvent::Type::MouseButtonPress) {
      qApp->mainForm()->tabWidget()->showDownloadManager();
    }
  }

  return false;
}

void StatusBar::clear() {
  while (!actions().isEmpty()) {
    QAction* act = actions().at(0);
    QWidget* widget = act->property("widget").isValid() ? static_cast<QWidget*>(act->property("widget").value<void*>()) : nullptr;

    if (widget != nullptr) {
      removeWidget(widget);

      widget->setParent(qApp->mainFormWidget());
      widget->setVisible(false);
    }

    removeAction(act);
  }
}

void StatusBar::showProgressFeeds(int progress, const QString& label) {
  if (actions().contains(m_barProgressFeedsAction)) {
    m_barProgressFeeds->setVisible(true);
    m_barProgressFeeds->setFormat(label);

    if (progress < 0) {
      m_barProgressFeeds->setRange(0, 0);
    }
    else {
      m_barProgressFeeds->setRange(0, 100);
      m_barProgressFeeds->setValue(progress);
    }
  }
}

void StatusBar::clearProgressFeeds() {
  m_barProgressFeeds->setVisible(false);
}

void StatusBar::showProgressDownload(int progress, const QString& tooltip) {
  if (actions().contains(m_barProgressDownloadAction)) {
    m_barProgressDownload->setVisible(true);
    m_barProgressDownload->setFormat(tooltip);
    m_barProgressDownload->setToolTip(tooltip);

    if (progress < 0) {
      m_barProgressDownload->setRange(0, 0);
    }
    else {
      m_barProgressDownload->setRange(0, 100);
      m_barProgressDownload->setValue(progress);
    }
  }
}

void StatusBar::clearProgressDownload() {
  m_barProgressDownload->setVisible(false);
  m_barProgressDownload->setValue(0);
}
