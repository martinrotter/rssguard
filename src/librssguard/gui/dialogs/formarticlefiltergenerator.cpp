// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/dialogs/formarticlefiltergenerator.h"

#include "filtering/filterobjects.h"
#include "gui/guiutilities.h"
#include "gui/reusable/helpspoiler.h"
#include "gui/reusable/labelwithstatus.h"
#include "gui/reusable/jssyntaxhighlighter.h"
#include "gui/reusable/plaintoolbutton.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJSEngine>
#include <QJSValue>
#include <QLineEdit>
#include <QLocale>
#include <QMetaMethod>
#include <QMetaProperty>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QtAlgorithms>

#include <functional>
#include <utility>

namespace {

QString jsStringLiteral(const QString& value) {
  const QByteArray json_array = QJsonDocument(QJsonArray({value})).toJson(QJsonDocument::JsonFormat::Compact);
  const QString literal = QString::fromUtf8(json_array);

  return literal.mid(1, literal.size() - 2);
}

QString displayName(const QString& identifier) {
  QString result;

  for (const QChar character : identifier) {
    if (!result.isEmpty() && character.isUpper()) {
      result.append(QLatin1Char(' '));
    }

    result.append(character);
  }

  return result.left(1).toUpper() + result.mid(1);
}

bool isSupportedFieldType(int type) {
  return type == QMetaType::Type::QString || type == QMetaType::Type::Bool || type == QMetaType::Type::Int ||
         type == QMetaType::Type::Double || type == QMetaType::Type::QDateTime;
}

bool isTextField(int type) {
  return type == QMetaType::Type::QString;
}

bool isBooleanField(int type) {
  return type == QMetaType::Type::Bool;
}

bool isDateTimeField(int type) {
  return type == QMetaType::Type::QDateTime;
}

bool isNumericField(int type) {
  return type == QMetaType::Type::Int || type == QMetaType::Type::Double;
}

} // namespace

class FormArticleFilterGenerator::ConditionRow : public QWidget {
  public:
    ConditionRow(const QList<Field>& fields, QWidget* parent) : QWidget(parent), m_fields(fields) {
      auto* layout = new QHBoxLayout(this);
      layout->setContentsMargins(0, 0, 0, 0);

      m_field = new QComboBox(this);
      m_operator = new QComboBox(this);
      m_value = new QLineEdit(this);
      m_boolValue = new QComboBox(this);
      m_caseSensitive = new QCheckBox(tr("Case sensitive"), this);
      auto* remove = new PlainToolButton(this);

      for (int index = 0; index < m_fields.size(); index++) {
        const Field& field = m_fields.at(index);
        m_field->addItem(field.m_title, index);

        if (field.m_expression == QSL("msg.title")) {
          m_field->setCurrentIndex(m_field->count() - 1);
        }
      }

      m_boolValue->addItem(tr("true"), true);
      m_boolValue->addItem(tr("false"), false);
      remove->setIcon(qApp->icons()->fromTheme(QSL("list-remove")));
      remove->setToolTip(tr("Remove condition"));

      layout->addWidget(m_field, 2);
      layout->addWidget(m_operator, 2);
      layout->addWidget(m_value, 3);
      layout->addWidget(m_boolValue, 1);
      layout->addWidget(m_caseSensitive);
      layout->addWidget(remove);

      configure();

      connect(m_field,
              static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
              this,
              [this]() {
                configure();
                changed();
              });
      connect(m_operator, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [this]() {
        changed();
      });
      connect(m_value, &QLineEdit::textChanged, this, [this]() {
        changed();
      });
      connect(m_boolValue, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [this]() {
        changed();
      });
      connect(m_caseSensitive, &QCheckBox::toggled, this, [this]() {
        changed();
      });
      connect(remove, &PlainToolButton::clicked, this, [this]() {
        if (m_removeCallback) {
          m_removeCallback();
        }
      });
    }

    void setChangedCallback(std::function<void()> callback) {
      m_changedCallback = std::move(callback);
    }

    void setRemoveCallback(std::function<void()> callback) {
      m_removeCallback = std::move(callback);
    }

    QString expression(QString* error) const {
      const Field& selected_field = field();
      const QString operation = m_operator->currentData().toString();

      if (isBooleanField(selected_field.m_type)) {
        const QString value = m_boolValue->currentData().toBool() ? QSL("true") : QSL("false");

        return QSL("%1 %2 %3")
          .arg(selected_field.m_expression, operation == QSL("notEquals") ? QSL("!=") : QSL("=="), value);
      }

      if (isDateTimeField(selected_field.m_type)) {
        bool valid = false;
        const int days = m_value->text().toInt(&valid);

        if (!valid || days < 0) {
          if (error != nullptr) {
            *error = tr("Enter a non-negative age in days for '%1'.").arg(selected_field.m_title);
          }

          return {};
        }

        const QString age = QSL("Date.now() - %1.getTime()").arg(selected_field.m_expression);
        const QString milliseconds = QString::number(qint64(days) * 24 * 60 * 60 * 1000);

        return operation == QSL("older") ? QSL("%1 > %2").arg(age, milliseconds)
                                          : QSL("%1 <= %2").arg(age, milliseconds);
      }

      if (isNumericField(selected_field.m_type)) {
        bool valid = false;
        const double value = selected_field.m_type == QMetaType::Type::Int ? m_value->text().toInt(&valid)
                                                                             : m_value->text().toDouble(&valid);

        if (!valid) {
          if (error != nullptr) {
            *error = tr("Enter a valid number for '%1'.").arg(selected_field.m_title);
          }

          return {};
        }

        const QString number = QLocale::c().toString(value, 'g', 15);
        QString comparison;

        if (operation == QSL("equals")) {
          comparison = QSL("==");
        }
        else if (operation == QSL("notEquals")) {
          comparison = QSL("!=");
        }
        else if (operation == QSL("greater")) {
          comparison = QSL(">");
        }
        else if (operation == QSL("greaterEquals")) {
          comparison = QSL(">=");
        }
        else if (operation == QSL("less")) {
          comparison = QSL("<");
        }
        else {
          comparison = QSL("<=");
        }

        return QSL("%1 %2 %3").arg(selected_field.m_expression, comparison, number);
      }

      const QString value = m_value->text();

      if (value.isEmpty()) {
        if (error != nullptr) {
          *error = tr("Enter text to match for '%1'.").arg(selected_field.m_title);
        }

        return {};
      }

      if (operation == QSL("regex")) {
        const QString flags = m_caseSensitive->isChecked() ? QSL("\"\"") : QSL("\"i\"");
        QJSEngine engine;
        const QJSValue regular_expression =
          engine.evaluate(QSL("new RegExp(%1, %2)").arg(jsStringLiteral(value), flags));

        if (regular_expression.isError()) {
          if (error != nullptr) {
            *error = tr("The regular expression for '%1' is invalid: %2")
                       .arg(selected_field.m_title, regular_expression.toString());
          }

          return {};
        }

        return QSL("new RegExp(%1, %2).test(String(%3))").arg(jsStringLiteral(value), flags, selected_field.m_expression);
      }

      const QString string_expression =
        m_caseSensitive->isChecked() ? QSL("String(%1)").arg(selected_field.m_expression)
                                     : QSL("String(%1).toLowerCase()").arg(selected_field.m_expression);
      const QString literal = jsStringLiteral(m_caseSensitive->isChecked() ? value : value.toLower());

      if (operation == QSL("contains")) {
        return QSL("%1.indexOf(%2) >= 0").arg(string_expression, literal);
      }
      else if (operation == QSL("notContains")) {
        return QSL("%1.indexOf(%2) < 0").arg(string_expression, literal);
      }
      else if (operation == QSL("equals")) {
        return QSL("%1 == %2").arg(string_expression, literal);
      }
      else if (operation == QSL("notEquals")) {
        return QSL("%1 != %2").arg(string_expression, literal);
      }
      else if (operation == QSL("startsWith")) {
        return QSL("%1.startsWith(%2)").arg(string_expression, literal);
      }
      else {
        return QSL("%1.endsWith(%2)").arg(string_expression, literal);
      }
    }

  private:
    void configure() {
      const Field& selected_field = field();
      const QString previous_operator = m_operator->currentData().toString();

      m_operator->clear();
      m_caseSensitive->setVisible(isTextField(selected_field.m_type));
      m_boolValue->setVisible(isBooleanField(selected_field.m_type));
      m_value->setVisible(!isBooleanField(selected_field.m_type));

      if (isTextField(selected_field.m_type)) {
        m_operator->addItem(tr("contains"), QSL("contains"));
        m_operator->addItem(tr("does not contain"), QSL("notContains"));
        m_operator->addItem(tr("equals"), QSL("equals"));
        m_operator->addItem(tr("does not equal"), QSL("notEquals"));
        m_operator->addItem(tr("starts with"), QSL("startsWith"));
        m_operator->addItem(tr("ends with"), QSL("endsWith"));
        m_operator->addItem(tr("matches regular expression"), QSL("regex"));
        m_value->setPlaceholderText(tr("Text to match"));
      }
      else if (isBooleanField(selected_field.m_type)) {
        m_operator->addItem(tr("is"), QSL("equals"));
        m_operator->addItem(tr("is not"), QSL("notEquals"));
      }
      else if (isNumericField(selected_field.m_type)) {
        m_operator->addItem(tr("equals"), QSL("equals"));
        m_operator->addItem(tr("does not equal"), QSL("notEquals"));
        m_operator->addItem(tr("is greater than"), QSL("greater"));
        m_operator->addItem(tr("is at least"), QSL("greaterEquals"));
        m_operator->addItem(tr("is less than"), QSL("less"));
        m_operator->addItem(tr("is at most"), QSL("lessEquals"));
        m_value->setPlaceholderText(tr("Number"));
      }
      else {
        m_operator->addItem(tr("is older than"), QSL("older"));
        m_operator->addItem(tr("is newer than"), QSL("newer"));
        m_value->setPlaceholderText(tr("Age in days"));
      }

      const int previous_index = m_operator->findData(previous_operator);

      if (previous_index >= 0) {
        m_operator->setCurrentIndex(previous_index);
      }
    }

    const Field& field() const {
      return m_fields.at(m_field->currentData().toInt());
    }

    void changed() {
      if (m_changedCallback) {
        m_changedCallback();
      }
    }

  private:
    const QList<Field>& m_fields;
    QComboBox* m_field;
    QComboBox* m_operator;
    QLineEdit* m_value;
    QComboBox* m_boolValue;
    QCheckBox* m_caseSensitive;
    std::function<void()> m_changedCallback;
    std::function<void()> m_removeCallback;
};

class FormArticleFilterGenerator::ActionRow : public QWidget {
  public:
    ActionRow(const QList<Action>& actions, const QList<Field>& writable_fields, QWidget* parent)
      : QWidget(parent), m_actions(actions), m_writableFields(writable_fields) {
      auto* layout = new QHBoxLayout(this);
      layout->setContentsMargins(0, 0, 0, 0);

      m_action = new QComboBox(this);
      m_field = new QComboBox(this);
      m_value = new QLineEdit(this);
      m_boolValue = new QComboBox(this);
      auto* remove = new PlainToolButton(this);

      for (int index = 0; index < m_actions.size(); index++) {
        const Action& action = m_actions.at(index);
        m_action->addItem(action.m_title, index);
      }

      m_boolValue->addItem(tr("true"), true);
      m_boolValue->addItem(tr("false"), false);
      remove->setIcon(qApp->icons()->fromTheme(QSL("list-remove")));
      remove->setToolTip(tr("Remove action"));

      layout->addWidget(m_action, 2);
      layout->addWidget(m_field, 2);
      layout->addWidget(m_value, 3);
      layout->addWidget(m_boolValue, 1);
      layout->addWidget(remove);

      configure();

      connect(m_action,
              static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
              this,
              [this]() {
                configure();
                changed();
              });
      connect(m_field, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [this]() {
        configure();
        changed();
      });
      connect(m_value, &QLineEdit::textChanged, this, [this]() {
        changed();
      });
      connect(m_boolValue, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [this]() {
        changed();
      });
      connect(remove, &PlainToolButton::clicked, this, [this]() {
        if (m_removeCallback) {
          m_removeCallback();
        }
      });
    }

    void setChangedCallback(std::function<void()> callback) {
      m_changedCallback = std::move(callback);
    }

    void setRemoveCallback(std::function<void()> callback) {
      m_removeCallback = std::move(callback);
    }

    QString statement(int index, const QString& branch, const QString& indentation, QString* error) const {
      const Action& selected_action = action();

      if (selected_action.m_valueMode == QSL("property")) {
        const Field& selected_field = field();
        const QString value = isBooleanField(selected_field.m_type)
                                ? (m_boolValue->currentData().toBool() ? QSL("true") : QSL("false"))
                                : m_value->text();
        const QString expression = valueExpression(selected_field, value, error);

        return expression.isEmpty() && error != nullptr && !error->isEmpty()
                 ? QString()
                 : QSL("%1%2 = %3;").arg(indentation, selected_field.m_expression, expression);
      }

      if (selected_action.m_valueMode == QSL("label")) {
        const QString label_title = m_value->text();

        if (label_title.isEmpty()) {
          if (error != nullptr) {
            *error = tr("Enter a label title for '%1'.").arg(selected_action.m_title);
          }

          return {};
        }

        const QString label_id = branch + QSL("LabelId%1").arg(index + 1);

        return QSL("%1const %2 = acc.findLabel(%3);\n"
                   "%1if (%2) {\n"
                   "%4  msg.%5(%2);\n"
                   "%1}")
          .arg(indentation, label_id, jsStringLiteral(label_title), indentation + QSL("  "), selected_action.m_method);
      }

      if (selected_action.m_valueMode.startsWith(QSL("literal:"))) {
        return QSL("%1msg.%2(%3);").arg(indentation, selected_action.m_method, selected_action.m_valueMode.mid(8));
      }

      return QSL("%1msg.%2();").arg(indentation, selected_action.m_method);
    }

  private:
    void configure() {
      const Action& selected_action = action();
      const bool is_property = selected_action.m_valueMode == QSL("property");

      if (is_property && m_field->count() == 0) {
        for (int index = 0; index < m_writableFields.size(); index++) {
          const Field& available_field = m_writableFields.at(index);
          m_field->addItem(available_field.m_title, index);

          if (available_field.m_expression == QSL("msg.isImportant")) {
            m_field->setCurrentIndex(m_field->count() - 1);
          }
        }
      }

      const bool uses_boolean = is_property && isBooleanField(field().m_type);
      const bool needs_text_value = is_property ? !uses_boolean : selected_action.m_valueMode == QSL("label");

      m_field->setVisible(is_property);
      m_boolValue->setVisible(uses_boolean);
      m_value->setVisible(needs_text_value);
      m_value->setPlaceholderText(selected_action.m_valueMode == QSL("label") ? tr("Label title") : tr("Value"));
    }

    const Action& action() const {
      return m_actions.at(m_action->currentData().toInt());
    }

    const Field& field() const {
      return m_writableFields.at(m_field->currentData().toInt());
    }

    QString valueExpression(const Field& field, const QString& value, QString* error) const {
      if (isTextField(field.m_type)) {
        return jsStringLiteral(value);
      }

      if (isBooleanField(field.m_type)) {
        return value == QSL("true") ? QSL("true") : QSL("false");
      }

      bool valid = false;
      const double number = field.m_type == QMetaType::Type::Int ? value.toInt(&valid) : value.toDouble(&valid);

      if (!valid) {
        if (error != nullptr) {
          *error = tr("Enter a valid value for '%1'.").arg(field.m_title);
        }

        return {};
      }

      return field.m_type == QMetaType::Type::Int ? QString::number(int(number))
                                                   : QLocale::c().toString(number, 'g', 15);
    }

    void changed() {
      if (m_changedCallback) {
        m_changedCallback();
      }
    }

  private:
    const QList<Action>& m_actions;
    const QList<Field>& m_writableFields;
    QComboBox* m_action;
    QComboBox* m_field;
    QLineEdit* m_value;
    QComboBox* m_boolValue;
    std::function<void()> m_changedCallback;
    std::function<void()> m_removeCallback;
};

QString FormArticleFilterGenerator::generate() {
  FormArticleFilterGenerator generator;

  return generator.exec() == QDialog::DialogCode::Accepted ? generator.m_generatedScript : QString();
}

FormArticleFilterGenerator::FormArticleFilterGenerator() : QDialog(nullptr) {
  m_ui.setupUi(this);
  GuiUtilities::applyDialogProperties(*this, qApp->icons()->fromTheme(QSL("document-new"), QSL("list-add")));
  m_ui.m_cmbDecision->setItemData(0, QSL("Msg.Accept"));
  m_ui.m_cmbDecision->setItemData(1, QSL("Msg.Ignore"));
  m_ui.m_cmbDecision->setItemData(2, QSL("Msg.Purge"));
  m_ui.m_cmbFallbackDecision->setItemData(0, QSL("Msg.Accept"));
  m_ui.m_cmbFallbackDecision->setItemData(1, QSL("Msg.Ignore"));
  m_ui.m_cmbFallbackDecision->setItemData(2, QSL("Msg.Purge"));
  m_ui.m_cmbFallbackDecision->setCurrentIndex(1);
  m_ui.m_helpDescription->setHelpText(
    tr("About the visual filter generator"),
    tr("<p><b>Build common article filters without writing JavaScript.</b> "
       "Choose actions and results for both matching and non-matching articles.</p>"
       "<ul>"
       "<li><b>All conditions</b> requires every condition to match; <b>at least one condition</b> matches any of them.</li>"
       "<li>Each outcome can change article properties or labels before its result is applied.</li>"
       "<li>The generated script remains editable, testable, and can be expanded with advanced JavaScript features.</li>"
       "</ul>"
       "<p>See the <a href=\"https://rssguard.readthedocs.io/en/stable/features/filters.html\">"
       "article-filter documentation</a> for the complete scripting reference.</p>"),
    false,
    true);

  auto addProperties = [this](const QMetaObject& meta_object, const QString& prefix, bool allow_writing) {
    for (int index = meta_object.propertyOffset(); index < meta_object.propertyCount(); index++) {
      const QMetaProperty property = meta_object.property(index);
      const int type = property.metaType().id();

      if (!isSupportedFieldType(type)) {
        continue;
      }

      const Field field = {prefix + QLatin1Char('.') + QString::fromLatin1(property.name()),
                           displayName(QString::fromLatin1(property.name())),
                           type,
                           allow_writing && property.isWritable()};

      m_fields.append(field);

      if (field.m_writable && !isDateTimeField(field.m_type)) {
        m_writableMessageFields.append(field);
      }
    }
  };

  addProperties(FilterMessage::staticMetaObject, QSL("msg"), true);
  addProperties(FilterFeed::staticMetaObject, QSL("feed"), false);
  addProperties(FilterAccount::staticMetaObject, QSL("acc"), false);

  m_actions.append({QSL("setProperty"), QSL("property"), tr("Set article property")});

  const QMetaObject& message_meta_object = FilterMessage::staticMetaObject;
  const int class_info_index = message_meta_object.indexOfClassInfo("articleFilterGeneratorActions");

  if (class_info_index >= 0) {
    const QStringList action_definitions =
      QString::fromLatin1(message_meta_object.classInfo(class_info_index).value()).split(QLatin1Char(';'));

    for (const QString& definition : action_definitions) {
      const QStringList parts = definition.split(QLatin1Char('|'));

      if (parts.size() != 3) {
        continue;
      }

      const QByteArray method_name = parts.at(0).toLatin1();
      bool found_invokable = false;

      for (int index = message_meta_object.methodOffset(); index < message_meta_object.methodCount(); index++) {
        const QMetaMethod method = message_meta_object.method(index);

        if (method.name() == method_name && method.methodType() == QMetaMethod::Method &&
            method.access() == QMetaMethod::Access::Public) {
          found_invokable = true;
          break;
        }
      }

      if (found_invokable) {
        m_actions.append({parts.at(0), parts.at(1), parts.at(2)});
      }
    }
  }

  m_ui.m_txtPreview->setFont(QFontDatabase::systemFont(QFontDatabase::SystemFont::FixedFont));
  new JsSyntaxHighlighter(m_ui.m_txtPreview->document());
  m_ui.m_btnAddCondition->setIcon(qApp->icons()->fromTheme(QSL("list-add")));
  m_ui.m_btnAddMatchingAction->setIcon(qApp->icons()->fromTheme(QSL("list-add")));
  m_ui.m_btnAddFallbackAction->setIcon(qApp->icons()->fromTheme(QSL("list-add")));

  connect(m_ui.m_btnAddCondition, &QPushButton::clicked, this, &FormArticleFilterGenerator::addCondition);
  connect(m_ui.m_btnAddMatchingAction, &QPushButton::clicked, this, [this]() {
    addAction(false);
  });
  connect(m_ui.m_btnAddFallbackAction, &QPushButton::clicked, this, [this]() {
    addAction(true);
  });
  connect(m_ui.m_cmbMatchMode,
          static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
          this,
          &FormArticleFilterGenerator::updatePreview);
  connect(m_ui.m_cmbDecision,
          static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
          this,
          &FormArticleFilterGenerator::updatePreview);
  connect(m_ui.m_cmbFallbackDecision,
          static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
          this,
          &FormArticleFilterGenerator::updatePreview);
  connect(m_ui.m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
  connect(m_ui.m_buttonBox, &QDialogButtonBox::accepted, this, [this]() {
    QString error;
    const QString script = buildScript(&error);

    if (!error.isEmpty()) {
      updatePreview();
      return;
    }

    m_generatedScript = script;
    accept();
  });

  addCondition();
  updateActionAvailability();
  m_ui.m_lblError->hide();
  updatePreview();
}

FormArticleFilterGenerator::~FormArticleFilterGenerator() {
  qDeleteAll(m_conditionRows);
  qDeleteAll(m_matchingActionRows);
  qDeleteAll(m_fallbackActionRows);
}

void FormArticleFilterGenerator::addCondition() {
  auto* row = new ConditionRow(m_fields, m_ui.m_conditionsContainer);

  m_ui.m_conditionsLayout->insertWidget(m_ui.m_conditionsLayout->count() - 1, row);
  m_conditionRows.append(row);
  updateActionAvailability();

  row->setChangedCallback([this]() {
    updatePreview();
  });
  row->setRemoveCallback([this, row]() {
    // Do not delete a row while its remove button is still dispatching its click signal.
    QTimer::singleShot(0, this, [this, row]() {
      if (m_conditionRows.removeOne(row)) {
        m_ui.m_conditionsLayout->removeWidget(row);
        delete row;
        updateActionAvailability();
        updatePreview();
      }
    });
  });
}

void FormArticleFilterGenerator::addAction(bool fallback) {
  auto* actions_layout = fallback ? m_ui.m_fallbackActionsLayout : m_ui.m_actionsLayout;
  auto* actions_container = fallback ? m_ui.m_fallbackActionsContainer : m_ui.m_actionsContainer;
  auto* action_rows = fallback ? &m_fallbackActionRows : &m_matchingActionRows;
  auto* row = new ActionRow(m_actions, m_writableMessageFields, actions_container);

  actions_layout->insertWidget(actions_layout->count() - 1, row);
  action_rows->append(row);

  row->setChangedCallback([this]() {
    updatePreview();
  });
  row->setRemoveCallback([this, row, actions_layout, action_rows]() {
    // Do not delete a row while its remove button is still dispatching its click signal.
    QTimer::singleShot(0, this, [this, row, actions_layout, action_rows]() {
      if (action_rows->removeOne(row)) {
        actions_layout->removeWidget(row);
        delete row;
        updatePreview();
      }
    });
  });
}

void FormArticleFilterGenerator::updateActionAvailability() {
  const bool has_conditions = !m_conditionRows.isEmpty();

  m_ui.m_cmbMatchMode->setEnabled(has_conditions);
  m_ui.m_twOutcomes->setTabEnabled(1, has_conditions);

  if (!has_conditions) {
    m_ui.m_twOutcomes->setCurrentIndex(0);
  }
}

void FormArticleFilterGenerator::updatePreview() {
  QString error;
  const QString script = buildScript(&error);

  if (error.isEmpty()) {
    m_ui.m_lblError->hide();
  }
  else {
    m_ui.m_lblError->setStatus(WidgetWithStatus::StatusType::Error, error, error);
    m_ui.m_lblError->show();
  }

  m_ui.m_txtPreview->setPlainText(script);
  m_ui.m_buttonBox->button(QDialogButtonBox::StandardButton::Ok)->setEnabled(error.isEmpty());
}

QString FormArticleFilterGenerator::buildScript(QString* error) const {
  QStringList conditions;

  for (const ConditionRow* row : m_conditionRows) {
    const QString condition = row->expression(error);

    if (condition.isEmpty()) {
      return {};
    }

    conditions.append(QSL("(%1)").arg(condition));
  }

  const bool match_all = m_ui.m_cmbMatchMode->currentIndex() == 0;
  const QString joiner = match_all ? QSL("\n    && ") : QSL("\n    || ");
  const QString decision = m_ui.m_cmbDecision->currentData().toString();
  const QString fallback_decision = m_ui.m_cmbFallbackDecision->currentData().toString();

  auto append_actions = [error](QStringList& lines,
                                const QList<ActionRow*>& actions,
                                const QString& branch,
                                const QString& indentation) {
    for (int index = 0; index < actions.size(); index++) {
      const QString action = actions.at(index)->statement(index, branch, indentation, error);

      if (action.isEmpty() && error != nullptr && !error->isEmpty()) {
        return false;
      }

      if (!action.isEmpty()) {
        lines.append(action);
      }
    }

    return true;
  };

  if (conditions.isEmpty()) {
    QStringList lines = {QSL("/*"),
                         QSL(" * Generated by RSS Guard Article Filter Generator."),
                         QSL(" * You can edit this script manually after generation."),
                         QSL(" */"),
                         QSL("function filterMessage() {")};

    if (!append_actions(lines, m_matchingActionRows, QSL("matching"), QSL("  "))) {
      return {};
    }

    lines << QSL("  return %1;").arg(decision) << QSL("}");
    return lines.join(QLatin1Char('\n'));
  }

  QStringList lines = {QSL("/*"),
                       QSL(" * Generated by RSS Guard Article Filter Generator."),
                       QSL(" * You can edit this script manually after generation."),
                       QSL(" */"),
                       QSL("function filterMessage() {"),
                       QSL("  const matches =\n    %1;").arg(conditions.join(joiner)),
                       QString(),
                       QSL("  if (matches) {")};

  if (!append_actions(lines, m_matchingActionRows, QSL("matching"), QSL("    "))) {
    return {};
  }

  lines << QSL("    return %1;").arg(decision) << QSL("  }") << QString();

  if (!append_actions(lines, m_fallbackActionRows, QSL("fallback"), QSL("  "))) {
    return {};
  }

  lines << QSL("  return %1;").arg(fallback_decision)
        << QSL("}");

  return lines.join(QLatin1Char('\n'));
}
