// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef MESSAGECOUNTSPINBOX_H
#define MESSAGECOUNTSPINBOX_H

#include <QSpinBox>

class RSSGUARD_DLLSPEC MessageCountSpinBox : public QSpinBox {
    Q_OBJECT

  public:
    explicit MessageCountSpinBox(QWidget* parent = nullptr);
};

#endif // MESSAGECOUNTSPINBOX_H
