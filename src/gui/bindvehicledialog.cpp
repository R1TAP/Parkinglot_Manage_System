#include "gui/bindvehicledialog.h"
#include "ui_bindvehicledialog.h"

BindVehicleDialog::BindVehicleDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BindVehicleDialog)
{
    ui->setupUi(this);

    connect(ui->pushButtonBind, &QPushButton::clicked, this, &BindVehicleDialog::accept);
    connect(ui->pushButtonCancel, &QPushButton::clicked, this, &BindVehicleDialog::reject);

    ui->pushButtonBind->setObjectName("pushButtonPay"); // Green
    ui->pushButtonCancel->setObjectName("msgBoxCancelButton"); // Red
}

BindVehicleDialog::~BindVehicleDialog()
{
    delete ui;
}

QString BindVehicleDialog::getPlateNumber() const
{
    return ui->lineEditPlate->text();
}
