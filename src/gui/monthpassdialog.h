#ifndef MONTHPASSDIALOG_H
#define MONTHPASSDIALOG_H

#include <QDialog>

class QButtonGroup;

namespace Ui {
class MonthPassDialog;
}

class MonthPassDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MonthPassDialog(QWidget *parent = nullptr);
    ~MonthPassDialog();

    int getSelectedDays();

private slots:
    void on_pushButtonOK_clicked();
    void on_pushButtonCancel_clicked();

private:
    Ui::MonthPassDialog *ui;
    QButtonGroup *m_buttonGroup;
    int m_selectedDays;
};

#endif // MONTHPASSDIALOG_H
