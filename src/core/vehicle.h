#ifndef VEHICLE_H
#define VEHICLE_H

#include <QString>
#include <QDateTime>
#include <cmath>

struct Vehicle {
    QString plate;
    QString owner;
    QDateTime entryTime;
    QDateTime exitTime;
    QString vehicle_type;
    QString vehicle_color;
    QDateTime pass_expiry_date;

    bool isMonthlyPassHolder() const {
        return pass_expiry_date.isValid() && QDateTime::currentDateTime() < pass_expiry_date;
    }

    double calculateFee() const {
        if (isMonthlyPassHolder() || !entryTime.isValid()) {
            return 0.0;
        }

        double fee = 0.0;
        qint64 durationMinutes = entryTime.secsTo(QDateTime::currentDateTime()) / 60;

        if (durationMinutes > 30) {
            double durationHours = std::ceil(durationMinutes / 60.0);
            if (vehicle_type == "蓝") {
                fee = durationHours * 5;
                if (fee > 50) fee = 50;
            } else if (vehicle_type == "绿") {
                fee = durationHours * 3;
                if (fee > 50) fee = 50;
            } else if (vehicle_type == "黄") {
                fee = durationHours * 10;
                if (fee > 70) fee = 70;
            }
        }
        return fee;
    }
};

#endif // VEHICLE_H
