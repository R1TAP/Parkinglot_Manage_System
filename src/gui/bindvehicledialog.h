#ifndef BINDVEHICLEDIALOG_H
#define BINDVEHICLEDIALOG_H

#include <QDialog>

namespace Ui {
class BindVehicleDialog;
}

class BindVehicleDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BindVehicleDialog(QWidget *parent = nullptr);
    ~BindVehicleDialog();
    QString getPlateNumber() const;

private:
    Ui::BindVehicleDialog *ui;
};

#endif // BINDVEHICLEDIALOG_H
