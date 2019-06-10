// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef LINEEDITWITHSTATUS_H
#define LINEEDITWITHSTATUS_H

#include "gui/widgetwithstatus.h"

#include "gui/baselineedit.h"

class LineEditWithStatus : public WidgetWithStatus {
  Q_OBJECT

  public:

    // Constructors and destructors.
    explicit LineEditWithStatus(QWidget* parent = 0);
    virtual ~LineEditWithStatus();

    // Access to line edit.
    inline BaseLineEdit* lineEdit() const {
      return static_cast<BaseLineEdit*>(m_wdgInput);
    }

};

#endif // LINEEDITWITHSTATUS_H
