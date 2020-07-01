// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/guiutilities.h"

#include "definitions/definitions.h"

#if defined (Q_OS_ANDROID)
#include <QApplication>
#include <QDesktopWidget>
#include <QSize>
#endif

void GuiUtilities::setLabelAsNotice(QLabel& label, bool is_warning) {
  label.setMargin(6);

  if (is_warning) {
    label.setStyleSheet(QSL("font-weight: bold; font-style: italic; color: red"));
  }
  else {
    label.setStyleSheet(QSL("font-style: italic;"));
  }
}

void GuiUtilities::applyDialogProperties(QWidget& widget, const QIcon& icon, const QString& title) {
  widget.setWindowFlags(/*Qt::MSWindowsFixedSizeDialogHint | */ Qt::Dialog | Qt::WindowSystemMenuHint | Qt::WindowTitleHint);
  widget.setWindowIcon(icon);

  if (!title.isEmpty()) {
    widget.setWindowTitle(title);
  }
}

void GuiUtilities::applyResponsiveDialogResize(QWidget& widget, double factor) {
#if defined (Q_OS_ANDROID)
  auto desktop_geom = QApplication::desktop()->screenGeometry();
  auto ratio = double(widget.size().height()) / widget.size().width();
  int widt = desktop_geom.width() * factor;
  int heig = widt * ratio;

  widget.resize(widt, heig);
#else
  Q_UNUSED(factor)
  Q_UNUSED(widget)
#endif
}
