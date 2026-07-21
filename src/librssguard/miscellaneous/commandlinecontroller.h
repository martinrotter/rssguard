// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef COMMANDLINECONTROLLER_H
#define COMMANDLINECONTROLLER_H

#include <QCommandLineParser>
#include <QStringList>

class Application;
class ApplicationPaths;

class CommandLineController {
  public:
    CommandLineController(Application* application, ApplicationPaths* paths, const QStringList& raw_arguments);

    void parseMyArguments(const QStringList& arguments, QString& custom_user_agent);
    void parseOtherInstanceArguments(const QString& message);
    bool isAlreadyRunning();

    QCommandLineParser* parser();
    QStringList rawArguments() const;

  private:
    void fillParser(QCommandLineParser& parser) const;

  private:
    Application* m_application;
    ApplicationPaths* m_paths;
    QStringList m_rawArguments;
    QCommandLineParser m_parser;
    bool m_allowMultipleInstances;
};

#endif // COMMANDLINECONTROLLER_H
