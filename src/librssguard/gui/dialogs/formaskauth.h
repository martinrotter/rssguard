// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMASKAUTH_H
#define FORMASKAUTH_H

#include "ui_formaskauth.h"

#include <QDialog>

class FormAskauth : public QDialog {
    Q_OBJECT

  public:
    explicit FormAskauth(QWidget* parent = nullptr);
    virtual ~FormAskauth();

    static QPair<QString, QString> getUsernamePassword(const QString& title);

  private slots:
    void onUsernameChanged(const QString& username);
    void onPasswordChanged(const QString& password);

  protected:
    virtual void closeEvent(QCloseEvent* event);
    virtual void keyPressEvent(QKeyEvent* event);

  private:
    Ui::FormAskauth m_ui;
};

#endif // FORMASKAUTH_H
