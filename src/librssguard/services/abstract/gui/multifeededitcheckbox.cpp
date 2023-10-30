// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/abstract/gui/multifeededitcheckbox.h"

MultiFeedEditCheckBox::MultiFeedEditCheckBox(QWidget* parent) : QCheckBox(parent) {
  setToolTip(tr("Apply this to all edited feeds."));
  setText(QString(4, ' '));
  setSizePolicy(QSizePolicy::Policy::Maximum, QSizePolicy::Policy::Maximum);
}

QList<QWidget*> MultiFeedEditCheckBox::actionWidgets() const {
  return m_actionWidgets;
}

void MultiFeedEditCheckBox::addActionWidget(QWidget* widget) {
  if (widget != nullptr) {
    m_actionWidgets.append(widget);
    connect(this, &MultiFeedEditCheckBox::toggled, widget, &QWidget::setEnabled);

    emit toggled(isChecked());
  }
}
