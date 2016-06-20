// This file is part of RSS Guard.
//
// Copyright (C) 2011-2016 by Martin Rotter <rotter.martinos@gmail.com>
//
// RSS Guard is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// RSS Guard is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with RSS Guard. If not, see <http://www.gnu.org/licenses/>.

#include "gui/statusbar.h"

#include "gui/dialogs/formmain.h"
#include "gui/tabwidget.h"
#include "gui/plaintoolbutton.h"
#include "miscellaneous/iconfactory.h"

#include <QToolButton>
#include <QLabel>
#include <QProgressBar>
#include <QThread>


StatusBar::StatusBar(QWidget *parent) : QStatusBar(parent) {
  setSizeGripEnabled(false);
  setContentsMargins(2, 0, 2, 2);

  m_barProgressFeeds = new QProgressBar(this);
  m_barProgressFeeds->setTextVisible(false);
  m_barProgressFeeds->setFixedWidth(100);
  m_barProgressFeeds->setVisible(false);
  m_barProgressFeeds->setObjectName(QSL("m_barProgressFeeds"));

  m_barProgressFeedsAction = new QAction(qApp->icons()->fromTheme(QSL("application-rss+xml")), tr("Feed update progress bar"), this);
  m_barProgressFeedsAction->setObjectName(QSL("m_barProgressFeedsAction"));

  m_lblProgressFeeds = new QLabel(this);
  m_lblProgressFeeds->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  m_lblProgressFeeds->setVisible(false);
  m_lblProgressFeeds->setObjectName(QSL("m_lblProgressFeeds"));

  m_lblProgressFeedsAction = new QAction(qApp->icons()->fromTheme(QSL("application-rss+xml")), tr("Feed update label"), this);
  m_lblProgressFeedsAction->setObjectName(QSL("m_lblProgressFeedsAction"));

  m_barProgressDownload = new QProgressBar(this);
  m_barProgressDownload->setTextVisible(true);
  m_barProgressDownload->setFixedWidth(100);
  m_barProgressDownload->setVisible(false);
  m_barProgressDownload->setObjectName(QSL("m_barProgressDownload"));

  m_barProgressDownloadAction = new QAction(qApp->icons()->fromTheme(QSL("emblem-downloads")), tr("File download progress bar"), this);
  m_barProgressDownloadAction->setObjectName(QSL("m_barProgressDownloadAction"));

  m_lblProgressDownload = new QLabel(this);
  m_lblProgressDownload->setText("Downloading files in background");
  m_lblProgressDownload->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  m_lblProgressDownload->setVisible(false);
  m_lblProgressDownload->setObjectName(QSL("m_lblProgressDownload"));

  m_lblProgressDownloadAction = new QAction(qApp->icons()->fromTheme(QSL("emblem-downloads")), tr("File download label"), this);
  m_lblProgressDownloadAction->setObjectName(QSL("m_lblProgressDownloadAction"));

  m_lblProgressDownload->installEventFilter(this);
  m_barProgressDownload->installEventFilter(this);
}

StatusBar::~StatusBar() {
  clear();
  qDebug("Destroying StatusBar instance.");
}

QList<QAction*> StatusBar::availableActions() const {
  QList<QAction*> actions = qApp->userActions();

  // Now, add placeholder actions for custom stuff.
  actions << m_barProgressDownloadAction << m_barProgressFeedsAction <<
             m_lblProgressDownloadAction << m_lblProgressFeedsAction;

  return actions;
}

QList<QAction*> StatusBar::changeableActions() const {
  return actions();
}

void StatusBar::saveChangeableActions(const QStringList &actions) {
  qApp->settings()->setValue(GROUP(GUI), GUI::StatusbarActions, actions.join(QSL(",")));
  loadChangeableActions(actions);
}

void StatusBar::loadChangeableActions() {
  QStringList action_names = qApp->settings()->value(GROUP(GUI), SETTING(GUI::StatusbarActions)).toString().split(',',
                                                                                                                  QString::SkipEmptyParts);

  loadChangeableActions(action_names);
}

void StatusBar::loadChangeableActions(const QStringList &action_names) {
  clear();

  QList<QAction*> available_actions = availableActions();

  // Iterate action names and add respectable actions into the toolbar.
  foreach (const QString &action_name, action_names) {
    QAction *matching_action = findMatchingAction(action_name, available_actions);
    QAction *action_to_add;
    QWidget *widget_to_add;

    if (matching_action == m_barProgressDownloadAction) {
      widget_to_add = m_barProgressDownload;
      action_to_add = m_barProgressDownloadAction;

      widget_to_add->setVisible(false);
    }
    else if (matching_action == m_barProgressFeedsAction) {
      widget_to_add = m_barProgressFeeds;
      action_to_add = m_barProgressFeedsAction;

      widget_to_add->setVisible(false);
    }
    else if (matching_action == m_lblProgressDownloadAction) {
      widget_to_add = m_lblProgressDownload;
      action_to_add = m_lblProgressDownloadAction;

      widget_to_add->setVisible(false);
    }
    else if (matching_action == m_lblProgressFeedsAction) {
      widget_to_add = m_lblProgressFeeds;
      action_to_add = m_lblProgressFeedsAction;

      widget_to_add->setVisible(false);
    }
    else {
      if (action_name == SEPARATOR_ACTION_NAME) {
        QLabel *lbl = new QLabel(QString::fromUtf8("•"));
        widget_to_add = lbl;

        action_to_add = new QAction(this);
        action_to_add->setSeparator(true);
        action_to_add->setProperty("should_remove_action", true);
      }
      else if (action_name == SPACER_ACTION_NAME) {
        QLabel *lbl = new QLabel(QSL("\t\t"));
        widget_to_add = lbl;

        action_to_add = new QAction(this);
        action_to_add->setProperty("should_remove_action", true);
        action_to_add->setIcon(qApp->icons()->fromTheme(QSL("system-search")));
        action_to_add->setProperty("type", SPACER_ACTION_NAME);
        action_to_add->setProperty("name", tr("Toolbar spacer"));
      }
      else if (matching_action != nullptr) {
        // Add originally toolbar action.
        PlainToolButton *tool_button = new PlainToolButton(this);
        tool_button->reactOnActionChange(matching_action);

        widget_to_add = tool_button;
        action_to_add = matching_action;

        connect(tool_button, SIGNAL(clicked(bool)), matching_action, SLOT(trigger()));
        connect(matching_action, SIGNAL(changed()), tool_button, SLOT(reactOnActionChange()));
      }
      else {
        action_to_add = nullptr;
        widget_to_add = nullptr;
      }

      if (action_to_add != nullptr) {
        action_to_add->setProperty("should_remove_widget", true);
      }
    }

    if (action_to_add != nullptr && widget_to_add != nullptr) {
      action_to_add->setProperty("widget", QVariant::fromValue((void*) widget_to_add));
      addPermanentWidget(widget_to_add);
      addAction(action_to_add);
    }
  }
}

bool StatusBar::eventFilter(QObject *watched, QEvent *event) {
  if (watched == m_lblProgressDownload || watched == m_barProgressDownload) {
    if (event->type() == QEvent::MouseButtonPress) {
      qApp->mainForm()->tabWidget()->showDownloadManager();
    }
  }

  return false;
}

void StatusBar::clear() {
  while (!actions().isEmpty()) {
    QAction *act = actions().at(0);
    QWidget *widget = act->property("widget").isValid() ? static_cast<QWidget*>(act->property("widget").value<void*>()) : nullptr;
    bool should_remove_widget = act->property("should_remove_widget").isValid();
    bool should_remove_action = act->property("should_remove_action").isValid();

    removeAction(act);

    if (widget != nullptr) {
      removeWidget(widget);
      widget->setVisible(false);

      if (should_remove_widget) {
        widget->deleteLater();
      }

      if (should_remove_action) {
        act->deleteLater();
      }
    }
  }
}

void StatusBar::showProgressFeeds(int progress, const QString &label) {
  if (actions().contains(m_barProgressFeedsAction)) {
    m_lblProgressFeeds->setVisible(true);
    m_barProgressFeeds->setVisible(true);

    m_lblProgressFeeds->setText(label);
    m_barProgressFeeds->setValue(progress);
  }
}

void StatusBar::clearProgressFeeds() {
  m_lblProgressFeeds->setVisible(false);
  m_barProgressFeeds->setVisible(false);
}

void StatusBar::showProgressDownload(int progress, const QString &tooltip) {
  if (actions().contains(m_barProgressDownloadAction)) {
    m_lblProgressDownload->setVisible(true);
    m_barProgressDownload->setVisible(true);
    m_barProgressDownload->setValue(progress);
    m_barProgressDownload->setToolTip(tooltip);
    m_lblProgressDownload->setToolTip(tooltip);
  }
}

void StatusBar::clearProgressDownload() {
  m_lblProgressDownload->setVisible(false);
  m_barProgressDownload->setVisible(false);
  m_barProgressDownload->setValue(0);
}
