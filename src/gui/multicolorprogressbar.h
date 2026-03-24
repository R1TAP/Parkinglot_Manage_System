#ifndef MULTICOLORPROGRESSBAR_H
#define MULTICOLORPROGRESSBAR_H

#include <QWidget>
#include <QMap>
#include <QColor>

class MultiColorProgressBar : public QWidget
{
    Q_OBJECT
public:
    explicit MultiColorProgressBar(QWidget *parent = nullptr);
    void setValues(const QMap<QString, int> &values, const QMap<QString, QColor> &colors);
    void setMaximum(int max);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QMap<QString, int> m_values;
    QMap<QString, QColor> m_colors;
    int m_maximum;
};

#endif // MULTICOLORPROGRESSBAR_H
