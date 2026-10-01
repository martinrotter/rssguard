// Button painting is unrelated to staged shortcut assignment behavior.
#ifndef SHORTCUT_TEST_PLAINTOOLBUTTON_H
#define SHORTCUT_TEST_PLAINTOOLBUTTON_H

#include <QToolButton>

class PlainToolButton : public QToolButton {
  public:
    using QToolButton::QToolButton;
};

#endif
