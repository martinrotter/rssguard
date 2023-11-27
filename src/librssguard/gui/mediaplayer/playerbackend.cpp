// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/mediaplayer/playerbackend.h"

#include <QVBoxLayout>

PlayerBackend::PlayerBackend(QWidget* parent) : QWidget(parent), m_mainLayout(new QVBoxLayout(this)) {
  m_mainLayout->setSpacing(0);
  m_mainLayout->setContentsMargins({0, 0, 0, 0});
}
