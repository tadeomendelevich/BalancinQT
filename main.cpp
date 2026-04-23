#include "mainwindow.h"

#include <QApplication>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Fusion style respects QSS 100% — el estilo nativo de Windows
    // ignora parcialmente el stylesheet en QSplitter, QScrollBar, etc.
    QApplication::setStyle(QStyleFactory::create("Fusion"));

    MainWindow w;
    w.show();
    return a.exec();
}

