#include "gui/mainwindow.h"
#include "gui/logindialog.h"
#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QFont>

// OpenCV test
#include <opencv2/opencv.hpp>

int main(int argc, char *argv[]) {
    qDebug() << "OpenCV version:" << CV_VERSION;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    QApplication a(argc, argv);

    QFont font("Microsoft YaHei UI", 9);
    a.setFont(font);

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
