#include "mainwindow.h"

#include <QApplication>
#include <QStyleFactory>
#include <QFile>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("BalancinQT");
    a.setOrganizationName("UNER");

    // Fusion style respects QSS 100% — el estilo nativo de Windows
    // ignora parcialmente el stylesheet en QSplitter, QScrollBar, etc.
    QApplication::setStyle(QStyleFactory::create("Fusion"));

    // Tema global centralizado (theme.qss): toda la estética de la app vive
    // ahí, no en estilos inline dispersos. Editar theme.qss y recompilar
    // (es un recurso embebido) para cambiar colores/estilos.
    QFile themeFile(":/style/theme.qss");
    if (themeFile.open(QFile::ReadOnly | QFile::Text)) {
        a.setStyleSheet(QString::fromUtf8(themeFile.readAll()));
    }

    MainWindow w;
    w.show();
    return a.exec();
}
