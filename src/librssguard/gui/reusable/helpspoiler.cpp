// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/reusable/helpspoiler.h"

#include "definitions/definitions.h"
#include "gui/guiutilities.h"
#include "gui/reusable/plaintoolbutton.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"

#include <QGridLayout>
#include <QLabel>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QScrollArea>
#include <QToolButton>

HelpSpoiler::HelpSpoiler(QWidget* parent) : QWidget(parent),
  m_btnToggle(new QToolButton(this)), m_content(new QScrollArea(this)), m_animation(new QParallelAnimationGroup(this)),
  m_layout(new QGridLayout(this)), m_text(new QLabel(this)) {
  m_btnToggle->setStyleSheet(QSL("QToolButton { border: none; }"));
  m_btnToggle->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextBesideIcon);
  m_btnToggle->setArrowType(Qt::ArrowType::RightArrow);
  m_btnToggle->setText(tr("View more information on this"));
  m_btnToggle->setCheckable(true);
  m_btnToggle->setChecked(false);

  m_content->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
  m_content->setMaximumHeight(0);
  m_content->setMinimumHeight(0);

  m_animation->addAnimation(new QPropertyAnimation(this, QSL("minimumHeight").toLocal8Bit()));
  m_animation->addAnimation(new QPropertyAnimation(this, QSL("maximumHeight").toLocal8Bit()));
  m_animation->addAnimation(new QPropertyAnimation(m_content, QSL("maximumHeight").toLocal8Bit()));

  // don't waste space
  m_layout->setHorizontalSpacing(0);
  m_layout->setVerticalSpacing(0);
  m_layout->setContentsMargins(0, 0, 0, 0);

  PlainToolButton* btn_help = new PlainToolButton(this);

  btn_help->setPadding(0);
  btn_help->setIcon(qApp->icons()->fromTheme(QSL("dialog-question")));

  m_layout->addWidget(btn_help, 0, 0);
  m_layout->addWidget(m_btnToggle, 0, 1, 1, 1, Qt::AlignmentFlag::AlignLeft);
  m_layout->addWidget(m_content, 1, 0, 1, 2);

  QObject::connect(m_btnToggle, &QToolButton::clicked, [this](const bool checked) {
    m_btnToggle->setArrowType(checked
                              ? Qt::ArrowType::DownArrow
                              : Qt::ArrowType::RightArrow);
    m_animation->setDirection(checked
                              ? QAbstractAnimation::Direction::Forward
                              : QAbstractAnimation::Direction::Backward);
    m_animation->start();
  });

  m_text->setWordWrap(true);

  auto* content_layout = new QVBoxLayout(m_content);

  content_layout->addWidget(m_text);
}

void HelpSpoiler::setHelpText(const QString& text, bool is_warning) {
  m_text->setText(text);
  GuiUtilities::setLabelAsNotice(*m_text, is_warning, false);

  const auto collapsed_height = sizeHint().height() - m_content->maximumHeight();
  auto content_height = m_content->layout()->sizeHint().height();

  for (int i = 0; i < m_animation->animationCount() - 1; i++) {
    QPropertyAnimation* spoiler_animation = static_cast<QPropertyAnimation*>(m_animation->animationAt(i));

    spoiler_animation->setDuration(100);
    spoiler_animation->setStartValue(collapsed_height);
    spoiler_animation->setEndValue(collapsed_height + content_height);
  }

  QPropertyAnimation* content_animation = static_cast<QPropertyAnimation*>(m_animation->animationAt(m_animation->animationCount() - 1));

  content_animation->setDuration(100);
  content_animation->setStartValue(0);
  content_animation->setEndValue(content_height);
}
