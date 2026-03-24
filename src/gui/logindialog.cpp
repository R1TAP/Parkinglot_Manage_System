#include "gui/logindialog.h"
#include "ui_logindialog.h"
#include "gui/mainwindow.h"
#include "gui/registerdialog.h"
#include "core/datamanager.h"
#include <QMessageBox>

LoginDialog::LoginDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

void LoginDialog::on_pushButtonLogin_clicked()
{
    QString username = ui->lineEditUser->text();
    QString password = ui->lineEditPassword->text();

    m_role = DataManager::instance().validateUser(username, password, &m_user);

    if (m_role != UserRole::Invalid) {
        accept();
    } else {
        QMessageBox::warning(this, "登录失败", "用户名或密码错误");
    }
}

void LoginDialog::on_pushButtonRegister_clicked()
{
    RegisterDialog dlg(this);
    dlg.exec();
}
