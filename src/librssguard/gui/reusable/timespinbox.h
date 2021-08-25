// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef TIMESPINBOX_H
#define TIMESPINBOX_H

#include <QDoubleSpinBox>

class TimeSpinBox : public QDoubleSpinBox {
  Q_OBJECT

  public:
    enum class Mode {
      HoursMinutes,
      MinutesSeconds
    };

    explicit TimeSpinBox(QWidget* parent = nullptr);

    double valueFromText(const QString& text) const;
    QString textFromValue(double val) const;
    void fixup(QString& input) const;
    QValidator::State validate(QString& input, int& pos) const;

    Mode mode() const;
    void setMode(const TimeSpinBox::Mode& mode);

  private:
    Mode m_mode;
};

#endif // TIMESPINBOX_H
