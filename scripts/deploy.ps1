# 一键部署：把 Qt5 Release 构建产物整理为可独立运行的目录
# 用法：powershell -ExecutionPolicy Bypass -File scripts\deploy.ps1
param(
    [string]$BuildDir  = "build/Desktop_Qt_5_15_19_MinGW_64_bit-Release/release",
    [string]$OutDir    = "deploy",
    [string]$QtBin     = "C:\Qt\5.15.19\mingw81_64\bin",
    [string]$MinGwBin  = "C:\Qt\Tools\mingw810_64\bin",
    [string]$OpenCVDir = $env:OPENCV_DIR
)

$ErrorActionPreference = "Stop"
$root = Split-Path $PSScriptRoot -Parent
$buildExe = Join-Path $root "$BuildDir\SmartParkingSystem.exe"
if (-not (Test-Path -LiteralPath $buildExe)) {
    throw "未找到构建产物: $buildExe（请先构建 Release）"
}
if (-not $OpenCVDir) {
    $OpenCVDir = "C:/0Datas/Program/Zero/SmartParkingSystem/opencv-3.4.5/build"
}

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

# 1. 主程序
Copy-Item -LiteralPath $buildExe -Destination $OutDir -Force

# 2. Qt 核心 DLL 与 MinGW 运行库
Copy-Item -Path "$QtBin\Qt5Core.dll","$QtBin\Qt5Gui.dll","$QtBin\Qt5Sql.dll","$QtBin\Qt5Widgets.dll" -Destination $OutDir -Force
Copy-Item -Path "$MinGwBin\libgcc_s_seh-1.dll","$MinGwBin\libstdc++-6.dll","$MinGwBin\libwinpthread-1.dll" -Destination $OutDir -Force
foreach ($extra in @('d3dcompiler_47.dll','opengl32sw.dll')) {
    if (Test-Path -LiteralPath "$QtBin\$extra") { Copy-Item -LiteralPath "$QtBin\$extra" -Destination $OutDir -Force }
}

# 3. Qt 插件（平台 / 图像格式 / SQL 驱动）
foreach ($plugin in @('platforms','imageformats','sqldrivers')) {
    Copy-Item -Path "$QtBin\..\plugins\$plugin" -Destination $OutDir -Recurse -Force
}

# 4. OpenCV DLL（仅工程实际链接的模块）
foreach ($mod in @('core','highgui','imgcodecs','imgproc','features2d','ml','objdetect')) {
    $dll = Join-Path $OpenCVDir "bin\libopencv_${mod}345.dll"
    if (Test-Path -LiteralPath $dll) { Copy-Item -LiteralPath $dll -Destination $OutDir -Force }
}

# 5. EasyPR 模型
Copy-Item -Path (Join-Path $root "EasyPRLib\model") -Destination $OutDir -Recurse -Force

Write-Host "部署完成: $OutDir"
