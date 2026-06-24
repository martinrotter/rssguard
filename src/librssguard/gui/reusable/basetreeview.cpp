// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/reusable/basetreeview.h"

#include "gui/reusable/treeviewcolumnsmenu.h"
#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"

#include <QContextMenuEvent>
#include <QHeaderView>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QScrollBar>

BaseTreeView::BaseTreeView(QWidget* parent) : QTreeView(parent), m_lastWheelTime(0), m_scrollSpeedFactor(1.0) {
  setAllColumnsShowFocus(true);
  setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  header()->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
  header()->setStretchLastSection(true);
  header()->setCascadingSectionResizes(false);

  connect(header(), &QHeaderView::customContextMenuRequested, this, [this](const QPoint& point) {
    displayColumnsContextMenu(header()->mapToGlobal(point), header()->logicalIndexAt(point));
  });

  m_allowedKeyboardKeys = {Qt::Key::Key_Back,
                           Qt::Key::Key_Select,
                           Qt::Key::Key_Copy,
                           Qt::Key::Key_Shift,
                           Qt::Key::Key_Control,
                           Qt::Key::Key_Up,
                           Qt::Key::Key_Down,
                           Qt::Key::Key_Left,
                           Qt::Key::Key_Right,
                           Qt::Key::Key_Home,
                           Qt::Key::Key_End,
                           Qt::Key::Key_PageUp,
                           Qt::Key::Key_PageDown};
}

bool BaseTreeView::isIndexHidden(const QModelIndex& idx) const {
  return QTreeView::isIndexHidden(idx);
}

QByteArray BaseTreeView::saveHeaderState() const {
  QJsonObject obj;

  obj[QSL("header_count")] = header()->count();
  obj[QSL("header_stretch_last_section")] = header()->stretchLastSection();
  obj[QSL("header_cascading_section_resizes")] = header()->cascadingSectionResizes();

  for (int i = 0; i < header()->count(); i++) {
    obj[QSL("header_%1_idx").arg(i)] = header()->visualIndex(i);
    obj[QSL("header_%1_size").arg(i)] = header()->sectionSize(i);
    obj[QSL("header_%1_hidden").arg(i)] = header()->isSectionHidden(i);
    obj[QSL("header_%1_resize_mode").arg(i)] = int(header()->sectionResizeMode(i));
  }

  const ColumnSortStates states = columnSortStates();

  obj[QSL("sort_count")] = states.size();

  for (int i = 0; i < states.size(); i++) {
    obj[QSL("sort_%1_column").arg(i)] = states.at(i).first;
    obj[QSL("sort_%1_order").arg(i)] = states.at(i).second;
  }

  return QJsonDocument(obj).toJson(QJsonDocument::JsonFormat::Compact);
}

void BaseTreeView::restoreHeaderState(const QByteArray& dta) {
  const QJsonObject obj = QJsonDocument::fromJson(dta).object();
  const int saved_header_count = obj[QSL("header_count")].toInt();

  if (saved_header_count < header()->count()) {
    qWarningNN << LOGSEC_GUI << "Detected invalid state for tree view" << QUOTE_W_SPACE_DOT(objectName());
    return;
  }

  header()->setStretchLastSection(obj.contains(QSL("header_stretch_last_section"))
                                    ? obj[QSL("header_stretch_last_section")].toBool()
                                    : true);
  header()->setCascadingSectionResizes(obj.contains(QSL("header_cascading_section_resizes"))
                                         ? obj[QSL("header_cascading_section_resizes")].toBool()
                                         : false);

  for (int i = 0; i < saved_header_count && i < header()->count(); i++) {
    const int vi = obj.contains(QSL("header_%1_idx").arg(i)) ? obj[QSL("header_%1_idx").arg(i)].toInt() : i;
    const int ss = obj[QSL("header_%1_size").arg(i)].toInt();
    const bool ish = obj[QSL("header_%1_hidden").arg(i)].toBool();
    const auto resize_mode =
      QHeaderView::ResizeMode(obj.contains(QSL("header_%1_resize_mode").arg(i))
                                ? obj[QSL("header_%1_resize_mode").arg(i)].toInt()
                                : int(header()->sectionResizeMode(i)));

    if (vi >= 0 && vi < header()->count()) {
      header()->swapSections(header()->visualIndex(i), vi);
    }

    header()->setSectionResizeMode(i, resize_mode);
    header()->resizeSection(i, ss);
    header()->setSectionHidden(i, ish);
  }

  ColumnSortStates states;
  const int saved_sort_count = obj[QSL("sort_count")].toInt();

  for (int i = 0; i < saved_sort_count; i++) {
    const int col = obj[QSL("sort_%1_column").arg(i)].toInt();
    const auto order = Qt::SortOrder(obj[QSL("sort_%1_order").arg(i)].toInt());

    if (col >= 0 && col < header()->count()) {
      states.append(QPair<int, Qt::SortOrder>(col, order));
    }
  }

  restoreColumnSortStates(states);
}

void BaseTreeView::wheelEvent(QWheelEvent* event) {
  qint64 now = m_scrollingTimer.elapsed();

  if (m_lastWheelTime == 0) {
    m_scrollingTimer.start();
    m_lastWheelTime = 1;
  }
  else {
    qint64 dt = now - m_lastWheelTime;
    m_lastWheelTime = now;

    // If scrolling quickly, boost speed; otherwise decay.
    if (dt < 120) {                                                // <120 ms between wheel events → user scrolling fast
      m_scrollSpeedFactor = qMin(m_scrollSpeedFactor * 1.15, 5.0); // accelerate up to 6×
    }
    else {
      m_scrollSpeedFactor = qMax(m_scrollSpeedFactor * 0.6, 1.0); // slowly return to normal
    }
  }

  int delta = static_cast<int>(event->angleDelta().y() * m_scrollSpeedFactor);
  verticalScrollBar()->setValue(verticalScrollBar()->value() - delta);

  qDebugNN << LOGSEC_GUI << "Tree view scrolling factor is" << NONQUOTE_W_SPACE_DOT(m_scrollSpeedFactor);
}

void BaseTreeView::keyPressEvent(QKeyEvent* event) {
  if (qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::OnlyBasicShortcutsInLists)).toBool() &&
      !m_allowedKeyboardKeys.contains(event->key()) && !event->matches(QKeySequence::StandardKey::SelectAll)) {
    event->ignore();
  }
  else {
    QTreeView::keyPressEvent(event);
  }
}

void BaseTreeView::contextMenuEvent(QContextMenuEvent* event) {
  if (!indexAt(event->pos()).isValid()) {
    displayColumnsContextMenu(event->globalPos());
  }
  else {
    QTreeView::contextMenuEvent(event);
  }
}

void BaseTreeView::displayColumnsContextMenu(const QPoint& global_pos, int highlighted_section) {
  TreeViewColumnsMenu menu(header(), highlighted_section);

  menu.setMenuExtensionBuilder([this, highlighted_section](TreeViewColumnsMenu* columns_menu) {
    return addColumnsContextMenuItems(columns_menu, highlighted_section);
  });
  menu.exec(global_pos);
}

bool BaseTreeView::addColumnsContextMenuItems(TreeViewColumnsMenu* menu, int highlighted_section) {
  Q_UNUSED(menu)
  Q_UNUSED(highlighted_section)

  return false;
}

BaseTreeView::ColumnSortStates BaseTreeView::columnSortStates() const {
  const int column = header()->sortIndicatorSection();

  if (!isSortingEnabled() || column < 0 || column >= header()->count()) {
    return {};
  }

  return {QPair<int, Qt::SortOrder>(column, header()->sortIndicatorOrder())};
}

void BaseTreeView::restoreColumnSortStates(const ColumnSortStates& states) {
  if (states.isEmpty()) {
    return;
  }

  header()->setSortIndicator(states.constFirst().first, states.constFirst().second);
}
