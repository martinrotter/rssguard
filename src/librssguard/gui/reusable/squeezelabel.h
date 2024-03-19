// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef SQUEEZELABEL_H
#define SQUEEZELABEL_H

#include <QLabel>

class SqueezeLabel : public QLabel {
    Q_OBJECT

  public:
    explicit SqueezeLabel(QWidget* parent = nullptr);

  protected:
    void paintEvent(QPaintEvent* event);

  private:
    QString m_squeezedTextCache;
};

#endif // SQUEEZELABEL_H
