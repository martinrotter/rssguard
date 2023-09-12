// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef BASETOASTNOTIFICATION_H
#define BASETOASTNOTIFICATION_H

#include <QDialog>

class QAbstractButton;

class BaseToastNotification : public QDialog {
    Q_OBJECT

  public:
    explicit BaseToastNotification(QWidget* parent = nullptr);
    virtual ~BaseToastNotification();

  protected:
    void setupCloseButton(QAbstractButton* btn);
};

#endif // BASETOASTNOTIFICATION_H
