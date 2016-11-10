// This file is part of RSS Guard.
//
// Copyright (C) 2011-2016 by Martin Rotter <rotter.martinos@gmail.com>
//
// RSS Guard is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// RSS Guard is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with RSS Guard. If not, see <http://www.gnu.org/licenses/>.

#ifndef STATUSBAR_H
#define STATUSBAR_H

#include <QStatusBar>

#include "gui/basetoolbar.h"


class QProgressBar;
class PlainToolButton;
class QLabel;
class Mutex;

class StatusBar : public QStatusBar, public BaseBar {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit StatusBar(QWidget *parent = 0);
    virtual ~StatusBar();

    QList<QAction*> availableActions() const;
    QList<QAction*> changeableActions() const;
    void saveChangeableActions(const QStringList &actions);
    void loadChangeableActions();

  public slots:
    // Progress bar operations
    void showProgressFeeds(int progress, const QString &label);
    void clearProgressFeeds();

    void showProgressDownload(int progress, const QString &tooltip);
    void clearProgressDownload();

  protected:
    bool eventFilter(QObject *watched, QEvent *event);

  private:
    void clear();
    void loadChangeableActions(const QStringList &action_names);

    Mutex *m_mutex;

    QProgressBar *m_barProgressFeeds;
    QAction *m_barProgressFeedsAction;

    QLabel *m_lblProgressFeeds;
    QAction *m_lblProgressFeedsAction;

    QProgressBar *m_barProgressDownload;
    QAction *m_barProgressDownloadAction;

    QLabel *m_lblProgressDownload;
    QAction *m_lblProgressDownloadAction;
};

#endif // STATUSBAR_H
