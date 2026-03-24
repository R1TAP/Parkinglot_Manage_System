#include "core/datamanager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>

DataManager &DataManager::instance()
{
    static DataManager inst;
    return inst;
}

DataManager::DataManager()
{
    initDatabase();
}

DataManager::~DataManager()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
}

#include <QCoreApplication>
#include <QDir>

void DataManager::initDatabase()
{
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    QString dbPath = QCoreApplication::applicationDirPath() + QDir::separator() + "parking.db";
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        qWarning() << "Error: connection with database failed:" << m_db.lastError();
        return;
    }

    QSqlQuery query;

    // Create users table
    if (!query.exec("CREATE TABLE IF NOT EXISTS users ( "
                    "username TEXT PRIMARY KEY, "
                    "password TEXT NOT NULL, "
                    "role INTEGER NOT NULL)")) {
        qWarning() << "Couldn't create the users table:" << query.lastError();
    }

    // Create vehicles table
    if (!query.exec("CREATE TABLE IF NOT EXISTS vehicles ("
                    "plate TEXT PRIMARY KEY, "
                    "owner TEXT, "
                    "entryTime TEXT NOT NULL)")) {
        qWarning() << "Couldn't create the vehicles table:" << query.lastError();
    }

    // Check and add new columns to vehicles table for backward compatibility (migration)
    QSqlQuery pragmaQuery("PRAGMA table_info(vehicles)");
    bool typeColumnExists = false;
    bool expiryColumnExists = false;
    bool colorColumnExists = false;
    while (pragmaQuery.next()) {
        if (pragmaQuery.value(1).toString() == "vehicle_type") {
            typeColumnExists = true;
        }
        if (pragmaQuery.value(1).toString() == "pass_expiry_date") {
            expiryColumnExists = true;
        }
        if (pragmaQuery.value(1).toString() == "vehicle_color") {
            colorColumnExists = true;
        }
    }

    if (!typeColumnExists) {
        if (!query.exec("ALTER TABLE vehicles ADD COLUMN vehicle_type TEXT DEFAULT '蓝'")) {
            qWarning() << "Couldn't add vehicle_type column to vehicles table:" << query.lastError();
        }
    }

    if (!expiryColumnExists) {
        if (!query.exec("ALTER TABLE vehicles ADD COLUMN pass_expiry_date TEXT")) {
            qWarning() << "Couldn't add pass_expiry_date column to vehicles table:" << query.lastError();
        }
    }

    if (!colorColumnExists) {
        if (!query.exec("ALTER TABLE vehicles ADD COLUMN vehicle_color TEXT DEFAULT '蓝'")) {
            qWarning() << "Couldn't add vehicle_color column to vehicles table:" << query.lastError();
        }
    }

    // Create transactions table
    if (!query.exec("CREATE TABLE IF NOT EXISTS transactions ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "plate TEXT, "
                    "amount REAL NOT NULL, "
                    "timestamp TEXT NOT NULL, "
                    "type TEXT NOT NULL, "
                    "duration INTEGER)")) {
        qWarning() << "Couldn't create the transactions table:" << query.lastError();
    }
    
    // Add root user if not exists
    if (!isUsernameTaken("root")) {
        addUser("root", "password");
    }
}

UserRole DataManager::validateUser(const QString &username, const QString &password, User *outUser)
{
    QSqlQuery query;
    query.prepare("SELECT username, password, role FROM users WHERE username = :username");
    query.bindValue(":username", username);
    if (query.exec() && query.next()) {
        if (query.value(1).toString() == password) {
            if (outUser) {
                outUser->username = query.value(0).toString();
                outUser->password = query.value(1).toString();
                outUser->role = static_cast<UserRole>(query.value(2).toInt());
            }
            return static_cast<UserRole>(query.value(2).toInt());
        }
    }
    return UserRole::Invalid;
}

bool DataManager::addUser(const QString &username, const QString &password)
{
    if (isUsernameTaken(username)) {
        return false;
    }

    QSqlQuery query;
    query.prepare("INSERT INTO users (username, password, role) VALUES (:username, :password, :role)");
    query.bindValue(":username", username);
    query.bindValue(":password", password);
    // New users are visitors, unless it's the root user being created for the first time
    query.bindValue(":role", (username == "root") ? static_cast<int>(UserRole::Admin) : static_cast<int>(UserRole::Visitor));

    if (!query.exec()) {
        qWarning() << "Couldn't add user:" << query.lastError();
        return false;
    }
    return true;
}

QVector<Vehicle> DataManager::findVehiclesByOwner(const QString &owner)
{
    QVector<Vehicle> vehicles;
    QSqlQuery query;
    query.prepare("SELECT plate, owner, entryTime, vehicle_type, pass_expiry_date, vehicle_color FROM vehicles WHERE owner = :owner");
    query.bindValue(":owner", owner);
    if (query.exec()) {
        while (query.next()) {
            Vehicle v;
            v.plate = query.value(0).toString();
            v.owner = query.value(1).toString();
            v.entryTime = QDateTime::fromString(query.value(2).toString(), Qt::ISODate);
            v.vehicle_type = query.value(3).toString();
            v.pass_expiry_date = QDateTime::fromString(query.value(4).toString(), Qt::ISODate);
            v.vehicle_color = query.value(5).toString();
            vehicles.append(v);
        }
    }
    return vehicles;
}

bool DataManager::isUsernameTaken(const QString &username) const
{
    QSqlQuery query;
    query.prepare("SELECT username FROM users WHERE username = :username");
    query.bindValue(":username", username);
    return query.exec() && query.next();
}

QVector<User> DataManager::getUsers()
{
    QVector<User> users;
    QSqlQuery query("SELECT username, password, role FROM users");
    while (query.next()) {
        User u;
        u.username = query.value(0).toString();
        u.password = query.value(1).toString();
        u.role = static_cast<UserRole>(query.value(2).toInt());
        users.append(u);
    }
    return users;
}

bool DataManager::updateUser(const User &user)
{
    if (user.username == "root") return false; // Cannot modify root

    QSqlQuery query;
    query.prepare("UPDATE users SET password = :password, role = :role WHERE username = :username");
    query.bindValue(":password", user.password);
    query.bindValue(":role", static_cast<int>(user.role));
    query.bindValue(":username", user.username);
    if (!query.exec()) {
        qWarning() << "Couldn't update user:" << query.lastError();
        return false;
    }
    return true;
}

bool DataManager::deleteUser(const QString &username)
{
    if (username == "root") return false; // Cannot delete root

    QSqlQuery query;
    query.prepare("DELETE FROM users WHERE username = :username");
    query.bindValue(":username", username);
    if (!query.exec()) {
        qWarning() << "Couldn't delete user:" << query.lastError();
        return false;
    }
    return true;
}

void DataManager::addVehicle(const Vehicle &vehicle)
{
    Vehicle* existingVehicle = findVehicleByPlate(vehicle.plate);
    QSqlQuery query;

    if (existingVehicle) {
        // If vehicle exists, just update its entry time and owner (i.e., start a new session)
        delete existingVehicle; // free memory from findVehicleByPlate
        query.prepare("UPDATE vehicles SET owner = :owner, entryTime = :entryTime WHERE plate = :plate");
        query.bindValue(":owner", vehicle.owner);
        query.bindValue(":entryTime", vehicle.entryTime.toString(Qt::ISODate));
        query.bindValue(":plate", vehicle.plate);
    } else {
        // If vehicle does not exist, insert a new record
        query.prepare("INSERT INTO vehicles (plate, owner, entryTime, vehicle_type, vehicle_color) "
                        "VALUES (:plate, :owner, :entryTime, :vehicle_type, :vehicle_color)");
        query.bindValue(":plate", vehicle.plate);
        query.bindValue(":owner", vehicle.owner);
        query.bindValue(":entryTime", vehicle.entryTime.toString(Qt::ISODate));
        query.bindValue(":vehicle_type", vehicle.vehicle_type);
        query.bindValue(":vehicle_color", vehicle.vehicle_color);
    }

    if (!query.exec()) {
        qWarning() << "Couldn't add or update vehicle:" << query.lastError();
    }
}

QVector<Vehicle> DataManager::getVehicles()
{
    QVector<Vehicle> vehicles;
    // Only get currently parked vehicles
    QSqlQuery query("SELECT plate, owner, entryTime, vehicle_type, pass_expiry_date, vehicle_color FROM vehicles WHERE entryTime IS NOT NULL AND entryTime != ''");
    while (query.next()) {
        Vehicle v;
        v.plate = query.value(0).toString();
        v.owner = query.value(1).toString();
        v.entryTime = QDateTime::fromString(query.value(2).toString(), Qt::ISODate);
        v.vehicle_type = query.value(3).toString();
        v.pass_expiry_date = QDateTime::fromString(query.value(4).toString(), Qt::ISODate);
        v.vehicle_color = query.value(5).toString();
        vehicles.append(v);
    }
    return vehicles;
}

// Note: This function now returns a dynamically allocated object. The caller is responsible for deleting it.
Vehicle* DataManager::findVehicleByPlate(const QString &plate)
{
    QSqlQuery query;
    query.prepare("SELECT plate, owner, entryTime, vehicle_type, pass_expiry_date, vehicle_color FROM vehicles WHERE plate = :plate");
    query.bindValue(":plate", plate);
    if (query.exec() && query.next()) {
        Vehicle *v = new Vehicle();
        v->plate = query.value(0).toString();
        v->owner = query.value(1).toString();
        v->entryTime = QDateTime::fromString(query.value(2).toString(), Qt::ISODate);
        v->vehicle_type = query.value(3).toString();
        v->pass_expiry_date = QDateTime::fromString(query.value(4).toString(), Qt::ISODate);
        v->vehicle_color = query.value(5).toString();
        return v;
    }
    return nullptr;
}

bool DataManager::updateVehicle(const Vehicle &vehicle)
{
    QSqlQuery query;
    query.prepare("UPDATE vehicles SET owner = :owner, vehicle_type = :vehicle_type, vehicle_color = :vehicle_color WHERE plate = :plate");
    query.bindValue(":owner", vehicle.owner);
    query.bindValue(":vehicle_type", vehicle.vehicle_type);
    query.bindValue(":vehicle_color", vehicle.vehicle_color);
    query.bindValue(":plate", vehicle.plate);
    if (!query.exec()) {
        qWarning() << "Couldn't update vehicle:" << query.lastError();
        return false;
    }
    return true;
}

bool DataManager::deleteVehicle(const QString &plate)
{
    QSqlQuery query;
    query.prepare("DELETE FROM vehicles WHERE plate = :plate");
    query.bindValue(":plate", plate);
    if (!query.exec()) {
        qWarning() << "Couldn't delete vehicle:" << query.lastError();
        return false;
    }
    return true;
}

bool DataManager::upgradeToMonthlyPass(const QString &plate, int daysToAdd)
{
    QDateTime expiryDate = QDateTime::currentDateTime().addDays(daysToAdd);
    QSqlQuery query;
    query.prepare("UPDATE vehicles SET pass_expiry_date = :expiry, entryTime = :entry WHERE plate = :plate");
    query.bindValue(":expiry", expiryDate.toString(Qt::ISODate));
    query.bindValue(":entry", QDateTime::currentDateTime().toString(Qt::ISODate)); // Reset entry time to clear old fees
    query.bindValue(":plate", plate);

    if (!query.exec()) {
        qWarning() << "Couldn't upgrade to monthly pass:" << query.lastError();
        return false;
    }
    return true;
}

void DataManager::endParkingSession(const QString &plate)
{
    Vehicle* vehicle = findVehicleByPlate(plate);
    if (!vehicle) {
        return; // Vehicle not found, nothing to do
    }

    bool isMonthly = vehicle->pass_expiry_date.isValid() && QDateTime::currentDateTime() < vehicle->pass_expiry_date;

    if (isMonthly) {
        // For monthly pass holders, just clear the entry time to mark them as not parked
        QSqlQuery query;
        query.prepare("UPDATE vehicles SET entryTime = '' WHERE plate = :plate");
        query.bindValue(":plate", plate);
        if (!query.exec()) {
            qWarning() << "Couldn't end parking session for monthly vehicle:" << query.lastError();
        }
    } else {
        // For temporary vehicles, delete the record entirely
        deleteVehicle(plate);
    }
    delete vehicle; // Clean up memory from findVehicleByPlate
}

void DataManager::logTransaction(const QString &plate, double amount, const QString &type, int duration)
{
    QSqlQuery query;
    query.prepare("INSERT INTO transactions (plate, amount, timestamp, type, duration) "
                    "VALUES (:plate, :amount, :timestamp, :type, :duration)");
    query.bindValue(":plate", plate);
    query.bindValue(":amount", amount);
    query.bindValue(":timestamp", QDateTime::currentDateTime().toString(Qt::ISODate));
    query.bindValue(":type", type);
    query.bindValue(":duration", duration);

    if (!query.exec()) {
        qWarning() << "Couldn't log transaction:" << query.lastError();
    }
}

QMap<QString, int> DataManager::getVehicleCounts()
{
    QMap<QString, int> counts;
    counts.insert("蓝", 0);
    counts.insert("绿", 0);
    counts.insert("黄", 0);

    QSqlQuery query("SELECT vehicle_type FROM vehicles WHERE entryTime IS NOT NULL AND entryTime != ''");
    while (query.next()) {
        QString type = query.value(0).toString();
        if (counts.contains(type)) {
            counts[type]++;
        }
    }
    return counts;
}

QMap<QString, double> DataManager::getDailyRevenue()
{
    QMap<QString, double> revenue;
    revenue.insert("temporary", 0.0);
    revenue.insert("monthly", 0.0);
    QDate today = QDate::currentDate();

    // Temporary revenue: Sum of all payments made today
    QSqlQuery tempQuery;
    tempQuery.prepare("SELECT amount, timestamp FROM transactions WHERE type = 'temporary'");
    if (tempQuery.exec()) {
        while (tempQuery.next()) {
            QDateTime timestamp = QDateTime::fromString(tempQuery.value(1).toString(), Qt::ISODate);
            if (timestamp.date() == today) {
                revenue["temporary"] += tempQuery.value(0).toDouble();
            }
        }
    }

    // Monthly revenue: Add daily rate for all currently active monthly passes
    QSqlQuery monthlyQuery;
    monthlyQuery.prepare("SELECT amount, timestamp, duration FROM transactions WHERE type = 'monthly'");
    if (monthlyQuery.exec()) {
        while (monthlyQuery.next()) {
            QDateTime startDate = QDateTime::fromString(monthlyQuery.value(1).toString(), Qt::ISODate);
            int duration = monthlyQuery.value(2).toInt();
            if (duration > 0) {
                QDateTime endDate = startDate.addDays(duration);
                if (today >= startDate.date() && today < endDate.date()) {
                     revenue["monthly"] += monthlyQuery.value(0).toDouble() / duration;
                }
            }
        }
    }
    return revenue;
}
