// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMTTRSSNOTE_H
#define FORMTTRSSNOTE_H

#include "ui_formttrssnote.h"

#include <QDialog>

class TtRssServiceRoot;

class FormTtRssNote : public QDialog {
    Q_OBJECT

  public:
    explicit FormTtRssNote(TtRssServiceRoot* root);

  private slots:
    void sendNote();
    void onTitleChanged(const QString& text);
    void onUrlChanged(const QString& text);

  private:
    void updateOkButton();

  private:
    Ui::FormTtRssNote m_ui;
    TtRssServiceRoot* m_root;
    bool m_titleOk;
    bool m_urlOk;
};

#endif // FORMTTRSSNOTE_H
