#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QGridLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QLineEdit>
#include <string>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    setWindowTitle("NetDraughts - Client");
    QWidget *mainWidget = new QWidget;
    QGridLayout *controlsLayout = new QGridLayout;

    int temp = 1;
    for(int i = 0; i < GAME_SIZE; i++)
    {
        for (int j = 0; j < GAME_SIZE; j++)
        {
            if(temp <= GAME_SIZE*GAME_SIZE)
            {
                xy[i][j] = new QPushButton();
                xy[i][j]->resize(BUTTON_SIZE, BUTTON_SIZE);
                xy[i][j]->move(BUTTON_SIZE*j, BUTTON_SIZE*i);
                xy[i][j]->setText(" ");
                controlsLayout->addWidget(xy[i][j], i, j);
                connect(xy[i][j], SIGNAL(clicked()), this, SLOT(onPushButtonGameClicked()));
                temp++; // <== really needed??
            }
        }
    }

    addressLineEdit = new QLineEdit(this);
    addressLineEdit->resize(GAME_SIZE * BUTTON_SIZE, BUTTON_SIZE);
    addressLineEdit->move(0, GAME_SIZE * BUTTON_SIZE);

    buttonConnect = new QPushButton(this);
    buttonConnect->setText("Connect to the server above");
    buttonConnect->resize(GAME_SIZE * BUTTON_SIZE, BUTTON_SIZE);
    buttonConnect->move(0, (GAME_SIZE + 1) * BUTTON_SIZE);

    socket = new QTcpSocket(this);

    controlsLayout->addWidget(addressLineEdit);
    controlsLayout->addWidget(buttonConnect);

    connect(buttonConnect, SIGNAL(clicked()), this, SLOT(onPushButtonConnectClicked()));
    mainWidget->setLayout(controlsLayout);
    setCentralWidget(mainWidget);
    delete controlsLayout;
    enableDisabledGame(true);       // FINALLY SET FALSE HERE
    this->game_startNew();          // FINALLY NOT THIS LINE HERE
    mainWidget->show();
}


MainWindow::~MainWindow()
{
    for (int i = 0; i < GAME_SIZE; i++)
        delete* xy[i];
    //delete* xy;   // <= this makes programme crash
                    // ... so how to repair it ???
    delete addressLineEdit;
    delete buttonConnect;
    delete ui;
    delete socket;
}


void MainWindow::onPushButtonGameClicked()
{
    buttons_clicked++;
    for (int i = 0; i < GAME_SIZE; i++) {
        for (int j = 0; j < GAME_SIZE; j++) {
            if(xy[i][j] == QObject::sender())
            {
                if (buttons_clicked == 1)
                {
                    x1 = i;
                    y1 = j;
                    if( (turn == 'B' && !(xy[i][j]->text()=="b" || xy[i][j]->text()=="B") )
                            || (turn == 'R' && !(xy[i][j]->text()=="r" || xy[i][j]->text()=="R")) )
                    {
                        QMessageBox Box;
                        Box.setText("Not your turn!");
                        Box.exec();
                        buttons_clicked = 0;
                    }
                }
                else if (buttons_clicked == 2)
                {
                    x2 = i;
                    y2 = j;
                    if (game_moveCheck(x1,y1,x2,y2) == false)
                    {
                        QMessageBox Box;
                        Box.setText("Illegal move!");
                        Box.exec();
                        x1 = -1, y1 = -1, x2 = -1, y2 = -1;
                    }
                    else
                    {
                        game_moveDo(x1,y1,x2,y2);
                        game_kingCheck();
                        game_gameoverCheck();
                    }
                    buttons_clicked = 0;
                }
            }
        }
    }
//    for(int i = 0; i < GAME_SIZE; i++)
//      {
//        for(int j = 0; j < GAME_SIZE; j++)
//          {
//            if(xy[i][j]==QObject::sender())
//              {
//                xy[i][j]->setText($);//set letter[wWbB] on button
//                message[0] = i+65;       // cause there are f.e. 8x8 places
//                message[1] = j+65;       // and we need to "know" who is who
//                message[2] = 'X';        // so modified and "sent"2serv is this
//                ...ETC
//               }
//            //button[i][j]->setEnabled(false);
//        }
//    }
//    this->socket->write(message, strlen(message));  // write to socket
//    //enableDisable(false); // block the game for a turn....
}


void MainWindow::onPushButtonConnectClicked()
{
    this->buttonConnect->setEnabled(false);
    this->addressLineEdit->setEnabled(false);
    if (this->socket->state() == QAbstractSocket::UnconnectedState)
    {
        QHostInfo::lookupHost(this->addressLineEdit->text(), this, SLOT(hostLookedUp(QHostInfo)));
    }
    else
    {
        this->socket->disconnectFromHost();
    }
}


void MainWindow::hostLookedUp(const QHostInfo& info)
{
    if (info.error() != QHostInfo::NoError)
    {
        QMessageBox Box;
        Box.setText("Cannot find host: " + info.hostName());
        Box.exec();
        buttonConnect->setEnabled(true);
    }
    else
    {
        connect(this->socket, SIGNAL(connected()), this, SLOT(onConnect()));
        connect(this->socket, SIGNAL(disconnected()), this, SLOT(onDisconnect()));
        connect(this->socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(socketError(QAbstractSocket::SocketError)));
        connect(this->socket, SIGNAL(readyRead()), this, SLOT(readMessage()));
        this->socket->connectToHost(info.hostName(), (qint16)GAME_PORT);
    }
}


void MainWindow::onConnect()
{
    this->addressLineEdit->setEnabled(false);   // block address line
    this->buttonConnect->setText("Disconnect from the game");
    enableDisabledGame(true);                   // enable the game
    this->buttonConnect->setEnabled(true);
}


void MainWindow::onDisconnect()
{
    QMessageBox Box;
    Box.setText("Disconnected from the game");
    Box.exec();

    this->addressLineEdit->setEnabled(true);
    this->buttonConnect->setText("Connect to the server above");
    this->buttonConnect->setEnabled(true);
    enableDisabledGame(false);
    this->socket = NULL;
}


void MainWindow::socketError(QAbstractSocket::SocketError)
{
    QMessageBox Box;
    Box.setText("Error while trying to connect:\n" + socket->errorString());
    Box.exec();
    this->buttonConnect->setEnabled(true);
    this->addressLineEdit->setEnabled(true);
    this->socket = NULL;
}


void MainWindow::readMessage()
{
    MsgAboutGame msg;
    char buf[sizeof(msg)];
    while(this->socket->read(buf, sizeof(msg)) > 0)
    {
        memcpy(&msg, buf, sizeof(msg));
    }

    switch(msg.type)    // ALL TO DO
    {
    case MOVE_MAKE:
    {
        QString tmp = xy[msg.x_old][msg.y_old]->text();
        xy[msg.x_old][msg.y_old]->setText(" ");
        xy[msg.x_new][msg.y_new]->setText(tmp);
        if (msg.x_beat != GAME_SIZE*GAME_SIZE)
        {
            xy[msg.x_beat][msg.y_beat]->setText(" ");
        }
    }
    case FULL_SERVER:
    {
        QMessageBox Box;
        Box.setText("Sorry, the server is already full.");
        Box.exec();
        this->addressLineEdit->setEnabled(true);
        this->buttonConnect->setText("Connect to the server above");
        this->buttonConnect->setEnabled(true);
        enableDisabledGame(false);
        this->socket = NULL;
    }
    case ELSE_DISCONNECT:
    {
        QMessageBox Box;
        Box.setText("Sorry, the opponent left the game.");
        Box.exec();
        this->addressLineEdit->setEnabled(true);
        this->buttonConnect->setText("Connect to the server above");
        this->buttonConnect->setEnabled(true);
        enableDisabledGame(false);
        this->socket = NULL;
    }
    case LOST_GAME:
    {
        QMessageBox Box;
        Box.setText("Sorry, YOU LOST THE GAME.");
        Box.exec();
        this->addressLineEdit->setEnabled(true);
        this->buttonConnect->setText("Connect to the server above");
        this->buttonConnect->setEnabled(true);
        enableDisabledGame(false);
        this->socket = NULL;
    }
    }
}


void MainWindow::enableDisabledGame(bool _enable)
{
    for(int i = 0; i < GAME_SIZE; i++)
    {
        for(int j = 0; j < GAME_SIZE; j++)
        {
            xy[i][j]->setEnabled(_enable);
        }
    }
}


/* ============================  GAME LOGIC HERE ============================ */
void MainWindow::game_startNew()
{
    turn = 'R';
    game_running = true;
    buttons_clicked = 0;
    x1 = -1, y1 = -1, x2 = -1, y2 = -1;

    for(int i = 0; i < GAME_SIZE; i++)
    {
        for(int j = 0; j < GAME_SIZE; j++)
        {
            if((i == 0 || i == 2) && (j%2 == 1))
                xy[i][j]->setText("b");
            if((i == 1) && (j%2 == 0))
                xy[i][j]->setText("b");
            if((i == 6) && (j%2 == 1))
                xy[i][j]->setText("r");
            if((i == 5 || i == 7) && (j%2 == 0))
                xy[i][j]->setText("r");
        }
    }
}


bool MainWindow::game_moveCheck(int row1, int column1, int row2, int column2)
{
    //it checks if a non-king piece is moving backwards.
    if (xy[row1][column1]->text() != "B" && xy[row1][column1]->text() != "R")
    {
        if ((turn == 'B' && row2 < row1) || (turn == 'R' && row2 > row1))
        {
            leap = false;
            return false;
        }
    }

    //It checks if the location the piece is moving to is already taken.
    if (xy[row2][column2]->text() != " ")
    {
        leap = false;
        return false;
    }

    //It checks if location entered by the user contains a piece to be moved.
    if (xy[row1][column1]->text() == " ")
    {
        leap = false;
        return false;
    }

    //It checks if the piece isn't moving diagonally.
    if (column1 == column2 || row1 == row2)
    {
        leap = false;
        return false;
    }

    //It checks if the piece is moving by more than 1 column and only 1 row
    if ((column2 > column1 + 1 || column2 < column1 - 1) && (row2 == row1 + 1 || row2 == row1 - 1))
    {
        leap = false;
        return false;
    }

    //It checks if the piece is leaping.
    if (row2 > row1 + 1 || row2 < row1 - 1)
    {
        //It checks if the piece is leaping too far.
        if (row2 > row1 + 2 || row2 < row1 - 2)
        {
            leap = false;
            return false;
        }

        //It checks if the piece isn't moving by exactly 2 columns
        if (column2 != column1 + 2 && column2 != column1 - 2)
        {
            leap = false;
            return false;
        }

        //It checks if the piece is leaping over another piece.
        if (row2 > row1 && column2 > column1)
        {
            if (xy[row2-1][column2-1]->text() == " ")
            {
                leap = false;
                return false;
            }
        }
        else if (row2 > row1 && column2 < column1)
        {
            if (xy[row2-1][column2+1]->text() == " ")
            {
                leap = false;
                return false;
            }
        }
        else if (row2 < row1 && column2 > column1)
        {
            if (xy[row2+1][column2-1]->text() == " ")
            {
                leap = false;
                return false;
            }
        }
        else if (row2 < row1 && column2 < column1)
        {
            if (xy[row2+1][column2+1]->text() == " ")
            {
                leap = false;
                return false;
            }
        }

        //Piece is not leaping too far.
        leap = true;
        return true;
    }

    //Piece is not leaping.
    leap = false;
    return true;
}


void MainWindow::game_moveDo(int row1, int column1, int row2, int column2)
{
    bool is_king = false;

    if (xy[row1][column1]->text() == "B" || xy[row1][column1]->text() == "R")
    {
        is_king = true;
    }

    xy[row1][column1]->setText(" ");

    if (turn == 'B')
    {
        if (is_king == false)
        {
            xy[row2][column2]->setText("b");
        }
        else if (is_king == true)
        {
            xy[row2][column2]->setText("B");
        }

        turn = 'R';
    }
    else if (turn == 'R')
    {
        if (is_king == false)
        {
            xy[row2][column2]->setText("r");
        }
        else if (is_king == true)
        {
            xy[row2][column2]->setText("R");
        }

        turn = 'B';
    }

    if (leap == true)
    {
        game_leapDo(row1, column1, row2, column2);
    }
}


void MainWindow::game_leapDo(int row1, int column1, int row2, int column2)
{
    //It removes the checker piece after leap.
    if (row2 > row1 && column2 > column1)
    {
        xy[row2-1][column2-1]->setText(" ");
    }
    else if (row2 > row1 && column2 < column1)
    {
        xy[row2-1][column2+1]->setText(" ");
    }
    else if (row2 < row1 && column2 > column1)
    {
        xy[row2+1][column2-1]->setText(" ");
    }
    else if (row2 < row1 && column2 < column1)
    {
        xy[row2+1][column2+1]->setText(" ");
    }

/*    //Asking if wanna do another leap, and if can - does it
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Question", "Do you want to do another leap if you can?",
                                QMessageBox::Yes|QMessageBox::No);
    if (reply == QMessageBox::Yes)
    {
        row1 = row2;
        column1 = column2;
        // ====== HERE SHOULD BE CLICKING NEXT LEAP IF WE HAVE CHOSEN TO =====
//        for (int i = 0; i < GAME_SIZE; i++) {
//            for (int j = 0; j < GAME_SIZE; j++) {
//                if(xy[i][j] == QObject::sender())
//                {
//                    connect(xy[i][j],SIGNAL(clicked()), this,
//                            SLOT(onPushButtonGameClickedForLeap(row1, column1)));
//                    i = row2;
//                    j = column2;
//                    qDebug() << "jestesmy w LEAP row,col=" << i << "," << j;
//                }
//            }
//        }
//        xy[row1][column1]->clicked(true);
        // ====== /HERE SHOULD BE CLICKING NEXT LEAP IF WE HAVE CHOSEN TO =====

        if (turn == 'B')
            turn = 'R';
        else if (turn == 'R')
            turn = 'B';

        game_moveCheck(row1, column1, row2, column2);

        if (leap == false)
        {
            QMessageBox Box;
            Box.setText("This leap is invalid.");
            Box.exec();

            if (turn == 'B')
                turn = 'R';
            else if (turn == 'R')
                turn = 'B';
        }
        else if (leap == true)
           game_moveDo(row1, column1, row2, column2);
    }//*/
}


void MainWindow::game_kingCheck()
{
    for (int i = 0; i < GAME_SIZE; i++)
    {
        if (xy[0][i]->text() == "r")
        {
            xy[0][i]->setText("R");
        }
        if (xy[GAME_SIZE-1][i]->text() == "b")
        {
            xy[GAME_SIZE-1][i]->setText("B");
        }
    }
}


void MainWindow::game_gameoverCheck()
{
    int blanks = 0;

    for (int i = 0; i < GAME_SIZE; i++)
        for (int j = 0; j < GAME_SIZE; j++)
            if (xy[i][j]->text() != " ")
                blanks++;

    if (blanks > 1)
        game_running = true;
    else if (blanks == 1)
        game_running = false;
}
