// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef TIMESPINBOX_H
#define TIMESPINBOX_H

#include <QDoubleSpinBox>

class TimeSpinBox : public QDoubleSpinBox {
  Q_OBJECT

  public:
    explicit TimeSpinBox(QWidget* parent = 0);
    virtual ~TimeSpinBox();

    double valueFromText(const QString& text) const;
    QString textFromValue(double val) const;
    void fixup(QString& input) const;
    QValidator::State validate(QString& input, int& pos) const;
};

#endif // TIMESPINBOX_H
