#include "gui/mainwindow.h"
#include "ui_mainwindow.h"
#include "gui/bindvehicledialog.h"
#include "gui/changepassworddialog.h"
#include "gui/userdialog.h"
#include "gui/vehicledialog.h"
#include "gui/monthpassdialog.h"
#include "core/datamanager.h"
#include "core/plateutil.h"
#include <QMessageBox>
#include <QStandardItemModel>
#include <QInputDialog>
#include <QDateTime>
#include <QLabel>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <cmath>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

  // Disable direct editing on the table view
    ui->tableViewVehicles->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableViewVehicles->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableViewVehicles->setSelectionMode(QAbstractItemView::SingleSelection);
    //
    // ui->tableViewVehicles->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    // ui->tableViewVehicles->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    // ui->tableViewVehicles->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    // ui->tableViewVehicles->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    // ui->tableViewVehicles->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);

    // Disable direct editing on the user table view
    ui->tableViewUsers->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableViewUsers->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableViewUsers->setSelectionMode(QAbstractItemView::SingleSelection);

  updateVehicleView();
    updateUserView();

    m_feeUpdateTimer = new QTimer(this);
    connect(m_feeUpdateTimer, &QTimer::timeout, this, &MainWindow::updateVehicleView);
    m_feeUpdateTimer->start(5000); // Update every 5 seconds

    // Initially hide the result group and pay button
    ui->groupBoxResult->setVisible(false);
    ui->pushButtonPay->setVisible(false);

    connect(m_feeUpdateTimer, &QTimer::timeout, this, &MainWindow::updateStatisticsPage);

    //connect(ui->pushButtonIntelligentQuery, &QPushButton::clicked, this, &MainWindow::on_pushButtonIntelligentQuery_clicked);
}

void MainWindow::on_pushButtonEditVehicle_clicked()
{
    QModelIndexList selectedRows = ui->tableViewVehicles->selectionModel()->selectedRows();
    if (selectedRows.isEmpty()) {
        QMessageBox::warning(this, "操作失败", "请先选择要修改的车辆");
        return;
    }

    int row = selectedRows.first().row();
    QString plate = ui->tableViewVehicles->model()->index(row, 0).data().toString();
    auto vehicleToEdit = DataManager::instance().findVehicleByPlate(plate);

    if (!vehicleToEdit) return; // Should not happen

    VehicleDialog dlg(this);
    dlg.setVehicle(*vehicleToEdit);

    if (dlg.exec() == QDialog::Accepted) {
        Vehicle updatedVehicle = dlg.getVehicle();
        DataManager::instance().updateVehicle(updatedVehicle);
        updateVehicleView();
    }
}

void MainWindow::on_pushButtonDeleteVehicle_clicked()
{
    QModelIndexList selectedRows = ui->tableViewVehicles->selectionModel()->selectedRows();
    if (selectedRows.isEmpty()) {
        QMessageBox::warning(this, "操作失败", "请先选择要删除的车辆");
        return;
    }

    QMessageBox msgBox(this);
    msgBox.setWindowTitle("确认删除");
    msgBox.setText("您确定要删除选中的车辆记录吗？");
    msgBox.setIcon(QMessageBox::Question);

    QPushButton *confirmButton = msgBox.addButton("确认", QMessageBox::YesRole);
    QPushButton *cancelButton = msgBox.addButton("取消", QMessageBox::NoRole);

    // Set object names for styling
    confirmButton->setObjectName("msgBoxConfirmButton");
    cancelButton->setObjectName("msgBoxCancelButton");

    msgBox.exec();

    if (msgBox.clickedButton() == confirmButton) {
        int row = selectedRows.first().row();
        QString plate = ui->tableViewVehicles->model()->index(row, 0).data().toString();
        DataManager::instance().deleteVehicle(plate);
        updateVehicleView();
    }
}

void MainWindow::on_pushButtonChangePassword_clicked()
{
    ChangePasswordDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        if (DataManager::instance().validateUser(m_currentUser.username, dlg.getOldPassword()) == UserRole::Invalid) {
            QMessageBox::warning(this, "失败", "旧密码不正确");
            return;
        }

        User updatedUser = m_currentUser;
        updatedUser.password = dlg.getNewPassword();

        if (DataManager::instance().updateUser(updatedUser)) {
            m_currentUser = updatedUser; // Update current user info
            m_currentUser.password = DataManager::hashPassword(dlg.getNewPassword());
            QMessageBox::information(this, "成功", "密码修改成功");
        } else {
            QMessageBox::warning(this, "失败", "更新密码时出错");
        }
    }
}

void MainWindow::displayMyVehicles()
{
    // Clear previous vehicle widgets
    QLayout *layout = ui->scrollAreaWidgetContents->layout();
    QLayoutItem *child;
    while ((child = layout->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }

    QVector<Vehicle> vehicles = DataManager::instance().findVehiclesByOwner(m_currentUser.username);

    if (vehicles.isEmpty()) {
        QLabel *emptyLabel = new QLabel("您还没有绑定任何车辆", this);
        emptyLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(emptyLabel);
        return;
    }

    for (const auto &vehicle : vehicles) {
        QGroupBox *groupBox = new QGroupBox(vehicle.plate, this);
        QString color;
        if (vehicle.vehicle_type == "蓝") {
            color = "#90CAF9";
        } else if (vehicle.vehicle_type == "绿") {
            color = "#8CE0A8";
        } else if (vehicle.vehicle_type == "黄") {
            color = "#FFD66E";
        }
        groupBox->setStyleSheet(QString("QGroupBox { border: 1px solid %1; border-radius: 16px; background-color: #211F26; margin-top: 1.4em; padding: 12px; } QGroupBox::title { subcontrol-origin: margin; left: 16px; padding: 0 6px; color: %1; font-weight: 600; background-color: #211F26; }").arg(color));
        QFormLayout *formLayout = new QFormLayout(groupBox);

        if (vehicle.isMonthlyPassHolder()) {
            QLabel* expiryLabel = new QLabel(QString("月卡到期时间: %1").arg(vehicle.pass_expiry_date.toString("yyyy-MM-dd")));
            expiryLabel->setStyleSheet("font-size: 14px; font-weight: 600; color: #8CE0A8;");
            formLayout->addRow(expiryLabel);
        } else {
            QLabel* entryTimeValue = new QLabel(vehicle.entryTime.toString("yyyy-MM-dd hh:mm:ss"));
            QLabel* feeValue = new QLabel();
            double fee = vehicle.calculateFee();
            feeValue->setText(QString("¥ %1").arg(fee, 0, 'f', 2));
            feeValue->setStyleSheet("font-size: 12pt; font-weight: bold;");

            formLayout->addRow("入场时间:", entryTimeValue);
            formLayout->addRow("当前费用:", feeValue);

            QHBoxLayout* buttonLayout = new QHBoxLayout();
            QPushButton *monthlyButton = new QPushButton("办理月卡");
            QPushButton *payButton = new QPushButton("去缴费");

            monthlyButton->setObjectName("monthlyPassButton"); // For styling
            payButton->setObjectName("pushButtonPay");

            buttonLayout->addWidget(monthlyButton);
            buttonLayout->addWidget(payButton);
            formLayout->addRow(buttonLayout);

            connect(payButton, &QPushButton::clicked, [this, plate = vehicle.plate]() {
                handlePayment(plate);
            });
            connect(monthlyButton, &QPushButton::clicked, [this, plate = vehicle.plate]() {
                handleMonthlyPass(plate);
            });
        }

        groupBox->setLayout(formLayout);
        layout->addWidget(groupBox);
    }
    layout->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));
}

void MainWindow::on_pushButtonBindVehicle_clicked()
{
    BindVehicleDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        QString plate = normalizePlate(dlg.getPlateNumber());
        if (!isValidPlate(plate)) {
            QMessageBox::warning(this, "绑定失败", "车牌格式不正确（示例：苏A12345）。");
            return;
        }
        auto vehicle = DataManager::instance().findVehicleByPlate(plate);
        if (vehicle) {
            if (!vehicle->owner.isEmpty() && vehicle->owner != m_currentUser.username) {
                QMessageBox::warning(this, "绑定失败", QString("车辆 %1 已被用户 %2 绑定。").arg(plate).arg(vehicle->owner));
                return;
            }
            if (vehicle->owner == m_currentUser.username) {
                QMessageBox::information(this, "绑定成功", QString("车辆 %1 已绑定到您的账户。").arg(plate));
                displayMyVehicles();
                return;
            }
            vehicle->owner = m_currentUser.username;
            if (DataManager::instance().updateVehicle(*vehicle)) {
                QMessageBox::information(this, "绑定成功", "车辆绑定成功！");
                displayMyVehicles();
            } else {
                QMessageBox::warning(this, "绑定失败", "更新车辆信息时出错。");
            }
        } else {
            QMessageBox::warning(this, "绑定失败", "未找到该车牌号的车辆。");
        }
    }
}

void MainWindow::handlePayment(const QString &plate)
{
    QDialog paymentDialog(this);
    paymentDialog.setWindowTitle("扫码支付");

    // QR Code Label
    QLabel *qrLabel = new QLabel(&paymentDialog);
    QPixmap qrPixmap(":/Pay.png");
    qrLabel->setPixmap(qrPixmap);
    qrLabel->setScaledContents(true);

    // Buttons
    QPushButton *cancelButton = new QPushButton("取消");
    QPushButton *paidButton = new QPushButton("我已完成缴费→");
    cancelButton->setObjectName("msgBoxCancelButton");
    paidButton->setObjectName("pushButtonPay");

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(paidButton);

    QVBoxLayout *mainLayout = new QVBoxLayout(&paymentDialog);
    mainLayout->addWidget(qrLabel);
    mainLayout->addLayout(buttonLayout);

    paymentDialog.setLayout(mainLayout);
    paymentDialog.setFixedSize(qrPixmap.width() + 40, qrPixmap.height() + 80);

    connect(cancelButton, &QPushButton::clicked, &paymentDialog, &QDialog::reject);
    connect(paidButton, &QPushButton::clicked, &paymentDialog, &QDialog::accept);

    if (paymentDialog.exec() == QDialog::Accepted) {
        auto v = DataManager::instance().findVehicleByPlate(plate);
        if (!v) {
            QMessageBox::warning(this, "操作失败", QString("未找到车牌 %1 的在场车辆，缴费未生效。").arg(plate));
            return;
        }
        double fee = v->calculateFee();
        DataManager::instance().logTransaction(plate, fee, "temporary");

        DataManager::instance().endParkingSession(plate);
        QMessageBox::information(this, "操作成功", "缴费成功，车辆已离场。");
        // Refresh both dashboard and my page views
        ui->groupBoxResult->setVisible(false);
        ui->pushButtonPay->setVisible(false);
        ui->lineEditQueryPlate->clear();
        m_queriedPlate.clear();
        displayMyVehicles();
        updateVehicleView(); // Also update admin view if open
    }
}

void MainWindow::handleMonthlyPass(const QString &plate)
{
    MonthPassDialog monthDlg(this);
    if (monthDlg.exec() == QDialog::Accepted) {
        int days = monthDlg.getSelectedDays();

        QDialog paymentDialog(this);
        paymentDialog.setWindowTitle("办理月卡");

        QLabel *qrLabel = new QLabel(&paymentDialog);
        QPixmap qrPixmap(":/Month.png");
        qrLabel->setPixmap(qrPixmap);
        qrLabel->setScaledContents(true);

        QPushButton *cancelButton = new QPushButton("取消");
        QPushButton *paidButton = new QPushButton("我已办理→");
        cancelButton->setObjectName("msgBoxCancelButton");
        paidButton->setObjectName("msgBoxConfirmButton");

        QHBoxLayout *buttonLayout = new QHBoxLayout;
        buttonLayout->addStretch();
        buttonLayout->addWidget(cancelButton);
        buttonLayout->addWidget(paidButton);

        QVBoxLayout *mainLayout = new QVBoxLayout(&paymentDialog);
        mainLayout->setContentsMargins(0, 0, 0, 10);
        mainLayout->addWidget(qrLabel);
        mainLayout->addLayout(buttonLayout);
        paymentDialog.setLayout(mainLayout);

        // Set a fixed width and calculate height to maintain aspect ratio
        int dialogWidth = 350;
        double aspectRatio = (double)qrPixmap.height() / qrPixmap.width();
        int dialogHeight = (dialogWidth * aspectRatio) + 60; // 60px for buttons and margins
        paymentDialog.setFixedSize(dialogWidth, dialogHeight);

        connect(cancelButton, &QPushButton::clicked, &paymentDialog, &QDialog::reject);
        connect(paidButton, &QPushButton::clicked, &paymentDialog, &QDialog::accept);

        if (paymentDialog.exec() == QDialog::Accepted) {
            double amount = 0;
            if (days == 30) amount = 300;
            if (days == 90) amount = 800;
            if (days == 365) amount = 3000;
            DataManager::instance().logTransaction(plate, amount, "monthly", days);

            if (DataManager::instance().upgradeToMonthlyPass(plate, days)) {
                QMessageBox::information(this, "办理成功", "月卡办理成功！");
                displayMyVehicles();
                updateVehicleView();
            } else {
                QMessageBox::warning(this, "办理失败", "更新车辆信息时出错。");
            }
        }
    }
}

#include <QFileDialog>
#include "easypr.h"

using namespace easypr;

#include <QRegularExpression>

std::string colorToString(easypr::Color color) {
    switch (color) {
        case easypr::BLUE: return "蓝";
        case easypr::GREEN: return "绿";
        case easypr::YELLOW: return "黄";
        case easypr::WHITE: return "白";
        default: return "未知";
    }
}

QString MainWindow::formatPlateNumber(const std::string &rawPlate)
{
    // EasyPR 输出形如 "蓝牌:苏A12345"；去掉颜色前缀与噪音字符后校验。
    QString plate = QString::fromLocal8Bit(rawPlate.c_str());
    const int colon = plate.indexOf(':');
    if (colon >= 0) {
        plate = plate.mid(colon + 1);
    }
    plate = normalizePlate(plate);
    if (isValidPlate(plate)) {
        return plate;
    }
    // 识别失败时 EasyPR 可能只输出颜色串（如“蓝”），这里统一视为无效
    return QString();
}

void MainWindow::on_pushButtonUpload_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, "选择图片", "", "Images (*.png *.xpm *.jpg *.bmp)");
    if (filePath.isEmpty()) {
        return;
    }

    // Update image preview
    QPixmap pixmap(filePath);
    ui->labelImagePreview->setPixmap(pixmap.scaled(ui->labelImagePreview->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));

    // Perform plate recognition
    CPlateRecognize pr;
    pr.setResultShow(false);
    pr.setDetectType(PR_DETECT_CMSER | PR_DETECT_COLOR);
    std::vector<CPlate> plateVec;
    cv::Mat image = cv::imread(filePath.toStdString());
    if (image.empty()) {
        ui->labelRecognitionResult->setText("图片读取失败，请确认文件有效");
        QMessageBox::warning(this, "识别失败", "无法读取所选图片。");
        return;
    }
    int result = pr.plateRecognize(image, plateVec);

    if (result == 0 && !plateVec.empty()) {
        CPlate plate = plateVec.front();
        QString formattedPlate = formatPlateNumber(plate.getPlateStr());
        if (formattedPlate.isEmpty()) {
            ui->labelRecognitionResult->setText("识别结果无效，请尝试更清晰的图片");
            QMessageBox::warning(this, "识别失败", "未能识别出有效车牌。");
            return;
        }
        std::string colorStr = colorToString(plate.getPlateColor());

        ui->labelRecognitionResult->setText(QString("识别成功: %1 (%2)").arg(formattedPlate).arg(QString::fromStdString(colorStr)));

        if (colorStr != "蓝" && colorStr != "绿" && colorStr != "黄") {
            QMessageBox::warning(this, "录入失败", QString("无法确定车牌颜色（%1），暂不支持录入该车辆。").arg(QString::fromStdString(colorStr)));
            return;
        }

        // 同牌临时车已在场时拒绝，避免重置计费起点
        auto existing = DataManager::instance().findVehicleByPlate(formattedPlate);
        const bool isParked = existing && existing->entryTime.isValid();
        const bool isMonthly = existing && existing->isMonthlyPassHolder();
        if (existing && isParked && !isMonthly) {
            QMessageBox::warning(this, "录入失败", QString("检测到相同车牌的临时车辆 %1 已在场，请确认信息").arg(formattedPlate));
            return;
        }

        // Add to database
        Vehicle newVehicle;
        newVehicle.plate = formattedPlate;
        newVehicle.entryTime = QDateTime::currentDateTime();

        if (colorStr == "蓝") {
            newVehicle.vehicle_type = "蓝";
            newVehicle.vehicle_color = "蓝";
        } else if (colorStr == "绿") {
            newVehicle.vehicle_type = "绿";
            newVehicle.vehicle_color = "绿";
        } else if (colorStr == "黄") {
            newVehicle.vehicle_type = "黄";
            newVehicle.vehicle_color = "黄";
        }
        if (existing) {
            newVehicle.owner = existing->owner; // 已绑定车辆入场时保留车主
        }

        DataManager::instance().addVehicle(newVehicle);
        QMessageBox::information(this, "操作成功", QString("车辆 %1 已成功录入系统！").arg(formattedPlate));
        updateVehicleView(); // Refresh the vehicle list view
    } else {
        ui->labelRecognitionResult->setText("识别失败，请尝试更清晰的图片");
        QMessageBox::warning(this, "识别失败", "未能在图片中识别出车牌。");
    }
}

void MainWindow::on_pushButtonUpload_Leave_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, "选择图片", "", "Images (*.png *.xpm *.jpg *.bmp)");
    if (filePath.isEmpty()) {
        return;
    }

    // Update image preview for the 'Leave' tab
    QPixmap pixmap(filePath);
    QLabel *imagePreviewLabel = this->findChild<QLabel*>("labelImagePreview_Leave");
    if (imagePreviewLabel) {
        imagePreviewLabel->setPixmap(pixmap.scaled(imagePreviewLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    QLabel *resultLabel = this->findChild<QLabel*>("labelRecognitionResult_Leave");

    // Perform plate recognition
    CPlateRecognize pr;
    pr.setResultShow(false);
    pr.setDetectType(PR_DETECT_CMSER | PR_DETECT_COLOR);
    std::vector<CPlate> plateVec;
    cv::Mat image = cv::imread(filePath.toStdString());
    if (image.empty()) {
        if (resultLabel) {
            resultLabel->setText("图片读取失败，请确认文件有效");
        }
        QMessageBox::warning(this, "识别失败", "无法读取所选图片。");
        return;
    }
    int result = pr.plateRecognize(image, plateVec);

    if (result == 0 && !plateVec.empty()) {
        CPlate plate = plateVec.front();
        QString formattedPlate = formatPlateNumber(plate.getPlateStr());
        if (formattedPlate.isEmpty()) {
            if (resultLabel) {
                resultLabel->setText("识别结果无效，请尝试更清晰的图片");
            }
            QMessageBox::warning(this, "识别失败", "未能识别出有效车牌。");
            return;
        }

        if (resultLabel) {
            resultLabel->setText(QString("识别成功: %1").arg(formattedPlate));
        }

        // Find vehicle and handle payment/exit
        auto vehicle = DataManager::instance().findVehicleByPlate(formattedPlate);
        if (vehicle) {
            handlePayment(formattedPlate);
        } else {
            QMessageBox::warning(this, "操作失败", "未在停车场中找到该车牌号的车辆。");
        }
    } else {
        if (resultLabel) {
            resultLabel->setText("识别失败，请尝试更清晰的图片");
        }
        QMessageBox::warning(this, "识别失败", "未能在图片中识别出车牌。");
    }
}




MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setUser(const User &user)
{
    m_currentUser = user;
    ui->labelUsernameHeader->setText(QString("欢迎您，%1").arg(user.username));

    if (user.role == UserRole::Visitor) {
        for (int i = 0; i < ui->tabWidget->count(); ++i) {
            QString tabText = ui->tabWidget->tabText(i);
            if (tabText == "车辆管理" || tabText == "账户管理" || tabText == "统计" || tabText == "智能入场" || tabText == "智能离场") {
                ui->tabWidget->setTabVisible(i, false);
            }
        }
        displayMyVehicles();

        // Add the Intelligent Bind button if it doesn't exist
        if (!this->findChild<QPushButton*>("pushButtonIntelligentBind")) {
            // Find the layout containing the existing buttons.
            // This is a more robust way than assuming a specific layout type.
            QPushButton* refButton = ui->pushButtonBindVehicle; // Use one of the existing buttons as a reference.
            if (refButton && refButton->parentWidget()) {
                QBoxLayout* layout = qobject_cast<QBoxLayout*>(refButton->parentWidget()->layout());
                if (layout) {
                    QPushButton* intelligentBindButton = new QPushButton("智能绑定");
                    intelligentBindButton->setObjectName("pushButtonIntelligentBind");
                    connect(intelligentBindButton, &QPushButton::clicked, this, &MainWindow::on_pushButtonIntelligentBind_clicked);
                    layout->addWidget(intelligentBindButton);
                }
            }
        }
    } else if (user.role == UserRole::Admin) {
        for (int i = 0; i < ui->tabWidget->count(); ++i) {
            QString tabText = ui->tabWidget->tabText(i);
            if (tabText == "我的") {
                ui->tabWidget->setTabVisible(i, false);
            } else {
                ui->tabWidget->setTabVisible(i, true);
            }
        }
        updateStatisticsPage(); // Initial update for admin
    }
}

void MainWindow::on_actionExit_triggered()
{
    close();
}

void MainWindow::on_actionAbout_triggered()
{
    QMessageBox aboutBox(this);
    aboutBox.setWindowTitle("关于");
    aboutBox.setText("(c)2026, Retap Technology");
    aboutBox.setIcon(QMessageBox::Information);
    // Apply the stylesheet to this specific message box
    QString style;
    QFile file(":/style.qss");
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        style = QLatin1String(file.readAll());
    }
    aboutBox.setStyleSheet(style);
    aboutBox.exec();
}

void MainWindow::on_pushButtonAddVehicle_clicked()
{
    VehicleDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) {
        return;
    }

    Vehicle newVehicle = dlg.getVehicle();
    newVehicle.plate = normalizePlate(newVehicle.plate);
    if (!isValidPlate(newVehicle.plate)) {
        QMessageBox::warning(this, "录入失败", "车牌格式不正确（示例：苏A12345，新能源8位如苏AD12345）。");
        return;
    }
    auto existingVehicle = DataManager::instance().findVehicleByPlate(newVehicle.plate);

    if (existingVehicle) {
        bool isParked = !existingVehicle->entryTime.toString().isEmpty();
        bool isExistingVehicleMonthly = existingVehicle->pass_expiry_date.isValid() && QDateTime::currentDateTime() < existingVehicle->pass_expiry_date;
        if (!isExistingVehicleMonthly && isParked) {
            QMessageBox::warning(this, "录入失败", "检测到相同车牌的临时车辆已在场，请确认信息");
            return; // Abort
        }
    }

    DataManager::instance().addVehicle(newVehicle);
    updateVehicleView();
}

void MainWindow::updateVehicleView()
{
    QStandardItemModel *model = new QStandardItemModel(0, 5, this);
    model->setHorizontalHeaderLabels({"车牌号", "类型", "车主", "入场时间", "当前费用"});

    QVector<Vehicle> vehicles = DataManager::instance().getVehicles();
    for (const auto &vehicle : vehicles) {
        QList<QStandardItem *> items;
        QStandardItem *plateItem = new QStandardItem(vehicle.plate);
        if (vehicle.vehicle_color == "蓝") {
            plateItem->setData(QColor("#90CAF9"), Qt::ForegroundRole);
        } else if (vehicle.vehicle_color == "绿") {
            plateItem->setData(QColor("#8CE0A8"), Qt::ForegroundRole);
        } else if (vehicle.vehicle_color == "黄") {
            plateItem->setData(QColor("#FFD66E"), Qt::ForegroundRole);
        }
        QFont font = plateItem->font();
        font.setBold(true);
        plateItem->setFont(font);
        items.append(plateItem);

        if (vehicle.vehicle_type == "蓝") {
            items.append(new QStandardItem("燃油车"));
        } else if (vehicle.vehicle_type == "绿") {
            items.append(new QStandardItem("新能源"));
        } else if (vehicle.vehicle_type == "黄") {
            items.append(new QStandardItem("大型车"));
        } else {
            items.append(new QStandardItem("未知"));
        }

        items.append(new QStandardItem(vehicle.owner));
        items.append(new QStandardItem(vehicle.entryTime.toString("yyyy-MM-dd hh:mm:ss")));

        if (vehicle.isMonthlyPassHolder()) {
            items.append(new QStandardItem("月卡用户"));
        } else {
            double fee = vehicle.calculateFee();
            items.append(new QStandardItem(QString("¥ %1").arg(fee, 0, 'f', 2)));
        }
        model->appendRow(items);
    }

    QAbstractItemModel* oldModel = ui->tableViewVehicles->model();
    ui->tableViewVehicles->setModel(model);
    if (oldModel) {
        delete oldModel;
    }
}

void MainWindow::on_pushButtonQuery_clicked()
{
    QString plate = normalizePlate(ui->lineEditQueryPlate->text());
    if (plate.isEmpty()) {
        QMessageBox::warning(this, "查询失败", "请输入车牌号");
        return;
    }
    if (!isValidPlate(plate)) {
        QMessageBox::warning(this, "查询失败", "车牌格式不正确（示例：苏A12345，新能源8位如苏AD12345）。");
        return;
    }

    m_queriedPlate.clear();
    auto vehicle = DataManager::instance().findVehicleByPlate(plate);

    if (vehicle) {
        bool isParked = vehicle->entryTime.isValid();

        QString color;
        if (vehicle->vehicle_type == "蓝") {
            color = "#90CAF9";
        } else if (vehicle->vehicle_type == "绿") {
            color = "#8CE0A8";
        } else if (vehicle->vehicle_type == "黄") {
            color = "#FFD66E";
        }
        ui->labelPlateValue->setText(QString("<font color='%1'><b>%2</b></font>").arg(color).arg(vehicle->plate));
        m_queriedPlate = vehicle->plate; // 保存纯文本车牌供缴费使用
        ui->groupBoxResult->setStyleSheet(QString("QGroupBox { border: 1px solid %1; border-radius: 16px; background-color: #211F26; margin-top: 1.4em; padding: 12px; } QGroupBox::title { subcontrol-origin: margin; left: 16px; padding: 0 6px; color: %1; font-weight: 600; background-color: #211F26; }").arg(color));
        ui->groupBoxResult->setVisible(true);

        if (isParked) {
            ui->labelEntryTime->setVisible(true);
            ui->labelExitTime->setVisible(true);
            ui->labelParkingFee->setVisible(true);
            ui->labelParkingFee->setText("当前费用:");

            ui->labelEntryTimeValue->setText(vehicle->entryTime.toString("yyyy-MM-dd hh:mm:ss"));
            ui->labelExitTimeValue->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));

            if (vehicle->isMonthlyPassHolder()) {
                ui->labelParkingFeeValue->setText("月卡用户");
                ui->pushButtonPay->setText("确认离场");
            } else {
                double fee = vehicle->calculateFee();
                ui->labelParkingFeeValue->setText(QString("¥ %1").arg(fee, 0, 'f', 2));
                ui->pushButtonPay->setText("去缴费");
            }
            ui->pushButtonPay->setVisible(true);

        } else { // Not parked
            ui->labelEntryTime->setVisible(false);
            ui->labelExitTime->setVisible(false);
            ui->labelParkingFee->setVisible(true);
            ui->labelParkingFee->setText("状态:");

            ui->labelEntryTimeValue->setText("");
            ui->labelExitTimeValue->setText("");
            if(vehicle->isMonthlyPassHolder()){
                 ui->labelParkingFeeValue->setText(QString("月卡至: %1").arg(vehicle->pass_expiry_date.toString("yyyy-MM-dd")));
            } else {
                ui->labelParkingFeeValue->setText("已离场");
            }
           
            
            ui->pushButtonPay->setVisible(false);
        }
        
    } else {
        QMessageBox::information(this, "查询结果", "未在系统中找到该车牌号");
        ui->groupBoxResult->setVisible(false);
        ui->pushButtonPay->setVisible(false);
    }
}

void MainWindow::on_pushButtonPay_clicked()
{
    QString plateToPay = m_queriedPlate;
    if(plateToPay.isEmpty()) return;

    // If it's a monthly user, just end the parking session without payment
    if (ui->pushButtonPay->text() == "确认离场") {
        DataManager::instance().endParkingSession(plateToPay);
        QMessageBox::information(this, "操作成功", "车辆已离场。");
        ui->groupBoxResult->setVisible(false);
        ui->pushButtonPay->setVisible(false);
        ui->lineEditQueryPlate->clear();
        m_queriedPlate.clear();
        updateVehicleView(); // Refresh admin view
        displayMyVehicles(); // Refresh my page view
    } else { // Otherwise, go to payment
        handlePayment(plateToPay);
    }
}

void MainWindow::on_actionLogout_triggered()
{
    // Exit the application with a special code to signal logout
    QApplication::exit(1337);
}

void MainWindow::updateUserView()
{
    QStandardItemModel *model = new QStandardItemModel(0, 2, this);
    model->setHorizontalHeaderLabels({"用户名", "角色"});

    for (const auto &user : DataManager::instance().getUsers()) {
        QList<QStandardItem *> items;
        items.append(new QStandardItem(user.username));
        QString roleStr = (user.role == UserRole::Admin) ? "管理员" : "访客";
        items.append(new QStandardItem(roleStr));
        model->appendRow(items);
    }

    ui->tableViewUsers->setModel(model);
}

void MainWindow::on_pushButtonAddUser_clicked()
{
    UserDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        User newUser = dlg.getUser();
        if (DataManager::instance().isUsernameTaken(newUser.username)) {
            QMessageBox::warning(this, "操作失败", "用户名已存在");
            return;
        }
        if (DataManager::instance().addUser(newUser.username, newUser.password)) {
            updateUserView();
        }
    }
}

void MainWindow::on_pushButtonEditUser_clicked()
{
    QModelIndexList selectedRows = ui->tableViewUsers->selectionModel()->selectedRows();
    if (selectedRows.isEmpty()) {
        QMessageBox::warning(this, "操作失败", "请先选择要修改的用户");
        return;
    }

    int row = selectedRows.first().row();
    QString username = ui->tableViewUsers->model()->index(row, 0).data().toString();

    if (username == "root") {
        QMessageBox::warning(this, "操作失败", "不能修改root管理员账户");
        return;
    }

    // This is not perfectly efficient as it iterates all users, but fine for this scale
    User userToEdit;
    for (const auto& user : DataManager::instance().getUsers()) {
        if (user.username == username) {
            userToEdit = user;
            break;
        }
    }

    UserDialog dlg(this);
    dlg.setUser(userToEdit);

    if (dlg.exec() == QDialog::Accepted) {
        if (DataManager::instance().updateUser(dlg.getUser())) {
            updateUserView();
        }
    }
}

void MainWindow::on_pushButtonDeleteUser_clicked()
{
    QModelIndexList selectedRows = ui->tableViewUsers->selectionModel()->selectedRows();
    if (selectedRows.isEmpty()) {
        QMessageBox::warning(this, "操作失败", "请先选择要删除的用户");
        return;
    }

    int row = selectedRows.first().row();
    QString username = ui->tableViewUsers->model()->index(row, 0).data().toString();

    if (username == "root") {
        QMessageBox::warning(this, "操作失败", "不能删除root管理员账户");
        return;
    }

    QMessageBox msgBox(this);
    msgBox.setWindowTitle("确认删除");
    msgBox.setText(QString("您确定要删除用户 '%1' 吗？").arg(username));
    msgBox.setIcon(QMessageBox::Question);
    QPushButton *confirmButton = msgBox.addButton("确认", QMessageBox::YesRole);
    msgBox.addButton("取消", QMessageBox::NoRole);
    confirmButton->setObjectName("msgBoxConfirmButton");

    msgBox.exec();

    if (msgBox.clickedButton() == confirmButton) {
        if (DataManager::instance().deleteUser(username)) {
            updateUserView();
        }
    }
}

void MainWindow::updateStatisticsPage()
{
    // Update Occupancy
    QMap<QString, int> counts = DataManager::instance().getVehicleCounts();
    int blueCount = counts.value("蓝");
    int greenCount = counts.value("绿");
    int yellowCount = counts.value("黄");
    int totalCount = blueCount + greenCount + yellowCount;
    int maxOccupancy = DataManager::instance().parkingCapacity();
    if (maxOccupancy < 1) maxOccupancy = 1;
    int emptyCount = qMax(0, maxOccupancy - totalCount);

    ui->occupancyStatsLabel->setText(QString("总车位: %1 | 燃油车: %2 | 新能源: %3 | 大型车: %4 | 空余: %5")
                                   .arg(maxOccupancy).arg(blueCount).arg(greenCount).arg(yellowCount).arg(emptyCount));

    QList<QString> occupancyParts;
    QList<double> occupancyWidths;
    if (blueCount > 0) { occupancyParts.append("background-color: #90CAF9;"); occupancyWidths.append(qMin(100.0, blueCount * 100.0 / maxOccupancy)); }
    if (greenCount > 0) { occupancyParts.append("background-color: #8CE0A8;"); occupancyWidths.append(qMin(100.0, greenCount * 100.0 / maxOccupancy)); }
    if (yellowCount > 0) { occupancyParts.append("background-color: #FFD66E;"); occupancyWidths.append(qMin(100.0, yellowCount * 100.0 / maxOccupancy)); }
    if (emptyCount > 0) { occupancyParts.append("background-color: #36343B;"); occupancyWidths.append(qMin(100.0, emptyCount * 100.0 / maxOccupancy)); }

    QString occupancyHtml = "<table width='100%' height='100%' style='border-collapse: collapse;'><tr>";
    for(int i = 0; i < occupancyParts.size(); ++i) {
        QString style = occupancyParts[i];
        if (occupancyParts.size() == 1) {
            style += "border-radius: 5px;";
        } else {
            if (i == 0) { style += "border-top-left-radius: 5px; border-bottom-left-radius: 5px;"; }
            if (i == occupancyParts.size() - 1) { style += "border-top-right-radius: 5px; border-bottom-right-radius: 5px;"; }
        }
        occupancyHtml += QString("<td width='%1%' style='%2'></td>").arg(occupancyWidths[i]).arg(style);
    }
    occupancyHtml += "</tr></table>";
    ui->occupancyBarLabel->setText(occupancyHtml);

    // Update Revenue
    QMap<QString, double> revenue = DataManager::instance().getDailyRevenue();
    double tempRevenue = revenue.value("temporary");
    double monthlyRevenue = revenue.value("monthly");
    double totalRevenue = tempRevenue + monthlyRevenue;

    ui->revenueStatsLabel->setText(QString("总收入: ¥%1 | 临时车: ¥%2 | 月卡: ¥%3")
                               .arg(totalRevenue, 0, 'f', 2)
                               .arg(tempRevenue, 0, 'f', 2)
                               .arg(monthlyRevenue, 0, 'f', 2));

    QList<QString> revenueParts;
    QList<double> revenueWidths;
    if (totalRevenue > 0) {
        if (tempRevenue > 0) { revenueParts.append("background-color: #FFD66E;"); revenueWidths.append(tempRevenue * 100.0 / totalRevenue); }
        if (monthlyRevenue > 0) { revenueParts.append("background-color: #8CE0A8;"); revenueWidths.append(monthlyRevenue * 100.0 / totalRevenue); }
    }
    if (revenueParts.isEmpty()) {
        revenueParts.append("background-color: #36343B;"); revenueWidths.append(100.0);
    }

    QString revenueHtml = "<table width='100%' height='100%' style='border-collapse: collapse;'><tr>";
    for(int i = 0; i < revenueParts.size(); ++i) {
        QString style = revenueParts[i];
        if (revenueParts.size() == 1) {
            style += "border-radius: 5px;";
        } else {
            if (i == 0) { style += "border-top-left-radius: 5px; border-bottom-left-radius: 5px;"; }
            if (i == revenueParts.size() - 1) { style += "border-top-right-radius: 5px; border-bottom-right-radius: 5px;"; }
        }
        revenueHtml += QString("<td width='%1%' style='%2'></td>").arg(revenueWidths[i]).arg(style);
    }
    revenueHtml += "</tr></table>";
    ui->revenueBarLabel->setText(revenueHtml);
}

void MainWindow::on_pushButtonIntelligentBind_clicked()
{
    QDialog bindDialog(this);
    bindDialog.setWindowTitle("智能绑定车辆");

    // Main layout
    QVBoxLayout *mainLayout = new QVBoxLayout(&bindDialog);

    // Image preview
    QLabel *imagePreviewLabel = new QLabel();
    imagePreviewLabel->setObjectName("labelImagePreview");
    imagePreviewLabel->setFixedSize(400, 300);
    imagePreviewLabel->setStyleSheet("border: 1px solid #49454F; border-radius: 16px; background-color: #211F26; color: #938F99;");
    imagePreviewLabel->setAlignment(Qt::AlignCenter);
    imagePreviewLabel->setText("图片预览");

    // Result label
    QLabel *resultLabel = new QLabel("请上传车辆图片进行识别");
    resultLabel->setObjectName("labelRecognitionResult");
    resultLabel->setAlignment(Qt::AlignCenter);

    // Upload button
    QPushButton *uploadButton = new QPushButton("上传图片");
    uploadButton->setObjectName("pushButtonUpload"); // Reuse style

    mainLayout->addWidget(imagePreviewLabel);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(resultLabel);
    mainLayout->addSpacing(20);
    mainLayout->addWidget(uploadButton);

    // Connection using a lambda
    connect(uploadButton, &QPushButton::clicked, this, [&]() {
        QString filePath = QFileDialog::getOpenFileName(&bindDialog, "选择图片", "", "Images (*.png *.xpm *.jpg *.bmp)");
        if (filePath.isEmpty()) {
            return;
        }

        QPixmap pixmap(filePath);
        imagePreviewLabel->setPixmap(pixmap.scaled(imagePreviewLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));

        CPlateRecognize pr;
        pr.setResultShow(false);
        pr.setDetectType(PR_DETECT_CMSER | PR_DETECT_COLOR);
        std::vector<CPlate> plateVec;
        cv::Mat image = cv::imread(filePath.toStdString());
        if (image.empty()) {
            QMessageBox::warning(&bindDialog, "识别失败", "无法读取所选图片。");
            return;
        }
        int result = pr.plateRecognize(image, plateVec);

        if (result == 0 && !plateVec.empty()) {
            CPlate plate = plateVec.front();
            QString formattedPlate = formatPlateNumber(plate.getPlateStr());
            if (formattedPlate.isEmpty()) {
                resultLabel->setText("识别结果无效，请尝试更清晰的图片");
                QMessageBox::warning(&bindDialog, "识别失败", "未能识别出有效车牌。");
                return;
            }
            
            resultLabel->setText(QString("识别成功: %1").arg(formattedPlate));

            auto vehicle = DataManager::instance().findVehicleByPlate(formattedPlate);
            if (vehicle) {
                if (!vehicle->owner.isEmpty() && vehicle->owner != m_currentUser.username) {
                    QMessageBox::warning(this, "绑定失败", QString("车辆 %1 已被用户 %2 绑定。").arg(formattedPlate).arg(vehicle->owner));
                    return;
                }
                if (vehicle->owner == m_currentUser.username) {
                    QMessageBox::information(this, "绑定成功", QString("车辆 %1 已绑定到您的账户。").arg(formattedPlate));
                    bindDialog.accept();
                    return;
                }

                vehicle->owner = m_currentUser.username;
                if (DataManager::instance().updateVehicle(*vehicle)) {
                    QMessageBox::information(this, "绑定成功", QString("车辆 %1 绑定成功！").arg(formattedPlate));
                    displayMyVehicles(); // Refresh view
                    bindDialog.accept();
                } else {
                    QMessageBox::warning(this, "绑定失败", "更新车辆信息时出错。");
                }
            } else {
                QMessageBox::warning(this, "绑定失败", "未在停车场中找到该车牌号的车辆，请确认车辆已入场。");
            }
        } else {
            resultLabel->setText("识别失败，请尝试更清晰的图片");
            QMessageBox::warning(this, "识别失败", "未能在图片中识别出车牌。");
        }
    });

    bindDialog.exec();
}

void MainWindow::on_pushButtonIntelligentQuery_clicked()
{
    QDialog queryDialog(this);
    queryDialog.setWindowTitle("智能识别查询");

    // Main layout
    QVBoxLayout *mainLayout = new QVBoxLayout(&queryDialog);

    // Image preview
    QLabel *imagePreviewLabel = new QLabel();
    imagePreviewLabel->setFixedSize(400, 300);
    imagePreviewLabel->setStyleSheet("border: 1px solid #49454F; border-radius: 16px; background-color: #211F26; color: #938F99;");
    imagePreviewLabel->setAlignment(Qt::AlignCenter);
    imagePreviewLabel->setText("图片预览");

    // Result label
    QLabel *resultLabel = new QLabel("请上传车辆图片进行识别");
    resultLabel->setAlignment(Qt::AlignCenter);

    // Upload button
    QPushButton *uploadButton = new QPushButton("上传图片");
    uploadButton->setObjectName("pushButtonUpload"); // Reuse style

    mainLayout->addWidget(imagePreviewLabel);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(resultLabel);
    mainLayout->addSpacing(20);
    mainLayout->addWidget(uploadButton);

    connect(uploadButton, &QPushButton::clicked, this, [&]() {
        QString filePath = QFileDialog::getOpenFileName(&queryDialog, "选择图片", "", "Images (*.png *.xpm *.jpg *.bmp)");
        if (filePath.isEmpty()) {
            return;
        }

        QPixmap pixmap(filePath);
        imagePreviewLabel->setPixmap(pixmap.scaled(imagePreviewLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));

        CPlateRecognize pr;
        pr.setResultShow(false);
        pr.setDetectType(PR_DETECT_CMSER | PR_DETECT_COLOR);
        std::vector<CPlate> plateVec;
        cv::Mat image = cv::imread(filePath.toStdString());
        if (image.empty()) {
            QMessageBox::warning(&queryDialog, "识别失败", "无法读取所选图片。");
            return;
        }
        int result = pr.plateRecognize(image, plateVec);

        if (result == 0 && !plateVec.empty()) {
            CPlate plate = plateVec.front();
            QString formattedPlate = formatPlateNumber(plate.getPlateStr());
            if (formattedPlate.isEmpty()) {
                resultLabel->setText("识别结果无效，请尝试更清晰的图片");
                QMessageBox::warning(&queryDialog, "识别失败", "未能识别出有效车牌。");
                return;
            }
            
            resultLabel->setText(QString("识别成功: %1").arg(formattedPlate));

            // Set the text of the line edit on the main window
            ui->lineEditQueryPlate->setText(formattedPlate);
            
            // Programmatically click the query button
            ui->pushButtonQuery->click();
            
            // Close the dialog
            queryDialog.accept();

        } else {
            resultLabel->setText("识别失败，请尝试更清晰的图片");
            QMessageBox::warning(&queryDialog, "识别失败", "未能在图片中识别出车牌。");
        }
    });

    queryDialog.exec();
}
