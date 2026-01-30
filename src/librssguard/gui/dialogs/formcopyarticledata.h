// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMCOPYARTICLEDATA_H
#define FORMCOPYARTICLEDATA_H

#include "ui_formcopyarticledata.h"

#include <optional>

#include <QAbstractItemModel>
#include <QDialog>

struct PatternDecision {
    QString m_pattern;
    bool m_escapeCsv;
};

class FormCopyArticleData : public QDialog {
    Q_OBJECT

  public:
    explicit FormCopyArticleData(QAbstractTableModel* model, QWidget* parent = nullptr);
    virtual ~FormCopyArticleData();

  public slots:
    std::optional<PatternDecision> pattern();

  protected:
    virtual void closeEvent(QCloseEvent* event);
    virtual void keyPressEvent(QKeyEvent* event);

  private:
    QString helpText(QAbstractTableModel* model) const;

  private:
    Ui::FormCopyArticleData m_ui;
};

#endif // FORMLOG_H
