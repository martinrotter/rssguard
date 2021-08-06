// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/reusable/baselineedit.h"

#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"

#include <QAction>
#include <QKeyEvent>

BaseLineEdit::BaseLineEdit(QWidget* parent)
  : QLineEdit(parent), m_actShowPassword(new QAction(qApp->icons()->fromTheme(QSL("dialog-password")),
                                                     tr("Show/hide the password"),
                                                     this)) {

  connect(m_actShowPassword, &QAction::triggered, this, [this]() {
    setEchoMode(echoMode() == QLineEdit::EchoMode::Password
                ? QLineEdit::EchoMode::Normal
                : QLineEdit::EchoMode::Password);
  });
  connect(this, &QLineEdit::textChanged, this, [this](const QString& text) {
    if (actions().contains(m_actShowPassword)) {
      m_actShowPassword->setVisible(!text.isEmpty());
    }
  });

  setClearButtonEnabled(true);
}

void BaseLineEdit::setPasswordMode(bool is_password) {
  if (is_password) {
    setEchoMode(QLineEdit::EchoMode::Password);
    addAction(m_actShowPassword, QLineEdit::ActionPosition::LeadingPosition);
  }
  else {
    setEchoMode(QLineEdit::EchoMode::Normal);
    removeAction(m_actShowPassword);
  }

  emit textChanged(text());
}

void BaseLineEdit::submit(const QString& text) {
  setText(text);
  emit submitted(text);
}

void BaseLineEdit::keyPressEvent(QKeyEvent* event) {
  if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
    emit submitted(text());

    event->accept();
  }

  QLineEdit::keyPressEvent(event);
}
