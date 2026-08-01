# SmartParkingSystem

此项目为**第一次项目练手**，包含大量**AI参与的前端界面与部分业务逻辑**，可能含有大量问题，**仅供学习经验分享**。

此项目是基于 **Qt 5 / C++** 的现代化停车场智能管理系统桌面应用：集成 **SQLite 本地存储 + EasyPR(OpenCV 3.4.5)** 车牌识别能力，支持**管理员/访客双角色、车辆进出场、临停计费、月卡办理、营收与车位统计**等完整业务流程。

## 项目亮点

- 桌面应用程序：Qt Widgets 构建完整业务界面，开箱即用。
- UI风格：Material Design 3 风格，简洁易读。
- 智能识别：支持图片上传识别车牌（模拟摄像头自动识别），实现智能入场、智能离场、智能绑定与智能查询。
- 双角色权限：管理员/访客分权，界面动态裁剪。
- 本地数据库：SQLite 持久化用户、车辆、交易数据，自动建表，兼容迁移。
- 业务闭环：车辆入场、计费、扫码支付、离场、统计分析完整覆盖。

## 技术栈

- 开发语言：C++ 11
- UI框架：Qt 5.15
- 数据库：SQLite
- 图像与识别：OpenCV 3.4.5 + EasyPR
- 构建系统：Qmake

## 目录结构

```text
SmartParkingSystem.pro      # Qt工程文件
resources.qrc               # 资源索引
src/
  main.cpp                  # 程序入口、样式加载、模型路径、登录态循环
  core/
    datamanager.*           # SQLite 数据访问层
    plateutil.h             # 车牌规范化与格式校验
    user.h / vehicle.h      # 模型与计费规则
  gui/                      # 窗口与业务流程
forms/                      # Qt Designer 界面文件
styles/style.qss            # Material Design 3 深色主题
resources/                  # 资源文件
EasyPRLib/                  # EasyPR 运行模型
tests/datamanager_test/     # 数据层回归测试
scripts/deploy.ps1          # 部署脚本
```

## 主要功能

### 1. 登录与注册

- 登录鉴权：（用户角色识别）
- 访客注册：（`root` 保护）
- 默认管理员账户自动初始化：
  - 用户名：`root`
  - 密码：`password`
- **密码保护**：以 SHA-256 哈希存储

### 2. 车辆管理（管理员）

- **新增/编辑/删除/查询**车辆
- 按车牌查询车辆在场状态
- 在场费用实时刷新
- 车牌颜色与车型映射显示（**蓝/绿/黄** 车牌分类）

### 3. **智能识别**

- 智能入场：检测车辆图片，识别车牌后自动入库
- 智能离场：检测车辆图片，识别后触发缴费与离场
- 智能查询：上传图片识别，自动填充车牌并执行查询
- 识别结果进行车牌格式与颜色校验，防止无效结果入库

### 4. 用户与绑定（访客）

- 普通绑定：输入车牌绑定当前账号
- 智能绑定：上传图片识别车牌并绑定
- **绑定保护**：自动筛查绑定信息，防止误绑定
- 我的车辆页面：查看已绑车辆、在场时长、当前费用
- 修改密码

### 5. 月卡与支付

- 月卡逻辑：30/90/365 天，续费自动叠加剩余天数
- 办理月卡后可按月卡身份进出场（免临停费）
- 临停缴费与月卡办理均支持二维码支付流程（界面模拟）

### 6. 数据统计（管理员）

- 车位占用统计（默认总车位可修改）
- 今日收入统计（临停 + 月卡按日摊销）
- 可视化比例条实时更新
- 停车历史：临时车离场自动归档到 `parking_history`

## 收费规则（可随意调整）

- 前 30 分钟免费
- 超过 30 分钟后按小时向上取整计费
- 燃油车（蓝牌）：5 元/小时，封顶 50 元
- 新能源（绿牌）：3 元/小时，封顶 50 元
- 大型车（黄牌）：10 元/小时，封顶 70 元
- 月卡有效期内停车免费；离场仅清空入场状态，保留档案

## 构建要求

- Windows10（或更高） + Qt 5.15（MinGW 8.1 64 位，含SQL）
- OpenCV 3.4.5（MinGW）（`libopencv_*345.dll.a`）

### 配置 OpenCV 路径

项目通过环境变量 `OPENCV_DIR` 定位 OpenCV 构建目录（含 `install/include` 与 `lib`）：

```powershell
$env:OPENCV_DIR = "D:\path\to\opencv-3.4.5\build"
```

### Qt Creator 配置

打开 `SmartParkingSystem.pro`，使用 `Qt 5.15 MinGW 64-bit Kit` 构建。

## 运行与部署

程序在可执行文件同目录自动创建 `parking.db`；车牌识别模型从可执行文件旁 `model/` 目录加载。

支持一键部署：

```powershell
powershell -ExecutionPolicy Bypass -File scripts\deploy.ps1
```

## 鸣谢与项目帮助

提示：
- EasyPR 为 SVM+ANN ，对倾斜/低照度/模糊图识别率有限
- OpenCV 与 Qt 需配置 `OPENCV_DIR` 与 Qt Kit。

鸣谢：
- 此项目基于开源的EasyPR库，在此感谢：https://github.com/liuruoze/EasyPR
