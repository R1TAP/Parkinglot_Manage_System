# SmartParkingSystem

此项目为第一次项目练手，包含大量AI参与的前端界面，可能含有大量问题，仅供学习经验分享。

此项目是基于 Qt/C++ 的现代化停车场智能管理系统桌面应用，集成 SQLite 本地数据存储与 EasyPR(OpenCV) 车牌识别能力，支持管理员与访客双角色、车辆进出场、临停收费、月卡办理、营收与车位统计等完整业务流程。

## 项目亮点

- 桌面端一体化：Qt Widgets 构建完整业务界面，开箱即用。
- 智能识别：支持图片上传识别车牌，实现智能入场、智能离场、智能绑定与智能查询。
- 双角色权限：管理员/访客分权，界面按角色动态裁剪。
- 本地数据库：SQLite 持久化用户、车辆、交易数据，自动建表与兼容迁移。
- 业务闭环：从车辆入场、计费、扫码支付、离场到统计分析完整覆盖。

## 技术栈

- 开发语言：C++
- UI 框架：Qt 5 (Core / Gui / Widgets / Sql)
- 数据库：SQLite (QSQLITE)
- 图像与识别：OpenCV 3.4.5 + EasyPR
- 构建系统：qmake

## 主要功能

### 1. 登录与注册

- 登录鉴权（支持角色识别）
- 访客注册（保留用户名 `root` 不允许注册）
- 默认管理员账户自动初始化：
  - 用户名：`root`
  - 密码：`password`

### 2. 车辆管理（管理员）

- 新增/编辑/删除车辆
- 按车牌查询车辆在场状态
- 实时显示当前费用（每 5 秒刷新）
- 车牌颜色与车型映射显示（蓝/绿/黄）

### 3. 智能识别

- 智能入场：上传车辆图片，识别车牌后自动入库
- 智能离场：上传车辆图片，识别后触发缴费与离场
- 智能查询：上传图片识别后自动填充车牌并执行查询

### 4. 用户与绑定（访客）

- 普通绑定：输入车牌绑定当前账号
- 智能绑定：上传图片识别车牌并绑定
- 我的车辆页面：查看已绑车辆、在场时长、当前费用
- 修改密码

### 5. 月卡与支付

- 月卡套餐：30/90/365 天
- 办理月卡后可按月卡身份进出场（免临停费）
- 临停缴费与月卡办理均支持二维码支付流程（界面模拟）

### 6. 数据统计（管理员）

- 车位占用统计（默认总车位可修改）
- 今日收入统计（临停 + 月卡按日摊销）
- 可视化比例条实时更新

## 收费规则

### 临停计费

- 前 30 分钟免费
- 超过 30 分钟后按小时向上取整计费
- 燃油车（蓝牌）：5 元/小时，封顶 50 元
- 新能源（绿牌）：3 元/小时，封顶 50 元
- 大型车（黄牌）：10 元/小时，封顶 70 元

### 月卡逻辑

- 办理后记录到期时间
- 月卡有效期内停车费用为 0
- 月卡车辆离场时仅清空入场状态，不删除车辆档案
- 临停车辆缴费离场后从在场记录中移除

## 系统架构

```text
src/
  main.cpp                  # 程序入口、样式加载、登录态循环
  core/
    datamanager.*           # SQLite 数据访问层（单例）
    user.h                  # 用户模型与角色枚举
    vehicle.h               # 车辆模型与计费逻辑
  gui/
    mainwindow.*            # 主业务窗口与核心流程
    logindialog.*           # 登录
    registerdialog.*        # 注册
    vehicledialog.*         # 车辆编辑
    userdialog.*            # 用户编辑
    bindvehicledialog.*     # 车辆绑定
    changepassworddialog.*  # 修改密码
    monthpassdialog.*       # 月卡办理
forms/                      # Qt Designer UI 文件
styles/style.qss            # 全局样式
resources.qrc               # 资源索引
```

## 数据库设计

程序首次启动会在可执行文件同目录自动创建 `parking.db`。

### users

- `username` TEXT PRIMARY KEY
- `password` TEXT NOT NULL
- `role` INTEGER NOT NULL

### vehicles

- `plate` TEXT PRIMARY KEY
- `owner` TEXT
- `entryTime` TEXT NOT NULL
- `vehicle_type` TEXT DEFAULT '蓝'
- `pass_expiry_date` TEXT
- `vehicle_color` TEXT DEFAULT '蓝'

### transactions

- `id` INTEGER PRIMARY KEY AUTOINCREMENT
- `plate` TEXT
- `amount` REAL NOT NULL
- `timestamp` TEXT NOT NULL
- `type` TEXT NOT NULL (`temporary` / `monthly`)
- `duration` INTEGER

## 运行环境

- Windows (推荐)
- Qt 5.11+（需包含 `Qt SQL` 模块）
- 支持 C++11 的编译器（如 MinGW / MSVC）
- OpenCV 3.4.5（需与 `.pro` 中库名一致）
- EasyPR 源码目录（项目通过 `EasyPRLib` 源码方式编译）

## 构建与运行

### 1. 克隆项目

```bash
git clone <your-repo-url>
cd SmartParkingSystem
```

### 2. 配置 OpenCV 路径

编辑 `System.pro`，按本机环境修改以下内容：

- `INCLUDEPATH += <opencv>/include`
- `LIBS += -L<opencv>/lib -lopencv_core...`

> 注意：当前工程文件中使用了本地绝对路径，换机后必须调整。

### 3. 打开并编译

- 使用 Qt Creator 打开 `System.pro`
- 选择 Kit（MinGW 或 MSVC）
- `qmake` -> `Build`

### 4. 运行

- 启动程序后进入登录页
- 可使用默认管理员账户登录

## 权限说明

- 管理员：可访问车辆管理、账户管理、统计、智能入场/离场等完整功能。
- 访客：聚焦“我的”页面，仅保留绑定车辆、智能绑定、缴费、月卡、改密等个人功能。

## 项目资源

- 应用图标：`resources/appicon.ico`
- 缴费二维码：`resources/PayNew.png`
- 月卡二维码：`resources/Month.png`
- 下拉箭头图标：`resources/Arrowhead-Down.png`

## 已知注意事项

- 工程依赖 OpenCV 与 EasyPR 的本地环境，跨机器时需同步配置。
- 当前账户密码为明文存储，生产场景建议改为哈希存储（如 bcrypt/argon2）。
- 月卡年卡价格在 UI 文案与业务代码中可能存在不一致，建议统一配置为单一来源。

## 后续可扩展方向

- 接入摄像头实时识别（替代手动上传）
- 增加导出报表（CSV/Excel/PDF）
- 增加操作日志与审计追踪
- 引入密码加盐哈希、权限细粒度控制
- 将车位容量、费率、月卡价格改为可配置项

# 鸣谢

- 此项目基于开源的EasyPR库，在此感谢：https://github.com/liuruoze/EasyPR