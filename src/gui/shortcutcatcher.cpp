/******************************************************************************
Copyright (c) 2010, Artem Galichkin <doomer3d@gmail.com>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

#include <QHBoxLayout>
#include <QToolButton>

#include "gui/shortcutcatcher.h"
#include "gui/shortcutbutton.h"
#include "gui/iconthemefactory.h"


ShortcutCatcher::ShortcutCatcher(QWidget *parent)
  : QWidget(parent) {
  // Setup layout of the control
  m_layout = new QHBoxLayout(this);
  m_layout->setMargin(0);
  m_layout->setSpacing(1);

  // Create clear button.
  m_clearButton = new QToolButton(this);
  m_clearButton->setIcon(IconThemeFactory::getInstance()->fromTheme("document-revert"));
  m_clearButton->setFocusPolicy(Qt::NoFocus);
  m_clearButton->setToolTip(tr("Reset shortcut."));

  // Clear main shortcut catching button.
  m_sequenceButton = new ShortcutButton(this);
  m_sequenceButton->setFocusPolicy(Qt::StrongFocus);
  m_sequenceButton->setToolTip(tr("Set shortcut."));

  // Add both buttons to the layout.
  m_layout->addWidget(m_sequenceButton);
  m_layout->addWidget(m_clearButton);

  // Establish needed connections.
  connect(m_clearButton, SIGNAL(clicked()), this, SLOT(clearShortcut()));
  connect(m_sequenceButton, SIGNAL(clicked()), this, SLOT(startRecording()));

  // Prepare initial state of the control.
  updateDisplayShortcut();
}

ShortcutCatcher::~ShortcutCatcher() {
  delete m_clearButton;
  delete m_sequenceButton;
  delete m_layout;
}

void ShortcutCatcher::startRecording() {
  m_numKey = 0;
  m_modifierKeys = 0;
  m_currentSequence = QKeySequence();
  m_isRecording = true;
  m_sequenceButton->setDown(true);
  m_sequenceButton->grabKeyboard();

  updateDisplayShortcut();
}

void ShortcutCatcher::doneRecording() {
  m_isRecording = false;
  m_sequenceButton->releaseKeyboard();
  m_sequenceButton->setDown(false);

  updateDisplayShortcut();

  emit shortcutChanged(m_currentSequence);
}

void ShortcutCatcher::controlModifierlessTimout() {
  if (m_numKey && !m_modifierKeys) {
    doneRecording();
  }
}

void ShortcutCatcher::updateDisplayShortcut()
{
  QString str = m_currentSequence.toString(QKeySequence::NativeText);
  str.replace('&', QLatin1String("&&"));

  if (m_isRecording) {
    if (m_modifierKeys) {
      if (!str.isEmpty()) {
        str.append(",");
      }
      if (m_modifierKeys & Qt::META) {
        str += "Meta + ";
      }
      if (m_modifierKeys & Qt::CTRL) {
        str += "Ctrl + ";
      }
      if (m_modifierKeys & Qt::ALT) {
        str += "Alt + ";
      }
      if (m_modifierKeys & Qt::SHIFT) {
        str += "Shift + ";
      }
    }
  }

  m_sequenceButton->setText(str);
}

QKeySequence ShortcutCatcher::shortcut() const {
  return m_currentSequence;
}

void ShortcutCatcher::setShortcut(const QKeySequence &key) {
  m_currentSequence = m_defaultSequence = key;
  doneRecording();
}

void ShortcutCatcher::clearShortcut() {
  setShortcut(m_defaultSequence);
}
