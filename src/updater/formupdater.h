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

    // Debug handlers for messages.
#if QT_VERSION >= 0x050000
    static void debugHandler(QtMsgType type,
                             const QMessageLogContext &placement,
                             const QString &message);
#else
    static void debugHandler(QtMsgType type,
                             const char *message);
#endif

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
