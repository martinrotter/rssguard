// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef ARTICLEAMOUNTCONTROL_H
#define ARTICLEAMOUNTCONTROL_H

#include <QWidget>

#include "ui_articleamountcontrol.h"

class ArticleAmountControl : public QWidget {
    Q_OBJECT

  public:
    struct Setup {
        // Ignoring articles.
        bool m_avoidOldArticles = false;
        bool m_addAnyArticlesToDb = false;
        QDateTime m_dtToAvoid = QDateTime();
        int m_hoursToAvoid = 0;

        // Limitting articles.
        int m_keepCountOfArticles = 0;
        bool m_doNotRemoveStarred = true;
        bool m_doNotRemoveUnread = true;
        bool m_moveToBinDontPurge = false;
    };

    explicit ArticleAmountControl(QWidget* parent = nullptr);

    void setForAppWideFeatures(bool app_wide, bool batch_edit);

    void load(const Setup& setup);
    Setup save() const;

  private slots:
    void updateArticleCountSuffix(int count);

  private:
    Ui::ArticleAmountControl m_ui;

  signals:
    void changed();
};

#endif // ARTICLEAMOUNTCONTROL_H
