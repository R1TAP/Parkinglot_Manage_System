#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
#include "core/user.h"

namespace Ui {
class LoginDialog;
}

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);
    ~LoginDialog();
    UserRole getRole() const { return m_role; }
    User getUser() const { return m_user; }

private slots:
    void on_pushButtonLogin_clicked();
    void on_pushButtonRegister_clicked();

private:
    Ui::LoginDialog *ui;
    UserRole m_role = UserRole::Invalid;
    User m_user;
};

#endif // LOGINDIALOG_H
