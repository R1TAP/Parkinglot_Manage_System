#include "core/datamanager.h"
#include "core/plateutil.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>
#include <QCryptographicHash>
#include <QCoreApplication>
#include <QDir>

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

    // 停车历史表：临时车离场不再物理消失
    if (!query.exec("CREATE TABLE IF NOT EXISTS parking_history ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "plate TEXT NOT NULL, "
                    "owner TEXT, "
                    "vehicle_type TEXT, "
                    "vehicle_color TEXT, "
                    "entry_time TEXT NOT NULL, "
                    "exit_time TEXT NOT NULL, "
                    "fee REAL NOT NULL DEFAULT 0)")) {
        qWarning() << "Couldn't create the parking_history table:" << query.lastError();
    }

    // 配置表（车位容量等）
    if (!query.exec("CREATE TABLE IF NOT EXISTS settings ("
                    "key TEXT PRIMARY KEY, "
                    "value TEXT NOT NULL)")) {
        qWarning() << "Couldn't create the settings table:" << query.lastError();
    }
    if (!query.exec("INSERT OR IGNORE INTO settings (key, value) VALUES ('total_spaces', '50')")) {
        qWarning() << "Couldn't init settings table:" << query.lastError();
    }

    // 历史数据迁移：去掉旧格式车牌里的冒号（苏A:12345 -> 苏A12345）
    const char *normalizeSql[] = {
        "UPDATE vehicles SET plate = REPLACE(REPLACE(plate, ':', ''), '：', '')",
        "UPDATE transactions SET plate = REPLACE(REPLACE(plate, ':', ''), '：', '')",
        "UPDATE parking_history SET plate = REPLACE(REPLACE(plate, ':', ''), '：', '')"
    };
    for (const char *sql : normalizeSql) {
        if (!query.exec(sql)) {
            qWarning() << "Couldn't normalize legacy plates:" << query.lastError();
        }
    }

    // Add root user if not exists
    if (!isUsernameTaken("root")) {
        addUser("root", "password");
    }
}

QString DataManager::hashPassword(const QString &password)
{
    return QString::fromLatin1(
        QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex());
}

UserRole DataManager::validateUser(const QString &username, const QString &password, User *outUser)
{
    QSqlQuery query;
    query.prepare("SELECT username, password, role FROM users WHERE username = :username");
    query.bindValue(":username", username);
    if (query.exec() && query.next()) {
        const QString stored = query.value(1).toString();
        const QString hashed = hashPassword(password);
        if (stored == password || stored == hashed) {
            // 老库中是明文密码：登录成功后自动升级为哈希存储
            if (stored == password) {
                QSqlQuery upgrade;
                upgrade.prepare("UPDATE users SET password = :password WHERE username = :username");
                upgrade.bindValue(":password", hashed);
                upgrade.bindValue(":username", username);
                if (!upgrade.exec()) {
                    qWarning() << "Couldn't upgrade password hash:" << upgrade.lastError();
                }
            }
            if (outUser) {
                outUser->username = query.value(0).toString();
                outUser->password = hashed;
                outUser->role = static_cast<UserRole>(query.value(2).toInt());
            }
            return static_cast<UserRole>(query.value(2).toInt());
        }
    }
    return UserRole::Invalid;
}

bool DataManager::addUser(const QString &username, const QString &password)
{
    if (isUsernameTaken(username) || password.isEmpty()) {
        return false;
    }

    QSqlQuery query;
    query.prepare("INSERT INTO users (username, password, role) VALUES (:username, :password, :role)");
    query.bindValue(":username", username);
    query.bindValue(":password", hashPassword(password));
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
        u.password.clear(); // 不再把密码回显到界面
        u.role = static_cast<UserRole>(query.value(2).toInt());
        users.append(u);
    }
    return users;
}

bool DataManager::updateUser(const User &user)
{
    QSqlQuery query;
    if (user.username == "root") {
        // root 不允许被其他入口修改角色，但允许通过改密弹窗更新密码
        if (user.password.isEmpty()) {
            return true;
        }
        query.prepare("UPDATE users SET password = :password WHERE username = :username");
        query.bindValue(":password", hashPassword(user.password));
        query.bindValue(":username", user.username);
    } else if (user.password.isEmpty()) {
        // 管理端编辑用户时密码留空 = 保持原密码
        query.prepare("UPDATE users SET role = :role WHERE username = :username");
        query.bindValue(":role", static_cast<int>(user.role));
        query.bindValue(":username", user.username);
    } else {
        query.prepare("UPDATE users SET password = :password, role = :role WHERE username = :username");
        query.bindValue(":password", hashPassword(user.password));
        query.bindValue(":role", static_cast<int>(user.role));
        query.bindValue(":username", user.username);
    }

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
    Vehicle v = vehicle;
    v.plate = normalizePlate(v.plate);
    if (!isValidPlate(v.plate)) {
        qWarning() << "拒绝录入非法车牌:" << v.plate;
        return;
    }

    QSharedPointer<Vehicle> existingVehicle = findVehicleByPlate(v.plate);
    QSqlQuery query;

    if (existingVehicle) {
        // 已存在记录：更新入场时间；owner 为空时保留原车主（智能入场不“解绑”）
        QString owner = v.owner.isEmpty() ? existingVehicle->owner : v.owner;
        query.prepare("UPDATE vehicles SET owner = :owner, entryTime = :entryTime WHERE plate = :plate");
        query.bindValue(":owner", owner);
        query.bindValue(":entryTime", v.entryTime.toString(Qt::ISODate));
        query.bindValue(":plate", v.plate);
    } else {
        // 新记录：插入
        query.prepare("INSERT INTO vehicles (plate, owner, entryTime, vehicle_type, vehicle_color) "
                      "VALUES (:plate, :owner, :entryTime, :vehicle_type, :vehicle_color)");
        query.bindValue(":plate", v.plate);
        query.bindValue(":owner", v.owner);
        query.bindValue(":entryTime", v.entryTime.toString(Qt::ISODate));
        query.bindValue(":vehicle_type", v.vehicle_type);
        query.bindValue(":vehicle_color", v.vehicle_color);
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

QSharedPointer<Vehicle> DataManager::findVehicleByPlate(const QString &plate)
{
    const QString key = normalizePlate(plate);
    if (key.isEmpty()) {
        return QSharedPointer<Vehicle>();
    }

    QSqlQuery query;
    query.prepare("SELECT plate, owner, entryTime, vehicle_type, pass_expiry_date, vehicle_color FROM vehicles WHERE plate = :plate");
    query.bindValue(":plate", key);
    if (query.exec() && query.next()) {
        QSharedPointer<Vehicle> v(new Vehicle());
        v->plate = query.value(0).toString();
        v->owner = query.value(1).toString();
        v->entryTime = QDateTime::fromString(query.value(2).toString(), Qt::ISODate);
        v->vehicle_type = query.value(3).toString();
        v->pass_expiry_date = QDateTime::fromString(query.value(4).toString(), Qt::ISODate);
        v->vehicle_color = query.value(5).toString();
        return v;
    }
    return QSharedPointer<Vehicle>();
}

bool DataManager::updateVehicle(const Vehicle &vehicle)
{
    const QString key = normalizePlate(vehicle.plate);
    if (key.isEmpty()) {
        return false;
    }

    QSqlQuery query;
    query.prepare("UPDATE vehicles SET owner = :owner, vehicle_type = :vehicle_type, vehicle_color = :vehicle_color WHERE plate = :plate");
    query.bindValue(":owner", vehicle.owner);
    query.bindValue(":vehicle_type", vehicle.vehicle_type);
    query.bindValue(":vehicle_color", vehicle.vehicle_color);
    query.bindValue(":plate", key);
    if (!query.exec()) {
        qWarning() << "Couldn't update vehicle:" << query.lastError();
        return false;
    }
    return true;
}

bool DataManager::deleteVehicle(const QString &plate)
{
    const QString key = normalizePlate(plate);
    if (key.isEmpty()) {
        return false;
    }

    QSqlQuery query;
    query.prepare("DELETE FROM vehicles WHERE plate = :plate");
    query.bindValue(":plate", key);
    if (!query.exec()) {
        qWarning() << "Couldn't delete vehicle:" << query.lastError();
        return false;
    }
    return true;
}

bool DataManager::upgradeToMonthlyPass(const QString &plate, int daysToAdd)
{
    // 续费叠加：以「现有到期日与当前时间中较晚者」为起点加天数，避免未到期续费损失天数
    QDateTime base = QDateTime::currentDateTime();
    QSharedPointer<Vehicle> existing = findVehicleByPlate(plate);
    if (existing && existing->pass_expiry_date.isValid() && existing->pass_expiry_date > base) {
        base = existing->pass_expiry_date;
    }
    const QDateTime expiryDate = base.addDays(daysToAdd);

    QSqlQuery query;
    query.prepare("UPDATE vehicles SET pass_expiry_date = :expiry, entryTime = :entry WHERE plate = :plate");
    query.bindValue(":expiry", expiryDate.toString(Qt::ISODate));
    query.bindValue(":entry", QDateTime::currentDateTime().toString(Qt::ISODate)); // Reset entry time to clear old fees
    query.bindValue(":plate", normalizePlate(plate));

    if (!query.exec()) {
        qWarning() << "Couldn't upgrade to monthly pass:" << query.lastError();
        return false;
    }
    return true;
}

void DataManager::endParkingSession(const QString &plate)
{
    const QString key = normalizePlate(plate);
    QSharedPointer<Vehicle> vehicle = findVehicleByPlate(key);
    if (!vehicle) {
        return; // Vehicle not found, nothing to do
    }

    const bool isMonthly = vehicle->isMonthlyPassHolder();

    if (isMonthly) {
        // For monthly pass holders, just clear the entry time to mark them as not parked
        QSqlQuery query;
        query.prepare("UPDATE vehicles SET entryTime = '' WHERE plate = :plate");
        query.bindValue(":plate", key);
        if (!query.exec()) {
            qWarning() << "Couldn't end parking session for monthly vehicle:" << query.lastError();
        }
        return;
    }

    // 临时车：先写入停车历史，再删除在场记录
    QSqlQuery history;
    history.prepare("INSERT INTO parking_history (plate, owner, vehicle_type, vehicle_color, entry_time, exit_time, fee) "
                    "VALUES (:plate, :owner, :vehicle_type, :vehicle_color, :entry_time, :exit_time, :fee)");
    history.bindValue(":plate", key);
    history.bindValue(":owner", vehicle->owner);
    history.bindValue(":vehicle_type", vehicle->vehicle_type);
    history.bindValue(":vehicle_color", vehicle->vehicle_color);
    history.bindValue(":entry_time", vehicle->entryTime.toString(Qt::ISODate));
    history.bindValue(":exit_time", QDateTime::currentDateTime().toString(Qt::ISODate));
    history.bindValue(":fee", vehicle->calculateFee());
    if (!history.exec()) {
        qWarning() << "Couldn't archive parking session:" << history.lastError();
    }

    deleteVehicle(key);
}

QVector<Vehicle> DataManager::getParkingHistory(int limit) const
{
    QVector<Vehicle> vehicles;
    QSqlQuery query;
    query.prepare("SELECT plate, owner, entry_time, exit_time, vehicle_type, vehicle_color, fee "
                  "FROM parking_history ORDER BY id DESC LIMIT :limit");
    query.bindValue(":limit", limit);
    if (query.exec()) {
        while (query.next()) {
            Vehicle v;
            v.plate = query.value(0).toString();
            v.owner = query.value(1).toString();
            v.entryTime = QDateTime::fromString(query.value(2).toString(), Qt::ISODate);
            v.exitTime = QDateTime::fromString(query.value(3).toString(), Qt::ISODate);
            v.vehicle_type = query.value(4).toString();
            v.vehicle_color = query.value(5).toString();
            v.fee = query.value(6).toDouble();
            vehicles.append(v);
        }
    }
    return vehicles;
}

void DataManager::logTransaction(const QString &plate, double amount, const QString &type, int duration)
{
    QSqlQuery query;
    query.prepare("INSERT INTO transactions (plate, amount, timestamp, type, duration) "
                  "VALUES (:plate, :amount, :timestamp, :type, :duration)");
    query.bindValue(":plate", normalizePlate(plate));
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

    const QDateTime dayStart(QDate::currentDate(), QTime(0, 0, 0));
    const QDateTime dayEnd = dayStart.addDays(1);

    // 临时车收入：只查今天的流水（SQL 侧过滤，不再全表拉到内存）
    QSqlQuery tempQuery;
    tempQuery.prepare("SELECT amount FROM transactions "
                      "WHERE type = 'temporary' AND timestamp >= :start AND timestamp < :end");
    tempQuery.bindValue(":start", dayStart.toString(Qt::ISODate));
    tempQuery.bindValue(":end", dayEnd.toString(Qt::ISODate));
    if (tempQuery.exec()) {
        while (tempQuery.next()) {
            revenue["temporary"] += tempQuery.value(0).toDouble();
        }
    }

    // 月卡收入：按“今天处于生效期”的月卡均摊日收入
    QSqlQuery monthlyQuery;
    monthlyQuery.prepare("SELECT amount, timestamp, duration FROM transactions "
                         "WHERE type = 'monthly' AND timestamp < :end");
    monthlyQuery.bindValue(":end", dayEnd.toString(Qt::ISODate));
    if (monthlyQuery.exec()) {
        while (monthlyQuery.next()) {
            QDateTime startDate = QDateTime::fromString(monthlyQuery.value(1).toString(), Qt::ISODate);
            int duration = monthlyQuery.value(2).toInt();
            if (duration > 0) {
                QDateTime endDate = startDate.addDays(duration);
                if (QDate::currentDate() >= startDate.date() && QDate::currentDate() < endDate.date()) {
                    revenue["monthly"] += monthlyQuery.value(0).toDouble() / duration;
                }
            }
        }
    }
    return revenue;
}

int DataManager::parkingCapacity() const
{
    QSqlQuery query("SELECT value FROM settings WHERE key = 'total_spaces'");
    if (query.exec() && query.next()) {
        int capacity = query.value(0).toInt();
        return capacity > 0 ? capacity : 50;
    }
    return 50;
}
