#ifndef USER_H
#define USER_H

#include <QString>

// Define user roles
enum class UserRole {
    Admin,
    Visitor,
    Invalid // Represents a failed login
};

struct User {
    QString username;
    QString password;
    UserRole role; // Add role member
};

#endif // USER_H
