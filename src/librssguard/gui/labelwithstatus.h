// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef LABELWITHSTATUS_H
#define LABELWITHSTATUS_H

#include "gui/widgetwithstatus.h"

#include <QLabel>

class LabelWithStatus : public WidgetWithStatus {
  Q_OBJECT

  public:
    explicit LabelWithStatus(QWidget* parent = nullptr);

    void setStatus(StatusType status, const QString& label_text, const QString& status_text);

    // Access to label.
    QLabel* label() const;
};

inline QLabel* LabelWithStatus::label() const {
  return static_cast<QLabel*>(m_wdgInput);
}

#endif // LABELWITHSTATUS_H
