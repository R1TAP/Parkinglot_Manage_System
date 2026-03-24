#include "gui/monthpassdialog.h"
#include "ui_monthpassdialog.h"
#include <QButtonGroup>

MonthPassDialog::MonthPassDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MonthPassDialog),
    m_selectedDays(0)
{
    ui->setupUi(this);

    // Group the radio buttons to make them mutually exclusive
    m_buttonGroup = new QButtonGroup(this);
    m_buttonGroup->addButton(ui->radioButtonMonth);
    m_buttonGroup->addButton(ui->radioButtonQuarter);
    m_buttonGroup->addButton(ui->radioButtonYear);

    // Connections for OK and Cancel
    connect(ui->pushButtonOK, &QPushButton::clicked, this, &MonthPassDialog::on_pushButtonOK_clicked);
    connect(ui->pushButtonCancel, &QPushButton::clicked, this, &MonthPassDialog::on_pushButtonCancel_clicked);
}

MonthPassDialog::~MonthPassDialog()
{
    delete ui;
}

int MonthPassDialog::getSelectedDays()
{
    return m_selectedDays;
}

void MonthPassDialog::on_pushButtonOK_clicked()
{
    if (ui->radioButtonMonth->isChecked()) {
        m_selectedDays = 30;
    } else if (ui->radioButtonQuarter->isChecked()) {
        m_selectedDays = 90;
    } else if (ui->radioButtonYear->isChecked()) {
        m_selectedDays = 365;
    }
    accept();
}

void MonthPassDialog::on_pushButtonCancel_clicked()
{
    reject();
}
