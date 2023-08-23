// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMLOG_H
#define FORMLOG_H

#include <QDialog>

#include "ui_formlog.h"

class FormLog : public QDialog {
    Q_OBJECT

  public:
    explicit FormLog(QWidget* parent = nullptr);
    virtual ~FormLog();

  public slots:
    void appendLogMessage(const QString& message);

  private:
    Ui::FormLog m_ui;
};

#endif // FORMLOG_H
