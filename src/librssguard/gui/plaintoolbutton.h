// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef CLOSEBUTTON_H
#define CLOSEBUTTON_H

#include <QToolButton>

class PlainToolButton : public QToolButton {
  Q_OBJECT

  public:

    // Contructors and destructors.
    explicit PlainToolButton(QWidget* parent = 0);
    virtual ~PlainToolButton();

    // Padding changers.
    int padding() const;
    void setPadding(int padding);

  public slots:
    void setChecked(bool checked);
    void reactOnActionChange(QAction* action);
    void reactOnSenderActionChange();

  protected:

    // Custom look.
    void paintEvent(QPaintEvent* e);

  private:
    int m_padding;
};

#endif // CLOSEBUTTON_H
