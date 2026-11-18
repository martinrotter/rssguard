// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/reusable/basetreeview.h"

#include "gui/reusable/treeviewcolumnsmenu.h"
#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/settingskeys.h"

#include <algorithm>

#include <QContextMenuEvent>
#include <QHash>
#include <QHeaderView>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QScrollBar>
#include <QSet>
#include <QSignalBlocker>

BaseTreeView::BaseTreeView(QWidget* parent) : QTreeView(parent), m_lastWheelTime(0), m_scrollSpeedFactor(1.0) {
  setAllColumnsShowFocus(true);
  setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  header()->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
  header()->setStretchLastSection(false);
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
    const auto id = header()->model()->headerData(i, Qt::Horizontal, COLUMN_ID_ROLE).toString();
    if (!id.isEmpty()) {
      obj[QSL("header_%1_id").arg(i)] = id;
    }
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

  QHash<QString, int> current_ids;
  for (int i = 0; i < header()->count(); ++i) {
    const auto id = header()->model()->headerData(i, Qt::Horizontal, COLUMN_ID_ROLE).toString();
    if (!id.isEmpty()) {
      current_ids.insert(id, i);
    }
  }
  const bool named_state = !current_ids.isEmpty() && obj.contains(QSL("header_0_id"));
  const auto restore_section = [&](int saved, int logical) {
    const int size = obj[QSL("header_%1_size").arg(saved)].toInt(header()->sectionSize(logical));
    const bool hidden = obj[QSL("header_%1_hidden").arg(saved)].toBool(header()->isSectionHidden(logical));
    const int mode = obj[QSL("header_%1_resize_mode").arg(saved)].toInt(int(header()->sectionResizeMode(logical)));
    if (size >= 0) {
      header()->resizeSection(logical, size);
    }
    header()->setSectionHidden(logical, hidden);
    if (mode >= int(QHeaderView::Interactive) && mode <= int(QHeaderView::ResizeToContents)) {
      header()->setSectionResizeMode(logical, QHeaderView::ResizeMode(mode));
    }
  };

  if (saved_header_count < header()->count() && current_ids.isEmpty()) {
    qWarningNN << LOGSEC_GUI << "Detected invalid state for tree view" << QUOTE_W_SPACE_DOT(objectName());
  }
  else {
    header()->setStretchLastSection(obj.contains(QSL("header_stretch_last_section"))
                                      ? obj[QSL("header_stretch_last_section")].toBool()
                                      : false);
    header()->setCascadingSectionResizes(obj.contains(QSL("header_cascading_section_resizes"))
                                           ? obj[QSL("header_cascading_section_resizes")].toBool()
                                           : false);

    if (named_state) {
      QList<QPair<int, int>> order;
      QSet<int> restored;
      for (int saved = 0; saved < saved_header_count; ++saved) {
        const auto id = obj[QSL("header_%1_id").arg(saved)].toString();
        const auto target = current_ids.constFind(id);
        if (target == current_ids.constEnd() || restored.contains(target.value())) {
          continue;
        }
        restored.insert(target.value());
        restore_section(saved, target.value());
        order.append({obj[QSL("header_%1_idx").arg(saved)].toInt(saved), target.value()});
      }
      std::stable_sort(order.begin(), order.end(), [](const auto& left, const auto& right) {
        return left.first < right.first;
      });
      int visual = 0;
      for (const auto& section : order) {
        header()->moveSection(header()->visualIndex(section.second), visual++);
      }
      for (int logical = 0; logical < header()->count(); ++logical) {
        if (!restored.contains(logical)) {
          header()->moveSection(header()->visualIndex(logical), visual++);
        }
      }
    }
    else {
      for (int i = 0; i < saved_header_count && i < header()->count(); i++) {
        const int vi = obj.contains(QSL("header_%1_idx").arg(i)) ? obj[QSL("header_%1_idx").arg(i)].toInt() : i;
        if (vi >= 0 && vi < header()->count()) {
          header()->swapSections(header()->visualIndex(i), vi);
        }
        restore_section(i, i);
      }
    }
  }

  ColumnSortStates states;
  QSet<int> sorted_columns;
  const int saved_sort_count = obj[QSL("sort_count")].toInt();

  for (int i = 0; i < saved_sort_count; i++) {
    const int col = obj[QSL("sort_%1_column").arg(i)].toInt(-1);
    const int order = obj[QSL("sort_%1_order").arg(i)].toInt(-1);

    if (col >= 0 && col < header()->count() &&
        (order == int(Qt::SortOrder::AscendingOrder) || order == int(Qt::SortOrder::DescendingOrder)) &&
        !sorted_columns.contains(col)) {
      states.append(QPair<int, Qt::SortOrder>(col, Qt::SortOrder(order)));
      sorted_columns.insert(col);
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
  Q_UNUSED(states)
}
