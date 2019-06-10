// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef COLORLABEL_H
#define COLORLABEL_H

#include <QLabel>

class ColorLabel : public QLabel {
  Q_OBJECT

  public:
    explicit ColorLabel(QWidget* parent = 0);
    virtual ~ColorLabel();

    QColor color() const;
    void setColor(const QColor& color);

  protected:
    void paintEvent(QPaintEvent* event);

  private:
    QColor m_color;
};

#endif // COLORLABEL_H
