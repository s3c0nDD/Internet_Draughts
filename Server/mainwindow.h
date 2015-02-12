#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#define GAME_PORT 11111

#include <QMainWindow>
#include <QTcpServer>
#include <QTcpSocket>
#include <QString>
#include <stdlib.h>
#include <QProcess>


/* structure to send info to the server */
enum info{

    MOVE = 0,
    WIN = 1,
    LOST = 2,
    OPPONENT_DISCONNECT = 3,
    FULL = 4,
    OTHERS = 5   //TODO - add / edit if neccesary in development

};


class MsgAboutGame{  //TEMPORARY, may change

public:
    char x_old;     // old 'x' position of draught
    char y_old;     // old 'y' position of draught
    char x_new;     // new 'x' position of draught
    char y_new;     // new 'y' position of draught
    info happened;   // 2 server about client's state | definition above^
    //info infFROMsrv? - info FROM server received by client
};


namespace Ui {
class MainWindow;
}


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    QTcpServer *server;
    QTcpSocket *socket[2];


private slots:
    void onNewConnection();
    void onDisconnected();
    void read();
};

#endif // MAINWINDOW_H
