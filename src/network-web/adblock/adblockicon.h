/* ============================================================
* QupZilla - WebKit based browser
* Copyright (C) 2010-2014  David Rosca <nowrep@gmail.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* ============================================================ */
#ifndef ADBLOCKICON_H
#define ADBLOCKICON_H

#include "qzcommon.h"
#include "clickablelabel.h"
#include "adblockrule.h"

class QMenu;
class QUrl;

class BrowserWindow;

class QUPZILLA_EXPORT AdBlockIcon : public ClickableLabel
{
    Q_OBJECT
public:
    explicit AdBlockIcon(BrowserWindow* window, QWidget* parent = 0);
    ~AdBlockIcon();

    void popupBlocked(const QString &ruleString, const QUrl &url);
    QAction* menuAction();

public slots:
    void setEnabled(bool enabled);
    void createMenu(QMenu* menu = 0);

private slots:
    void showMenu(const QPoint &pos);
    void toggleCustomFilter();

    void animateIcon();
    void stopAnimation();

private:
    BrowserWindow* m_window;
    QAction* m_menuAction;

    QVector<QPair<AdBlockRule*, QUrl> > m_blockedPopups;
    QTimer* m_flashTimer;

    int m_timerTicks;
    bool m_enabled;
};

#endif // ADBLOCKICON_H
