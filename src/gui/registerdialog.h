#ifndef REGISTERDIALOG_H
#define REGISTERDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui {
class RegisterDialog;
}
QT_END_NAMESPACE

class RegisterDialog : public QDialog {
  Q_OBJECT

public:
  explicit RegisterDialog(QWidget *parent = nullptr);
  ~RegisterDialog();

private slots:
  void on_pushButtonRegister_clicked();

private:
  Ui::RegisterDialog *ui;
};

#endif // REGISTERDIALOG_H
