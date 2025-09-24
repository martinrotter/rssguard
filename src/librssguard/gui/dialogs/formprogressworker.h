// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMPROGRESSWORKER_H
#define FORMPROGRESSWORKER_H

#include "ui_formprogressworker.h"

#include <QDialog>

class FormProgressWorker : public QDialog {
    Q_OBJECT

  public:
    explicit FormProgressWorker(const QString& title, const QList<QVariant>& data, QWidget* parent = nullptr);
    virtual ~FormProgressWorker();

  private:
    Ui::FormProgressWorker m_ui;
};

#endif // FORMPROGRESSWORKER_H
