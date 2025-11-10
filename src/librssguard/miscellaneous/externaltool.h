// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef EXTERNALTOOL_H
#define EXTERNALTOOL_H

#include <QMetaType>
#include <QStringList>

class ExternalTool {
  public:
    explicit ExternalTool() = default;
    ExternalTool(const ExternalTool& other);
    explicit ExternalTool(QString name, QString executable, QString parameters);

    QString executable() const;
    QString parameters() const;
    QString name() const;

    bool run(const QString& target);

    QByteArray toString();

  public:
    static ExternalTool fromString(const QByteArray& str);
    static QList<ExternalTool> toolsFromSettings();
    static void setToolsToSettings(QVector<ExternalTool>& tools);

  private:
    QString m_name;
    QString m_executable;
    QString m_parameters;

    void sanitizeParameters();
};

Q_DECLARE_METATYPE(ExternalTool)

#endif // EXTERNALTOOL_H
