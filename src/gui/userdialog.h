#ifndef USERDIALOG_H
#define USERDIALOG_H

#include <QDialog>
#include "core/user.h"

namespace Ui {
class UserDialog;
}

class UserDialog : public QDialog
{
    Q_OBJECT

public:
    explicit UserDialog(QWidget *parent = nullptr);
    ~UserDialog();

    User getUser() const;
    void setUser(const User &user);

private:
    Ui::UserDialog *ui;
};

#endif // USERDIALOG_H
