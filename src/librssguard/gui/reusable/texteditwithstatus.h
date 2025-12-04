// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef TEXTEDITWITHSTATUS_H
#define TEXTEDITWITHSTATUS_H

#include "gui/reusable/widgetwithstatus.h"

#include <QPlainTextEdit>

class RSSGUARD_DLLSPEC TextEditWithStatus : public WidgetWithStatus {
    Q_OBJECT

  public:
    explicit TextEditWithStatus(QWidget* parent = nullptr);

    // Access to line edit.
    QPlainTextEdit* textEdit() const;
};

inline QPlainTextEdit* TextEditWithStatus::textEdit() const {
  return static_cast<QPlainTextEdit*>(m_wdgInput);
}

#endif // TEXTEDITWITHSTATUS_H
