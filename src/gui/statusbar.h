// This file is part of RSS Guard.
//
// Copyright (C) 2011-2015 by Martin Rotter <rotter.martinos@gmail.com>
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


class QProgressBar;
class PlainToolButton;
class QLabel;
class AdBlockIcon;

class StatusBar : public QStatusBar {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit StatusBar(QWidget *parent = 0);
    virtual ~StatusBar();

    inline PlainToolButton *fullscreenSwitcher() const {
      return m_fullscreenSwitcher;
    }

    inline AdBlockIcon *adBlockIcon() {
      return m_adblockIcon;
    }

  public slots:
    // Progress bar operations
    void showProgressFeeds(int progress, const QString &label);
    void clearProgressFeeds();

    void showProgressDownload(int progress, const QString &tooltip);
    void clearProgressDownload();

  protected:
    bool eventFilter(QObject *watched, QEvent *event);

  private:
    QProgressBar *m_barProgressFeeds;
    QLabel *m_lblProgressFeeds;
    QProgressBar *m_barProgressDownload;
    QLabel *m_lblProgressDownload;
    PlainToolButton *m_fullscreenSwitcher;
    AdBlockIcon* m_adblockIcon;
};

#endif // STATUSBAR_H
