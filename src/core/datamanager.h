#ifndef DATAMANAGER_H
#define DATAMANAGER_H

#include "core/user.h"
#include "core/vehicle.h"
#include <QVector>
#include <QSqlDatabase>
#include <QMap>
#include <QSharedPointer>

class DataManager {
public:
    static DataManager &instance();
    ~DataManager();

    UserRole validateUser(const QString &username, const QString &password, User *outUser = nullptr);
    bool addUser(const QString &username, const QString &password);
    bool isUsernameTaken(const QString &username) const;

    QVector<User> getUsers();
    bool updateUser(const User &user);
    bool deleteUser(const QString &username);

    // SHA-256 哈希（登录成功后会把历史明文密码自动升级为哈希）
    static QString hashPassword(const QString &password);

    QVector<Vehicle> findVehiclesByOwner(const QString &owner);

    void addVehicle(const Vehicle &vehicle);
    bool updateVehicle(const Vehicle &vehicle);
    bool deleteVehicle(const QString &plate);
    void endParkingSession(const QString &plate);
    QVector<Vehicle> getVehicles();
    QSharedPointer<Vehicle> findVehicleByPlate(const QString &plate);
    bool upgradeToMonthlyPass(const QString &plate, int daysToAdd);

    // 离场车辆留痕（临时车离场后仍可回溯）
    QVector<Vehicle> getParkingHistory(int limit = 100) const;

    // Statistics methods
    void logTransaction(const QString &plate, double amount, const QString &type, int duration = 0);
    QMap<QString, double> getDailyRevenue();
    QMap<QString, int> getVehicleCounts();

    // 车位容量（可配置，默认 50）
    int parkingCapacity() const;

private:
    DataManager();
    void initDatabase();

    QSqlDatabase m_db;
};

#endif // DATAMANAGER_H
