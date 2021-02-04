// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef LINEEDITWITHSTATUS_H
#define LINEEDITWITHSTATUS_H

#include "gui/widgetwithstatus.h"

#include "gui/baselineedit.h"

#include <QPlainTextEdit>

class LineEditWithStatus : public WidgetWithStatus {
  Q_OBJECT

  public:
    explicit LineEditWithStatus(QWidget* parent = nullptr);

    // Access to line edit.
    BaseLineEdit* lineEdit() const;
};

inline BaseLineEdit* LineEditWithStatus::lineEdit() const {
  return static_cast<BaseLineEdit*>(m_wdgInput);
}

class TextEditWithStatus : public WidgetWithStatus {
  Q_OBJECT

  public:
    explicit TextEditWithStatus(QWidget* parent = nullptr);

    // Access to line edit.
    QPlainTextEdit* textEdit() const;
};

inline QPlainTextEdit* TextEditWithStatus::textEdit() const {
  return static_cast<QPlainTextEdit*>(m_wdgInput);
}

#endif // LINEEDITWITHSTATUS_H
