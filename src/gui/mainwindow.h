#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "core/user.h"
#include <QTimer>

QT_BEGIN_NAMESPACE
class QLabel;
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow();
  void setUser(const User &user);

private slots:
  void on_actionExit_triggered();
  void on_actionAbout_triggered();
  void on_pushButtonAddVehicle_clicked();
  void on_pushButtonEditVehicle_clicked();
  void on_pushButtonDeleteVehicle_clicked();
  void on_pushButtonQuery_clicked();
  void on_pushButtonPay_clicked();
  void on_actionLogout_triggered();

    void on_pushButtonAddUser_clicked();
    void on_pushButtonEditUser_clicked();
    void on_pushButtonDeleteUser_clicked();
    void on_pushButtonBindVehicle_clicked();
    void on_pushButtonChangePassword_clicked();
    void on_pushButtonIntelligentBind_clicked();
    void updateStatisticsPage();
    void on_pushButtonUpload_clicked();
    void on_pushButtonUpload_Leave_clicked();
    void on_pushButtonIntelligentQuery_clicked();


private:
  void updateVehicleView();
  void updateUserView();
  void displayMyVehicles();
  void handlePayment(const QString &plate);
  void handleMonthlyPass(const QString &plate);
  QString formatPlateNumber(const std::string &rawPlate);

private:
  Ui::MainWindow *ui;
  QTimer *m_feeUpdateTimer;
  User m_currentUser;
  QString m_queriedPlate; // 查询结果中的纯文本车牌（避免取到富文本 HTML）
};

#endif // MAINWINDOW_H
