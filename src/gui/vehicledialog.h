#ifndef VEHICLEDIALOG_H
#define VEHICLEDIALOG_H

#include <QDialog>
#include "core/vehicle.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class VehicleDialog;
}
QT_END_NAMESPACE

class VehicleDialog : public QDialog {
  Q_OBJECT

public:
  explicit VehicleDialog(QWidget *parent = nullptr);
  ~VehicleDialog();

    Vehicle getVehicle() const;
    void setVehicle(const Vehicle &vehicle);



private:
  Ui::VehicleDialog *ui;
};

#endif // VEHICLEDIALOG_H
