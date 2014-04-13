#ifndef FORMUPDATER_H
#define FORMUPDATER_H

#include <QMainWindow>


class FormUpdater : public QMainWindow {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit FormUpdater(QWidget *parent = 0);
    virtual ~FormUpdater();

  private:
    void moveToCenterAndResize();

};

#endif // FORMUPDATER_H
