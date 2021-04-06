// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/messagecountspinbox.h"

#include "definitions/definitions.h"

#include <QtGlobal>

MessageCountSpinBox::MessageCountSpinBox(QWidget* parent) : QSpinBox(parent) {
  connect(this, QOverload<int>::of(&QSpinBox::valueChanged), this, [=](int value) {
    if (value <= 0) {
      setSuffix(QSL(" ") + tr("= unlimited"));
    }
    else if (value == 1) {
      setSuffix(QSL(" ") + tr("message"));
    }
    else {
      setSuffix(QSL(" ") + tr("messages"));
    }
  });

  setMinimum(-1);
  setMaximum(999999);
  setValue(200);
}
