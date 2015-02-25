#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#define GAME_PORT 11111

#include <QMainWindow>
#include <QTcpServer>
#include <QTcpSocket>
#include <QString>
#include <QProcess>
#include <stdlib.h>


/* structure to send info to the server */
enum info{
    MOVE_MAKE = 0,
    LOST_GAME = 1,
    ELSE_DISCONNECT = 2,
    FULL_SERVER = 3,
    CLIENT_CONNECTED = 4,
    CLIENT_SECOND_CONNECTED = 5,
    YOUR_TURN_IS = 6
    //TODO - add / edit if neccesary in development
};


class MsgAboutGame{

public:
    char x_old;     // old 'x' position of draught
    char y_old;     // old 'y' position of draught
    char x_new;     // new 'x' position of draught
    char y_new;     // new 'y' position of draught
    char x_beat;     // 'x' position of beated draught, if none=-1
    char y_beat;     // 'y' position of beated draught
    info happened;   // 2 server about client's state | definition above^
    /* standard konstruktor */
    MsgAboutGame()
    {
        x_old = y_old = x_new = y_new = x_beat = y_beat = 0;
        happened = MOVE_MAKE;
    }
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
    void readAnswer();
};

#endif // MAINWINDOW_H
