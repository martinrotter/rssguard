// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/reusable/resizablestackedwidget.h"

ResizableStackedWidget::ResizableStackedWidget(QWidget* parent) : QStackedWidget(parent) {}

QSize ResizableStackedWidget::sizeHint() const {
  return currentWidget()->sizeHint();
}

QSize ResizableStackedWidget::minimumSizeHint() const {
  return currentWidget()->minimumSizeHint();
}
