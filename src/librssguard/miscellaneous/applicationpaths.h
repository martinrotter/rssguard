// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef APPLICATIONPATHS_H
#define APPLICATIONPATHS_H

#include <QString>
#include <QStringList>

class Application;

class ApplicationPaths {
  public:
    explicit ApplicationPaths(Application* application);

    QString tempFolder() const;
    QString documentsFolder() const;
    QString homeFolder() const;
    QString configFolder() const;
    QString userDataAppFolder() const;
    QString userDataHomeFolder() const;
    QString customDataFolder() const;
    QString userDataFolder() const;
    QString replaceUserDataFolderPlaceholder(QString text, bool double_escape) const;
    QStringList replaceUserDataFolderPlaceholder(QStringList texts) const;
    bool setCustomDataFolder(const QString& data_folder);

  private:
    Application* m_application;
    QString m_customDataFolder;
};

#endif // APPLICATIONPATHS_H
