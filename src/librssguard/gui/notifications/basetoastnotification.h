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

    // If true, then notification is always moved as close to top as possible.
    virtual bool alwaysOnTop() const = 0;

  protected:
    virtual bool eventFilter(QObject* watched, QEvent* event);
    virtual void closeEvent(QCloseEvent* event);

    void setupCloseButton(QAbstractButton* btn);

  signals:
    void closeRequested();
};

#endif // BASETOASTNOTIFICATION_H
