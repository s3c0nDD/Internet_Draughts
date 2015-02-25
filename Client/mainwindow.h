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
#include <QAbstractSocket>
#include <QHostInfo>


/* enum to send and reveive about type of msg to/from the server */
enum info{
    MOVE_MAKE = 0,
    MOVE_BEATEN = 1,
    LOST_GAME = 2,
    ELSE_DISCONNECT = 3,
    FULL_SERVER = 4,
    CLIENT_CONNECTED = 5,
    CLIENT_SECOND_CONNECTED = 6,
    YOUR_TURN_IS = 7
    //TODO - add / edit if neccesary in development
};


class MsgAboutGame{

public:
    char x_old;     // old 'x' position of draught
    char y_old;     // old 'y' position of draught
    char x_new;     // new 'x' position of draught
    char y_new;     // new 'y' position of draught
    char x_beat;  // 'x' position of beated draught, if none=-1
    char y_beat;  // 'y' position of beated draught
    info happened;     // 2 server about client's state | definition above^
    //standard konstruktor
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
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();


private:
    Ui::MainWindow *ui;
    QPushButton *xy[GAME_SIZE][GAME_SIZE];
    QLineEdit *addressLineEdit;
    QPushButton *buttonConnect;
    QTcpSocket *socket;
    QChar turn;
    QString tmp;
    bool leap, game_running;
    int x1, y1, x2, y2, buttons_clicked;

private slots:
    void onPushButtonGameClicked();
    void onPushButtonConnectClicked();
    /* ========  SOCKET SLOTS METHODS ========  */
    void hostLookedUp(const QHostInfo& info);
    void onConnect();
    void onDisconnect();
    void socketError(QAbstractSocket::SocketError);
    void readMessage();
    //=====================
    void enableDisabledGame(bool _enable);
    /* ===========  GAME LOGIC HERE =========== */
    void game_startNew();
    void game_clearBoard();
    bool game_moveCheck(int row1, int column1, int row2, int column2);
    void game_moveDo(int row1, int column1, int row2, int column2);
    void game_leapDo(int row1, int column1, int row2, int column2);
    void game_kingCheck();
    void game_gameoverCheck();
};    

#endif // MAINWINDOW_H
