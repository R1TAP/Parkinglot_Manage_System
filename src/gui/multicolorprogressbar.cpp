#include "gui/multicolorprogressbar.h"
#include <QPainter>

MultiColorProgressBar::MultiColorProgressBar(QWidget *parent)
    : QWidget(parent), m_maximum(100)
{
}

void MultiColorProgressBar::setMaximum(int max)
{
    m_maximum = max > 0 ? max : 100;
    update();
}

void MultiColorProgressBar::setValues(const QMap<QString, int> &values, const QMap<QString, QColor> &colors)
{
    m_values = values;
    m_colors = colors;
    update();
}

void MultiColorProgressBar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (m_maximum <= 0) return;

    double totalValue = 0;
    for (int value : m_values.values()) {
        totalValue += value;
    }

    double width = (double)this->width();
    double currentX = 0;

    // Draw the colored segments
    for (auto it = m_values.constBegin(); it != m_values.constEnd(); ++it) {
        if (it.value() > 0) {
            double segmentWidth = width * (it.value() / (double)m_maximum);
            painter.setBrush(m_colors.value(it.key()));
            painter.setPen(Qt::NoPen);
            painter.drawRect(currentX, 0, segmentWidth, this->height());
            currentX += segmentWidth;
        }
    }

    // Draw the remaining empty part
    double remainingValue = m_maximum - totalValue;
    if (remainingValue > 0) {
        double segmentWidth = width * (remainingValue / (double)m_maximum);
        painter.setBrush(QColor("#e0e0e0")); // Gray color for empty space
        painter.setPen(Qt::NoPen);
        painter.drawRect(currentX, 0, segmentWidth, this->height());
    }
}
