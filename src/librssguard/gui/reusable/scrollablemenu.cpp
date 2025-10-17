// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/reusable/scrollablemenu.h"

#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"

#include <cmath>

#include <QWheelEvent>

#define ACTIONS_PAGE_SIZE 20

ScrollableMenu::ScrollableMenu(QWidget* parent) : NonClosableMenu(parent) {}

ScrollableMenu::ScrollableMenu(const QString& title, QWidget* parent) : NonClosableMenu(title, parent) {}

void ScrollableMenu::setActions(const QList<QAction*>& actions, bool close_on_trigger) {
  m_actions = actions;
  m_actionsClosing = close_on_trigger;

  setArrowsVisible(m_actions.size() > ACTIONS_PAGE_SIZE);
  setPage(0);
}

void ScrollableMenu::wheelEvent(QWheelEvent* e) {
  bool up = e->angleDelta().y() > 0;
  bool down = e->angleDelta().y() < 0;

  if (up && m_actUp != nullptr && m_actUp->isEnabled()) {
    m_actUp->trigger();
  }
  else if (down && m_actDown != nullptr && m_actDown->isEnabled()) {
    m_actDown->trigger();
  }

  QMenu::wheelEvent(e);
}

bool ScrollableMenu::shouldActionClose(QAction* action) const {
  if (action != nullptr && (action == m_actDown || action == m_actUp)) {
    return false;
  }
  else {
    return m_actionsClosing;
  }
}

void ScrollableMenu::clearCurrentPage() {
  if (m_arrowsVisible) {
    // Do not clear arrows.
    auto acts = actions();

    acts.removeFirst();
    acts.removeFirst();
    acts.removeLast();
    acts.removeLast();

    for (auto* act : std::as_const(acts)) {
      removeAction(act);
    }
  }
  else {
    clear();
  }
}

void ScrollableMenu::initialize() {
  m_actUp = new QAction(qApp->icons()->fromTheme(QSL("go-up"), QSL("arrow-up")), tr("Go &up"), this);
  m_actDown = new QAction(qApp->icons()->fromTheme(QSL("go-down"), QSL("arrow-down")), tr("Go &down"), this);

  connect(m_actUp, &QAction::triggered, this, [this]() {
    setPage(m_page - 1);
  });
  connect(m_actDown, &QAction::triggered, this, [this]() {
    setPage(m_page + 1);
  });
}

void ScrollableMenu::setPageName(const QString& name) {
  actions().at(1)->setText(name);
}

void ScrollableMenu::setPage(int page) {
  m_page = page;

  if (!m_arrowsVisible) {
    // NOTE: No pagination, setPage() is only called once.
    addActions(m_actions);
  }
  else {
    // First we remove current actions.
    clearCurrentPage();

    QAction* last_separator = actions().at(actions().size() - 2);
    int first_index = page * ACTIONS_PAGE_SIZE;
    bool end_of_actions = false;
    for (int i = 0; i < ACTIONS_PAGE_SIZE; i++) {
      int idx = i + first_index;

      if (idx >= m_actions.size()) {
        end_of_actions = true;
        break;
      }

      insertAction(last_separator, m_actions.at(idx));

      if (idx == m_actions.size() - 1) {
        end_of_actions = true;
        break;
      }
    }

    m_actDown->setEnabled(!end_of_actions);
    m_actUp->setEnabled(first_index > 0);
    setPageName(tr("%1 %2/%3")
                  .arg(title().isEmpty() ? tr("Page") : title(),
                       QString::number(page + 1),
                       QString::number(int(std::ceil(m_actions.size() * 1.0 / ACTIONS_PAGE_SIZE)))));
  }
}

void ScrollableMenu::setArrowsVisible(bool visible) {
  m_arrowsVisible = visible;
  clear();

  if (m_actUp != nullptr) {
    m_actUp->deleteLater();
    m_actUp = nullptr;
  }

  if (m_actDown != nullptr) {
    m_actDown->deleteLater();
    m_actDown = nullptr;
  }

  if (visible) {
    initialize();
    addAction(m_actUp);
    addSection(QString());
    addSeparator();
    addAction(m_actDown);
  }
}
