// This file is part of RSS Guard.
//
// Copyright (C) 2011-2014 by Martin Rotter <rotter.martinos@gmail.com>
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

#ifndef FORMUPDATER_H
#define FORMUPDATER_H

#include <QMainWindow>

#include <QtGlobal>
#include <QHash>


class QTextEdit;
class QKeyEvent;

class FormUpdater : public QMainWindow {
    Q_OBJECT

  public:
    // Describes the state of updater.
    enum UpdaterState {
      NoState,
      ExitNormal,
      ExitError
    };

    // Constructors and destructors.
    explicit FormUpdater(QWidget *parent = 0);
    virtual ~FormUpdater();

    // Prints various texts.
    void printText(const QString &text);
    void printNewline();
    void printHeading(const QString &header);

    // Starts the whole update process.
    void startUpgrade();

    // Various parts of update process.
    void saveArguments();
    void printArguments();
    bool printUpdateInformation();
    bool doPreparationCleanup();
    bool doExtractionAndCopying();
    bool doFinalCleanup();
    void executeMainApplication();

    // Used to trigger signal informaing about new debug message.
    void triggerDebugMessageConsumption(QtMsgType type, const QString &message);

    // Debug handlers for messages.
#if QT_VERSION >= 0x050000
    static void debugHandler(QtMsgType type,
                             const QMessageLogContext &placement,
                             const QString &message);
#else
    static void debugHandler(QtMsgType type,
                             const char *message);
#endif

  public slots:
    // Should be always called on GUI thread which is enforced
    // by signal/slot auto connection.
    void consumeDebugMessage(QtMsgType type, const QString &message);

  signals:
    // Emitted if new debug messaages is produced and should be printed.
    void debugMessageProduced(QtMsgType type, QString message);

  protected:
    // Catch the "press any key event" to exit the updater.
    void keyPressEvent(QKeyEvent *event);

    // Moves the window into the center of the screen and resizes it.
    void moveToCenterAndResize();

    // File/directory manipulators.
    bool copyDirectory(QString source, QString destination);
    bool removeDirectory(const QString & directory_name,
                         const QStringList &exception_file_list = QStringList(),
                         const QStringList &exception_folder_list = QStringList());

  private:
    UpdaterState m_state;
    QTextEdit *m_txtOutput;
    QHash<QString, QString> m_parsedArguments;

    static FormUpdater *s_instance;
};

#endif // FORMUPDATER_H
