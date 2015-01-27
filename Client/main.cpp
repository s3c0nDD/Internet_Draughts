#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;

    /* set "dynamic" window's height + width */
    w.setFixedHeight((GAME_SIZE + 2) * BUTTON_SIZE);
    w.setFixedWidth(GAME_SIZE * BUTTON_SIZE);

    w.show();

    return a.exec();
}
