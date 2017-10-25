// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef LABELWITHSTATUS_H
#define LABELWITHSTATUS_H

#include "gui/widgetwithstatus.h"

#include <QLabel>

class LabelWithStatus : public WidgetWithStatus {
  Q_OBJECT

  public:

    // Constructors and destructors.
    explicit LabelWithStatus(QWidget* parent = 0);
    virtual ~LabelWithStatus();

    void setStatus(StatusType status, const QString& label_text, const QString& status_text);

    // Access to label.
    inline QLabel* label() const {
      return static_cast<QLabel*>(m_wdgInput);
    }

};

#endif // LABELWITHSTATUS_H
