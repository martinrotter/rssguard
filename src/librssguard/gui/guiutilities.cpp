// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/guiutilities.h"

#include "definitions/definitions.h"

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

  widget.setWindowFlags(
    Qt::WindowType::Dialog |
    Qt::WindowType::WindowTitleHint |
    Qt::WindowType::WindowMaximizeButtonHint |
    Qt::WindowType::WindowCloseButtonHint);

  widget.setWindowIcon(icon);

  if (!title.isEmpty()) {
    widget.setWindowTitle(title);
  }
}

void GuiUtilities::restoreState(QWidget* wdg, QByteArray state) {
  QHash<QString, QHash<QString, QVariant>> props;
  QDataStream str(&state, QIODevice::OpenModeFlag::ReadOnly);

  str >> props;

  QList<QObject*> to_process = { wdg };

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
  QHash<QString, QStringList> props_to_serialize {
    { QSL("QCheckBox"), { QSL("checked") } },
    { QSL("QSpinBox"), { QSL("value") } }
  };
  QHash<QString, QHash<QString, QVariant>> props;
  QList<QObject*> to_process = { wdg };

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
