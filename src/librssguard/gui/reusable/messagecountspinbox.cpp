// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/reusable/messagecountspinbox.h"

#include "definitions/definitions.h"

#include <QtGlobal>

MessageCountSpinBox::MessageCountSpinBox(QWidget* parent) : QSpinBox(parent) {
  connect(this, QOverload<int>::of(&QSpinBox::valueChanged), this, [=](int value) {
    if (value <= 0) {
      setSuffix(QSL(" ") + tr("= unlimited"));
    }
    else if (value == 1) {
      setSuffix(QSL(" ") + tr("article"));
    }
    else {
      setSuffix(QSL(" ") + tr("articles"));
    }
  });

  setMinimum(-1);
  setMaximum(999999);
  setValue(200);
}
