// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMLOG_H
#define FORMLOG_H

#include "ui_formlog.h"

#include <QDialog>

class FormLog : public QDialog {
    Q_OBJECT

  public:
    explicit FormLog(QWidget* parent = nullptr);
    virtual ~FormLog();

  public slots:
    void clearLog();
    void appendLogMessage(const QString& message);

  private:
    Ui::FormLog m_ui;
};

#endif // FORMLOG_H
