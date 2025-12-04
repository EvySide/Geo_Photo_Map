#include "mainwindow.h"
#include <QApplication>
#include <QCoreApplication>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QtWebEngine/QtWebEngine>
#else
#include <QtWebEngineCore/QtWebEngineCore>
#endif

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QApplication a(argc, argv);
    //QtWebEngine::initialize();
    MainWindow w;
    w.show();
    return a.exec();
}
