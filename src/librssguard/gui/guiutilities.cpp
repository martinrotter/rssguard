// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/guiutilities.h"

#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"

#include <QDialog>
#include <QMainWindow>
#include <QScreen>
#include <QSettings>

#if defined(Q_OS_ANDROID)
#include <QApplication>
#include <QDesktopWidget>
#include <QSize>
#endif

void GuiUtilities::setLabelAsNotice(QLabel& label, bool is_warning, bool set_margins) {
  if (set_margins) {
    label.setMargin(6);
  }

  if (is_warning) {
    label.setStyleSheet(QSL("font-weight: bold; font-style: italic; color: red"));
  }
  else {
    label.setStyleSheet(QSL("font-style: italic;"));
  }
}

void GuiUtilities::applyDialogProperties(QWidget& widget, const QIcon& icon, const QString& title) {
  widget.setWindowFlags(Qt::WindowType::Dialog | Qt::WindowType::WindowTitleHint |
                        Qt::WindowType::WindowMaximizeButtonHint | Qt::WindowType::WindowCloseButtonHint);

  widget.setWindowIcon(icon);

  if (!title.isEmpty()) {
    widget.setWindowTitle(title);
  }

  loadDialogSize(widget);
  fixTooBigDialog(widget);
  saveSizeOnWidgetClosed(widget);
}

void GuiUtilities::fixTooBigDialog(QWidget& widget, bool move_to_center) {
  // We fix too big dialog size or out-of-bounds position.
  auto size_widget = widget.frameGeometry().size();
  auto size_widget_original = size_widget;
  auto size_screen = widget.screen()->availableSize();

  if (size_widget.width() > size_screen.width()) {
    size_widget.setWidth(size_screen.width() * 0.95);
  }

  if (size_widget.height() > size_screen.height()) {
    size_widget.setHeight(size_screen.height() * 0.95);
  }

  bool resized = false;

  if (size_widget != size_widget_original) {
    qWarningNN << LOGSEC_GUI << "Dialog" << QUOTE_W_SPACE(widget.metaObject()->className()) << "was down-sized from"
               << QUOTE_W_SPACE(widget.size()) << "to" << QUOTE_W_SPACE_DOT(size_widget);
    resized = true;
    widget.resize(size_widget);
  }

  auto pos_widget = widget.pos();

  if ((resized && move_to_center) || pos_widget.x() < 0 || pos_widget.y() < 0) {
    //  Calculate ideal position for centering the widget.
    auto size_parent = widget.parentWidget() != nullptr ? widget.frameGeometry().size() : QSize(0, 0);

    // If dialog is bigger than its parent, center it to screen.
    // If dialog is smaller than its parent, center to parent.
    bool screen_as_parent;
    QSize size_to_center;

    if (size_widget.width() > size_parent.width() || size_widget.height() > size_parent.height()) {
      screen_as_parent = true;
      size_to_center = size_screen;
    }
    else {
      screen_as_parent = false;
      size_to_center = size_parent;
    }

    auto origin_x = (size_to_center.width() - size_widget.width()) / 2.0;
    auto origin_y = (size_to_center.height() - size_widget.height()) / 2.0;
    auto origin_pos = QPoint(origin_x, origin_y);

    if (origin_pos != pos_widget) {
      qWarningNN << LOGSEC_GUI << "Dialog" << QUOTE_W_SPACE(widget.metaObject()->className()) << "was moved from"
                 << QUOTE_W_SPACE(pos_widget) << "to" << QUOTE_W_SPACE_DOT(origin_pos);
      widget.move(screen_as_parent ? origin_pos : origin_pos + widget.parentWidget()->pos());
    }
  }
}

void GuiUtilities::loadDialogSize(QWidget& wdg) {
  const QString on = wdg.objectName();

  if (on.isEmpty()) {
    qWarningNN << LOGSEC_GUI << "Object of class" << QUOTE_W_SPACE(wdg.metaObject()->className())
               << "has no name, cannot load its size.";
    return;
  }

  const QString key = QSL("%1_size").arg(on);

  wdg.resize(qApp->settings()->value(GROUP(DialogGeometries), key, wdg.size()).toSize());
}

void GuiUtilities::saveSizeOnWidgetClosed(QWidget& wdg) {
  const QString on = wdg.objectName();

  if (on.isEmpty()) {
    qWarningNN << LOGSEC_GUI << "Object of class" << QUOTE_W_SPACE(wdg.metaObject()->className())
               << "has no name, cannot save its size when it closes.";
    return;
  }

  QDialog* wdg_dialog = qobject_cast<QDialog*>(&wdg);

  if (wdg_dialog != nullptr) {
    QObject::connect(wdg_dialog, &QDialog::finished, [=]() {
      const QString key = QSL("%1_size").arg(on);

      qDebugNN << LOGSEC_GUI << "Saving size for dialog" << QUOTE_W_SPACE_DOT(on);
      qApp->settings()->setValue(GROUP(DialogGeometries), key, wdg_dialog->size());
    });
  }
}

void GuiUtilities::restoreState(QWidget* wdg, QByteArray state) {
  QHash<QString, QHash<QString, QVariant>> props;
  QDataStream str(&state, QIODevice::OpenModeFlag::ReadOnly);

  str >> props;

  QList<QObject*> to_process = {wdg};

  while (!to_process.isEmpty()) {
    QObject* act = to_process.takeFirst();

    if (props.contains(act->objectName())) {
      auto saved_props = props.value(act->objectName());
      auto saved_props_names = saved_props.keys();

      for (const QString& saved_key : saved_props_names) {
        act->setProperty(saved_key.toLocal8Bit().constData(), saved_props.value(saved_key));
      }
    }

    to_process.append(act->children());
  }
}

QByteArray GuiUtilities::saveState(QWidget* wdg) {
  QHash<QString, QStringList> props_to_serialize{{QSL("QCheckBox"), {QSL("checked")}},
                                                 {QSL("QSpinBox"), {QSL("value")}}};
  QHash<QString, QHash<QString, QVariant>> props;
  QList<QObject*> to_process = {wdg};

  while (!to_process.isEmpty()) {
    QObject* act = to_process.takeFirst();
    const QMetaObject* meta = act->metaObject();
    auto act_props = props_to_serialize.value(meta->className());
    QHash<QString, QVariant> props_obj;

    for (const QString& prop : act_props) {
      props_obj.insert(prop, act->property(prop.toLocal8Bit().constData()));
    }

    if (!props_obj.isEmpty()) {
      props.insert(act->objectName(), props_obj);
    }

    to_process.append(act->children());
  }

  QByteArray arr;
  QDataStream str(&arr, QIODevice::OpenModeFlag::WriteOnly);

  str << props;
  return arr;
}
