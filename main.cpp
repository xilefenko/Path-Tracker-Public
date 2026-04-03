#include "mainwindow.h"
#include "PathTrackerStruct.h"

#include <QApplication>
#include <QFile>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    /*
    if (!QFile::exists("Save.json") && QFile::exists("OldSave.json"))
    {
        PathTrackerStruct& data = PathTrackerStruct::instance();
        data.loadLegacy("OldSave.json");
        data.save("Save.json");
    }

    qWarning() << "Old Save Exists:" << (QFile::exists("OldSave.json") ? ("True") : ("False"));

    */

    MainWindow w;
    w.show();
    return a.exec();
}
