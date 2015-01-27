#ifndef MAINWINDOW_H
#define MAINWINDOW_H

/*********************************************************************
 * GAME SIZE - the size of board game, because some of Draughts ..
 *             .. types can have different sizes (and rules too!):
 *     => 8 - Standard/English/American Draughts
 *     => 10 - Polish = International Draughts
 *     => 12 - Canadian Draughts
 *
 * NOTE: Engine won't be known which exact until coded and sent 2 repo.
 *********************************************************************
 * BUTTON_SIZE - determining the size of game_window + buttons + etc.
 *********************************************************************
 * GAME_PORT - this is the port by whick game client try to connect
 *********************************************************************/
#define GAME_SIZE 8
#define BUTTON_SIZE 35
#define GAME_PORT 11111

#include <QMainWindow>
#include <QPushButton>
#include <QGridLayout>
#include <QLineEdit>
#include <QTcpSocket>


/* structure to send info to the server */
enum info{

    MOVE = 0,
    WIN = 1,
    LOST = 2,
    OPPONENT_DISCONNECT = 3,
    OTHERS = 4   //TODO - add / edit if neccesary in development

};


class MsgToServer{  //TEMPORARY, may change

public:
    char x_old;     // old 'x' position of draught
    char y_old;     // old 'y' position of draught
    char x_new;     // new 'x' position of draught
    char y_new;     // new 'y' position of draught
    info inf2srv;   // 2 server about client's state | definition above^
    //info infFROMsrv? - info FROM server received by client

};


namespace Ui {
class MainWindow;
}


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    QPushButton *xy[GAME_SIZE][GAME_SIZE];
    QLineEdit *addressLineEdit;
    QPushButton *buttonConnect;
    QTcpSocket *socket;
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;


private slots:
    void onPushButtonGameClicked();
    void onPushButtonConnectClicked();
    void onConnect();
    void onDisconnect();
    void socketError();
    void readMessage();
    void enableDisabledGame(bool _enable);
};    

#endif // MAINWINDOW_H
