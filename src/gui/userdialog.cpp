#include "gui/userdialog.h"
#include "ui_userdialog.h"

UserDialog::UserDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::UserDialog)
{
    ui->setupUi(this);
    ui->comboBoxRole->addItem("管理员", static_cast<int>(UserRole::Admin));
    ui->comboBoxRole->addItem("访客", static_cast<int>(UserRole::Visitor));

    // Manually connect signals and slots
    connect(ui->pushButtonOK, &QPushButton::clicked, this, &UserDialog::accept);
    connect(ui->pushButtonCancel, &QPushButton::clicked, this, &UserDialog::reject);

    // Set object names for styling
    ui->pushButtonOK->setObjectName("msgBoxConfirmButton");
    ui->pushButtonCancel->setObjectName("msgBoxCancelButton");
}

UserDialog::~UserDialog()
{
    delete ui;
}

User UserDialog::getUser() const
{
    User user;
    user.username = ui->lineEditUser->text();
    user.password = ui->lineEditPassword->text();
    user.role = static_cast<UserRole>(ui->comboBoxRole->currentData().toInt());
    return user;
}

void UserDialog::setUser(const User &user)
{
    ui->lineEditUser->setText(user.username);
    ui->lineEditUser->setReadOnly(true); // Username is primary key, should not be changed
    ui->lineEditPassword->clear(); // 密码不回显；留空提交 = 保持原密码
    int index = ui->comboBoxRole->findData(static_cast<int>(user.role));
    if (index != -1) {
        ui->comboBoxRole->setCurrentIndex(index);
    }
}
