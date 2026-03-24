#include "gui/registerdialog.h"
#include "ui_registerdialog.h"
#include "core/datamanager.h"
#include <QMessageBox>

RegisterDialog::RegisterDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RegisterDialog)
{
    ui->setupUi(this);
}

RegisterDialog::~RegisterDialog()
{
    delete ui;
}

void RegisterDialog::on_pushButtonRegister_clicked()
{
    QString username = ui->lineEditUser->text();
    QString password = ui->lineEditPassword->text();
    QString confirmPassword = ui->lineEditConfirmPassword->text();

    if (username == "root") {
        QMessageBox::warning(this, "注册失败", "此用户名已被系统保留");
        return;
    }

    if (password.isEmpty() || username.isEmpty()) {
        QMessageBox::warning(this, "注册失败", "用户名或密码不能为空");
        return;
    }

    if (password != confirmPassword) {
        QMessageBox::warning(this, "注册失败", "两次输入的密码不一致");
        return;
    }

    if (DataManager::instance().addUser(username, password)) {
        QMessageBox::information(this, "注册成功", "用户注册成功");
        accept();
    } else {
        QMessageBox::warning(this, "注册失败", "用户名已存在");
    }
}
