// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef COMBOBOXWITHSTATUS_H
#define COMBOBOXWITHSTATUS_H

#include "gui/reusable/widgetwithstatus.h"

#include <QComboBox>

class ComboBoxWithStatus : public WidgetWithStatus {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit ComboBoxWithStatus(QWidget* parent = nullptr);
    virtual ~ComboBoxWithStatus() = default;

    inline QComboBox* comboBox() const {
      return static_cast<QComboBox*>(m_wdgInput);
    }
};

#endif // COMBOBOXWITHSTATUS_H
