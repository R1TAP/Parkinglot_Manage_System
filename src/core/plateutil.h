#ifndef PLATEUTIL_H
#define PLATEUTIL_H

#include <QString>
#include <QRegularExpression>

// 车牌规范化：去掉空格、冒号（半角/全角）、横线等噪音，字母统一大写。
// EasyPR 旧格式会输出 "苏A:12345"，历史数据里也可能带冒号，统一在此收敛。
inline QString normalizePlate(const QString &raw)
{
    QString plate = raw;
    plate.remove(QStringLiteral(" "));
    plate.remove(QStringLiteral(":"));
    plate.remove(QStringLiteral("："));
    plate.remove(QStringLiteral("-"));
    plate.remove(QStringLiteral("_"));
    plate.remove(QStringLiteral("."));
    plate.remove(QStringLiteral("·"));
    return plate.toUpper();
}

// 大陆民用车牌：1 个汉字 + 1 个字母 + 5~6 位字母数字
// （6 位后缀兼容新能源 8 位绿牌，如 苏AD12345）
inline bool isValidPlate(const QString &plate)
{
    // PCRE2 语法：汉字范围用 \x{4e00}-\x{9fa5}
    static const QRegularExpression re(QStringLiteral("^[\\x{4e00}-\\x{9fa5}][A-Z][A-Z0-9]{5,6}$"));
    return re.match(plate).hasMatch();
}

#endif // PLATEUTIL_H
