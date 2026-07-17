// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMARTICLEFILTERGENERATOR_H
#define FORMARTICLEFILTERGENERATOR_H

#include "ui_formarticlefiltergenerator.h"

#include <QDialog>
#include <QList>

class QComboBox;

class FormArticleFilterGenerator : public QDialog {
    Q_OBJECT

  public:
    // Returns an empty string when the generator is cancelled.
    static QString generate();

    virtual ~FormArticleFilterGenerator();

  private:
    struct Field {
        QString m_expression;
        QString m_title;
        int m_type;
        bool m_writable;
    };

    struct Action {
        QString m_method;
        QString m_valueMode;
        QString m_title;
    };

    class ConditionRow;
    class ActionRow;

    FormArticleFilterGenerator();

    void addCondition();
    void addAction(bool fallback);
    void updatePreview();
    void updateActionAvailability();

    QString buildScript(QString* error = nullptr) const;

  private:
    Ui::FormArticleFilterGenerator m_ui;
    QList<Field> m_fields;
    QList<Field> m_writableMessageFields;
    QList<Action> m_actions;
    QList<ConditionRow*> m_conditionRows;
    QList<ActionRow*> m_matchingActionRows;
    QList<ActionRow*> m_fallbackActionRows;
    QString m_generatedScript;
};

#endif // FORMARTICLEFILTERGENERATOR_H
