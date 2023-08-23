// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef BASELINEEDIT_H
#define BASELINEEDIT_H

#include <QLineEdit>

class BaseLineEdit : public QLineEdit {
  Q_OBJECT

  public:
    explicit BaseLineEdit(QWidget* parent = nullptr);
    virtual ~BaseLineEdit();

    void setPasswordMode(bool is_password);

  public slots:
    void submit(const QString& text);

  protected:
    void keyPressEvent(QKeyEvent* event);

  signals:
    void submitted(const QString& text);

  private:
    QAction* m_actShowPassword;
};

#endif // BASELINEEDIT_H
