// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef EXTERNALTOOL_H
#define EXTERNALTOOL_H

#include <QMetaType>
#include <QStringList>

class ExternalTool {
  public:
    explicit ExternalTool() = default;
    ExternalTool(const ExternalTool& other);
    explicit ExternalTool(QString executable, QStringList parameters);

    QString toString();
    QString executable() const;
    QStringList parameters() const;

    static ExternalTool fromString(const QString& str);
    static QList<ExternalTool> toolsFromSettings();
    static void setToolsToSettings(QList<ExternalTool>& tools);

  private:
    QString m_executable;
    QStringList m_parameters;

    void sanitizeParameters();
};

Q_DECLARE_METATYPE(ExternalTool)

#endif // EXTERNALTOOL_H
