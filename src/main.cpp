#include "gui/mainwindow.h"
#include "gui/logindialog.h"
#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QFont>
#include <QStyleFactory>

// OpenCV test
#include <opencv2/opencv.hpp>
#include "easypr.h"
#include "core/datamanager.h"

int main(int argc, char *argv[]) {
    qDebug() << "OpenCV version:" << CV_VERSION;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    QApplication a(argc, argv);

    // Fusion style + QSS gives predictable, consistent Material rendering on Qt5
    a.setStyle(QStyleFactory::create("Fusion"));

    QFont font("Microsoft YaHei UI", 10);
    a.setFont(font);

    // EasyPR 模型改为从 exe 所在目录加载（部署时把 model 目录放在 exe 旁），
    // 不再依赖编译期写死的 C:/0Datas/... 路径；若目录缺失则回退到旧路径行为。
    {
        const QString modelDir = QCoreApplication::applicationDirPath() + QStringLiteral("/model");
        easypr::CPlateRecognize pr;
        pr.LoadSVM((modelDir + QStringLiteral("/svm_hist.xml")).toStdString());
        pr.LoadANN((modelDir + QStringLiteral("/ann.xml")).toStdString());
        pr.LoadChineseANN((modelDir + QStringLiteral("/ann_chinese.xml")).toStdString());
        pr.LoadGrayChANN((modelDir + QStringLiteral("/annCh.xml")).toStdString());
        pr.LoadChineseMapping((modelDir + QStringLiteral("/province_mapping")).toStdString());
    }

    DataManager::instance(); // 启动即初始化数据库（建表/迁移），问题尽早暴露

  a.setWindowIcon(QIcon(":/appicon.ico"));
  QFile file(":/style.qss");
  if (file.open(QFile::ReadOnly | QFile::Text)) {
      QTextStream stream(&file);
      a.setStyleSheet(stream.readAll());
      file.close();
  }

  int resultCode = 0;
  do {
      LoginDialog loginDlg;
      if (loginDlg.exec() != QDialog::Accepted) {
          resultCode = 0;
          break;
      }

      MainWindow w;
      w.setUser(loginDlg.getUser());
      w.show();
      
      resultCode = a.exec();

  } while (resultCode == 1337);

  return resultCode;
}
