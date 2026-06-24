#include "mainwindow.h"
#include <QApplication>
#include <QDir>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Ensure the "database" folder exists in the application runtime location
    QString appDir = QCoreApplication::applicationDirPath();
    QDir dir(appDir);
    if (!dir.exists("database")) {
        if (dir.mkdir("database")) {
            qDebug() << "Database folder successfully created at:" << appDir + "/database";
        } else {
            qDebug() << "Failed to create database folder.";
        }
    }

    MainWindow w;
    w.show();
    return a.exec();
}