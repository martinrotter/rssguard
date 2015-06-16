/* ============================================================
* QuiteRSS is a open-source cross-platform RSS/Atom news feeds reader
* Copyright (C) 2011-2015 QuiteRSS Team <quiterssteam@gmail.com>
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
#ifndef ADBLOCKDIALOG_H
#define ADBLOCKDIALOG_H

#include <QWidget>

#include "ui_adblockdialog.h"

class AdBlockSubscription;
class AdBlockTreeWidget;
class AdBlockManager;
class AdBlockRule;

class AdBlockDialog : public QWidget, public Ui_AdBlockDialog
{
  Q_OBJECT

public:
  explicit AdBlockDialog(QWidget* parent = 0);

  void showRule(const AdBlockRule* rule) const;

private slots:
  void addRule();
  void removeRule();

  void addSubscription();
  void removeSubscription();

  void currentChanged(int index);
  void filterString(const QString &string);
  void enableAdBlock(bool state);

  void aboutToShowMenu();
  void learnAboutRules();

  void loadSubscriptions();
  void load();

private:
  void closeEvent(QCloseEvent* ev);

  AdBlockManager* m_manager;
  AdBlockTreeWidget* m_currentTreeWidget;
  AdBlockSubscription* m_currentSubscription;

  QAction* m_actionAddRule;
  QAction* m_actionRemoveRule;
  QAction* m_actionAddSubscription;
  QAction* m_actionRemoveSubscription;

  bool m_loaded;
  bool m_useLimitedEasyList;
};

#endif // ADBLOCKDIALOG_H

