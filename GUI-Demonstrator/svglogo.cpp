#include "svglogo.h"

#include <QDebug>
#include <QPainter>
#include <QSvgRenderer>

SvgLogo::SvgLogo(QWidget *parent)
    : QWidget(parent)
    , m_renderer(new QSvgRenderer(this))
{
    setAttribute(Qt::WA_TranslucentBackground);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    updateSize();
}

bool SvgLogo::setSource(const QString &path)
{
    if (!m_renderer->load(path)) {
        qWarning().noquote() << "Logo nicht gefunden oder ungueltig:" << path;
        updateSize();
        return false;
    }

    m_renderer->setAspectRatioMode(Qt::KeepAspectRatio);
    updateSize();
    update();
    return true;
}

void SvgLogo::setLogoHeight(int height)
{
    m_height = qMax(1, height);
    updateSize();
}

void SvgLogo::setDescription(const QString &text)
{
    setAccessibleName(text);
    setToolTip(text);
}

void SvgLogo::updateSize()
{
    if (!m_renderer->isValid()) {
        setFixedSize(0, 0);
        return;
    }

    const QSizeF native = m_renderer->viewBoxF().size();
    const double aspect = native.height() > 0.0 ? native.width() / native.height() : 1.0;
    setFixedSize(qRound(m_height * aspect), m_height);
}

void SvgLogo::paintEvent(QPaintEvent *)
{
    if (!m_renderer->isValid())
        return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    m_renderer->render(&painter, QRectF(rect()));
}
