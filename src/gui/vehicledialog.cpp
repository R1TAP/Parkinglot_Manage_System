#include "gui/vehicledialog.h"
#include "ui_vehicledialog.h"

VehicleDialog::VehicleDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::VehicleDialog)
{
    ui->setupUi(this);
    ui->comboBoxType->addItems({"燃油车", "新能源", "大型车"});
}

VehicleDialog::~VehicleDialog()
{
    delete ui;
}

Vehicle VehicleDialog::getVehicle() const
{
    Vehicle v;
    v.plate = ui->lineEditPlate->text();
    v.owner = ui->lineEditOwner->text();
    v.entryTime = QDateTime::currentDateTime();

    int currentIndex = ui->comboBoxType->currentIndex();
    if (currentIndex == 0) {
        v.vehicle_type = "蓝";
        v.vehicle_color = "蓝";
    } else if (currentIndex == 1) {
        v.vehicle_type = "绿";
        v.vehicle_color = "绿";
    } else if (currentIndex == 2) {
        v.vehicle_type = "黄";
        v.vehicle_color = "黄";
    }

    return v;
}



void VehicleDialog::setVehicle(const Vehicle &vehicle)
{
    ui->lineEditPlate->setText(vehicle.plate);
    ui->lineEditPlate->setReadOnly(true); // Plate is a primary key, should not be editable.
    ui->lineEditOwner->setText(vehicle.owner);

    if (vehicle.vehicle_type == "蓝") {
        ui->comboBoxType->setCurrentIndex(0);
    } else if (vehicle.vehicle_type == "绿") {
        ui->comboBoxType->setCurrentIndex(1);
    } else if (vehicle.vehicle_type == "黄") {
        ui->comboBoxType->setCurrentIndex(2);
    }
}


