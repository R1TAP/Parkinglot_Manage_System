#ifndef CHANGEPASSWORDDIALOG_H
#define CHANGEPASSWORDDIALOG_H

#include <QDialog>

namespace Ui {
class ChangePasswordDialog;
}

class ChangePasswordDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChangePasswordDialog(QWidget *parent = nullptr);
    ~ChangePasswordDialog();
    QString getOldPassword() const;
    QString getNewPassword() const;

private slots:
    void on_pushButtonConfirm_clicked();

private:
    Ui::ChangePasswordDialog *ui;
};

#endif // CHANGEPASSWORDDIALOG_H
