#ifndef DATAMANAGER_H
#define DATAMANAGER_H

#include "core/user.h"
#include "core/vehicle.h"
#include <QVector>
#include <QSqlDatabase>
#include <QMap>

class DataManager {
public:
    static DataManager &instance();
    ~DataManager(); // Add a destructor to close the database

    UserRole validateUser(const QString &username, const QString &password, User *outUser = nullptr);
    bool addUser(const QString &username, const QString &password);
    bool isUsernameTaken(const QString &username) const;

    QVector<User> getUsers();
    bool updateUser(const User &user);
    bool deleteUser(const QString &username);

    QVector<Vehicle> findVehiclesByOwner(const QString &owner);

    void addVehicle(const Vehicle &vehicle);
    bool updateVehicle(const Vehicle &vehicle);
    bool deleteVehicle(const QString &plate);
    void endParkingSession(const QString &plate);
    QVector<Vehicle> getVehicles(); // Changed from returning reference to value
    Vehicle* findVehicleByPlate(const QString &plate);
    bool upgradeToMonthlyPass(const QString &plate, int daysToAdd);

    // Statistics methods
    void logTransaction(const QString &plate, double amount, const QString &type, int duration = 0);
    QMap<QString, double> getDailyRevenue();
    QMap<QString, int> getVehicleCounts();

private:
    DataManager();
    void initDatabase(); // A new private method to handle DB initialization

    QSqlDatabase m_db; // The database connection object

    // The m_users and m_vehicles vectors will be removed as they are no longer needed
};

#endif // DATAMANAGER_H
