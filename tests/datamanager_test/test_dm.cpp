#include "core/datamanager.h"
#include "core/plateutil.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDateTime>
#include <QFile>
#include <QTextStream>

static int g_failures = 0;
static QTextStream g_log;
static QFile g_logFile;

static void check(bool ok, const QString &name)
{
    g_log << (ok ? "PASS " : "FAIL ") << name << "\n";
    g_log.flush();
    if (ok) {
    } else {
        ++g_failures;
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    // 每次运行使用全新数据库，避免上次运行的数据干扰断言
    QFile::remove(QCoreApplication::applicationDirPath() + "/parking.db");
    g_logFile.setFileName("test_result.txt");
    g_logFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);
    g_log.setDevice(&g_logFile);
    DataManager &dm = DataManager::instance();

    // 1. 密码哈希 / 登录 / 老明文兼容
    check(!dm.isUsernameTaken("alice"), "alice not taken");
    check(dm.addUser("alice", "pw123"), "addUser alice");
    check(!dm.addUser("alice", "other"), "addUser duplicate rejected");
    check(dm.validateUser("alice", "pw123") == UserRole::Visitor, "alice login ok");
    check(dm.validateUser("alice", "wrong") == UserRole::Invalid, "alice wrong password rejected");
    check(dm.validateUser("root", "password") == UserRole::Admin, "root default login ok");

    // 2. root 改密（此前被禁止）
    User root;
    root.username = "root";
    root.password = "newpass";
    check(dm.updateUser(root), "root password change allowed");
    check(dm.validateUser("root", "newpass") == UserRole::Admin, "root new password ok");
    check(dm.validateUser("root", "password") == UserRole::Invalid, "root old password rejected");

    // 3. 密码留空 = 保持原密码
    User alice;
    alice.username = "alice";
    alice.password.clear();
    alice.role = UserRole::Visitor;
    check(dm.updateUser(alice), "updateUser keep password");
    check(dm.validateUser("alice", "pw123") == UserRole::Visitor, "alice password preserved");

    // 4. 车牌规范化 + 格式校验
    check(normalizePlate("苏A:12345") == "苏A12345", "normalize colon plate");
    check(normalizePlate(" 苏A-12345 ") == "苏A12345", "normalize dash/space");
    check(isValidPlate("苏A12345"), "7-char plate valid");
    check(isValidPlate("苏AD12345"), "8-char green plate valid");
    check(!isValidPlate("蓝"), "color-only garbage invalid");
    check(!isValidPlate("苏A1234"), "too short invalid");

    // 5. 智能入场规范化入库 + 已有车主不被清空
    Vehicle v;
    v.plate = "苏A:12345";
    v.entryTime = QDateTime::currentDateTime();
    v.vehicle_type = "蓝";
    v.vehicle_color = "蓝";
    dm.addVehicle(v);
    auto found = dm.findVehicleByPlate("苏A12345");
    check(found && found->plate == "苏A12345", "plate normalized in DB");
    check(dm.findVehicleByPlate("苏A:12345") != nullptr, "query with colon still works");

    if (found) {
        found->owner = "alice";
        check(dm.updateVehicle(*found), "bind vehicle to alice");
    }
    Vehicle reentry = v;
    reentry.plate = "苏A12345";
    reentry.owner.clear(); // 智能入场不带 owner
    reentry.entryTime = QDateTime::currentDateTime();
    dm.addVehicle(reentry);
    auto after = dm.findVehicleByPlate("苏A12345");
    check(after && after->owner == "alice", "owner preserved on re-entry");

    // 6. 月卡续费叠加
    check(dm.upgradeToMonthlyPass("苏A12345", 30), "first monthly pass");
    auto m1 = dm.findVehicleByPlate("苏A12345");
    check(dm.upgradeToMonthlyPass("苏A12345", 30), "renew monthly pass");
    auto m2 = dm.findVehicleByPlate("苏A12345");
    check(m1 && m2 && m2->pass_expiry_date > m1->pass_expiry_date, "renewal stacks remaining days");

    // 7. 临时车离场写入停车历史
    Vehicle temp;
    temp.plate = "苏B99999";
    temp.entryTime = QDateTime::currentDateTime().addSecs(-7200);
    temp.vehicle_type = "绿";
    temp.vehicle_color = "绿";
    dm.addVehicle(temp);
    dm.endParkingSession("苏B99999");
    check(dm.findVehicleByPlate("苏B99999") == nullptr, "temp vehicle removed from active list");
    const QVector<Vehicle> history = dm.getParkingHistory(10);
    bool archived = false;
    for (const Vehicle &h : history) {
        if (h.plate == "苏B99999" && h.fee >= 0.0 && h.exitTime.isValid()) {
            archived = true;
        }
    }
    check(archived, "temp vehicle archived to history with fee");

    // 8. 容量配置
    check(dm.parkingCapacity() == 50, "default capacity 50");

    g_log << (g_failures == 0 ? "ALL TESTS PASSED" : QString("FAILURES: %1").arg(g_failures)) << "\n";
    g_log.flush();
    g_logFile.close();
    return g_failures == 0 ? 0 : 1;
}
