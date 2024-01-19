// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef ARTICLEAMOUNTCONTROL_H
#define ARTICLEAMOUNTCONTROL_H

#include <QWidget>

#include "ui_articleamountcontrol.h"

#include "services/abstract/feed.h"

class ArticleAmountControl : public QWidget {
    Q_OBJECT

    friend class FormFeedDetails;

  public:
    explicit ArticleAmountControl(QWidget* parent = nullptr);

    void setForAppWideFeatures(bool app_wide, bool batch_edit);

    void load(const Feed::ArticleIgnoreLimit& setup, bool always_avoid = false);
    Feed::ArticleIgnoreLimit save() const;

    void saveFeed(Feed* fd, bool batch_edit) const;

  private slots:
    void updateArticleCountSuffix(int count);

  private:
    Ui::ArticleAmountControl m_ui;

  signals:
    void changed();
};

#endif // ARTICLEAMOUNTCONTROL_H
