// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef BASELINEEDIT_H
#define BASELINEEDIT_H

#include <QLineEdit>
#include <QTimer>

class RSSGUARD_DLLSPEC BaseLineEdit : public QLineEdit {
    Q_OBJECT

  public:
    explicit BaseLineEdit(QWidget* parent = nullptr);
    virtual ~BaseLineEdit();

    void setPasswordMode(bool is_password);

  public slots:
    void submit(const QString& text);

  protected:
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void focusInEvent(QFocusEvent* event);
    virtual void contextMenuEvent(QContextMenuEvent* event);

  signals:
    void submitted(const QString& text);

  private:
    QAction* m_actShowPassword;
    QTimer m_tmrSelectAll;
};

#endif // BASELINEEDIT_H
