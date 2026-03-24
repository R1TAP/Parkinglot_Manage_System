#include "gui/changepassworddialog.h"
#include "ui_changepassworddialog.h"
#include <QMessageBox>

ChangePasswordDialog::ChangePasswordDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ChangePasswordDialog)
{
    ui->setupUi(this);
    connect(ui->pushButtonCancel, &QPushButton::clicked, this, &ChangePasswordDialog::reject);

    ui->pushButtonConfirm->setObjectName("msgBoxConfirmButton");
    ui->pushButtonCancel->setObjectName("msgBoxCancelButton");
}

ChangePasswordDialog::~ChangePasswordDialog()
{
    delete ui;
}

QString ChangePasswordDialog::getOldPassword() const
{
    return ui->lineEditOldPassword->text();
}

QString ChangePasswordDialog::getNewPassword() const
{
    return ui->lineEditNewPassword->text();
}

void ChangePasswordDialog::on_pushButtonConfirm_clicked()
{
    if (ui->lineEditNewPassword->text() != ui->lineEditConfirmPassword->text()) {
        QMessageBox::warning(this, "错误", "两次输入的新密码不一致");
        return;
    }
    if (ui->lineEditNewPassword->text().isEmpty()) {
        QMessageBox::warning(this, "错误", "新密码不能为空");
        return;
    }
    accept();
}
