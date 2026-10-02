// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/reusable/baselineedit.h"

#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"

#include <QAction>
#include <QContextMenuEvent>
#include <QKeyEvent>

BaseLineEdit::BaseLineEdit(QWidget* parent)
  : QLineEdit(parent), m_actShowPassword(new QAction(qApp->icons()->fromTheme(QSL("dialog-password")),
                                                     tr("Show/hide the password"),
                                                     this)) {
  m_tmrSelectAll.setSingleShot(true);

  connect(&m_tmrSelectAll, &QTimer::timeout, this, [this]() {
    if (hasFocus() && !hasSelectedText()) {
      selectAll();
    }
  });
  connect(m_actShowPassword, &QAction::triggered, this, [this]() {
    setEchoMode(echoMode() == QLineEdit::EchoMode::Password ? QLineEdit::EchoMode::Normal
                                                            : QLineEdit::EchoMode::Password);
  });
  connect(this, &QLineEdit::textChanged, this, [this](const QString& text) {
    if (actions().contains(m_actShowPassword)) {
      m_actShowPassword->setVisible(!text.isEmpty());
    }
  });

  setClearButtonEnabled(true);
}

BaseLineEdit::~BaseLineEdit() {}

void BaseLineEdit::focusInEvent(QFocusEvent* event) {
  QLineEdit::focusInEvent(event);

  if (event->reason() != Qt::PopupFocusReason && !hasSelectedText()) {
    m_tmrSelectAll.start(100);
  }
}

void BaseLineEdit::contextMenuEvent(QContextMenuEvent* event) {
  if (!hasFocus()) {
    setFocus(Qt::MouseFocusReason);
  }

  if (m_tmrSelectAll.isActive()) {
    m_tmrSelectAll.stop();

    if (!hasSelectedText()) {
      selectAll();
    }
  }

  QLineEdit::contextMenuEvent(event);
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
  if (event->key() == Qt::Key::Key_Enter || event->key() == Qt::Key::Key_Return) {
    emit submitted(text());
    event->accept();
  }

  if (event->key() == Qt::Key::Key_Escape) {
    submit(QString());
    event->accept();
  }

  QLineEdit::keyPressEvent(event);
}
