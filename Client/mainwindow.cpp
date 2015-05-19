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

    for (int i = 0; i < GAME_SIZE; i++)
    {
        for (int j = 0; j < GAME_SIZE; j++)
        {
            xy[i][j] = new QPushButton();
            xy[i][j]->resize(BUTTON_SIZE, BUTTON_SIZE);
            xy[i][j]->move(BUTTON_SIZE*j, BUTTON_SIZE*i);
            xy[i][j]->setText(" ");
            controlsLayout->addWidget(xy[i][j], i, j);
            connect(xy[i][j], SIGNAL(clicked()), this, SLOT(onPushButtonGameClicked()));
        }
    }

    addressLineEdit = new QLineEdit(this);
    addressLineEdit->resize(GAME_SIZE * BUTTON_SIZE, BUTTON_SIZE);
    addressLineEdit->move(0, GAME_SIZE * BUTTON_SIZE);
    addressLineEdit->setText("127.0.0.1");

    buttonConnect = new QPushButton(this);
    buttonConnect->setText("Connect to the server above");
    buttonConnect->resize(GAME_SIZE * BUTTON_SIZE, BUTTON_SIZE);
    buttonConnect->move(0, (GAME_SIZE + 1) * BUTTON_SIZE);

    controlsLayout->addWidget(addressLineEdit);
    controlsLayout->addWidget(buttonConnect);

    connect(buttonConnect, SIGNAL(clicked()), this, SLOT(onPushButtonConnectClicked()));
    mainWidget->setLayout(controlsLayout);
    setCentralWidget(mainWidget);
    delete controlsLayout;
    enableDisabledGame(false);
    mainWidget->show();
}


MainWindow::~MainWindow()
{
    for (int i = 0; i < GAME_SIZE; i++)
        delete* xy[i];
}


void MainWindow::onPushButtonGameClicked()
{
    buttons_clicked++;
    game_gameoverCheck();
    for (int i = 0; i < GAME_SIZE; i++) {
        for (int j = 0; j < GAME_SIZE; j++) {
            if (xy[i][j] == QObject::sender())
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
                    else    //if move is LEGAL
                    {
                        game_moveDo(x1,y1,x2,y2);
                        game_kingCheck();

                        this->enableDisabledGame(false);    //disable game

                        // SEND TO SERVER what we have done
                        MsgAboutGame message;
                        message.happened = MOVE_MAKE;
                        message.x_old = x1;
                        message.y_old = y1;
                        message.x_new = x2;
                        message.y_new = y2;
                        char buffer[sizeof(MsgAboutGame)];
                        memcpy(buffer, &message, sizeof(MsgAboutGame));
                        this->socket->write(buffer, sizeof(MsgAboutGame));
                    }
                    buttons_clicked = 0;
                }
            }
        }
    }
}


void MainWindow::onPushButtonConnectClicked()
{
    socket = new QTcpSocket(this);
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
    else    // socket slots making and trying to connect
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
    this->buttonConnect->setEnabled(true);
}


void MainWindow::onDisconnect()
{
    this->addressLineEdit->setEnabled(true);
    this->buttonConnect->setText("Connect to the server above");
    this->buttonConnect->setEnabled(true);
    game_clearBoard();
    enableDisabledGame(false);
}


void MainWindow::socketError(QAbstractSocket::SocketError)
{
    QMessageBox Box;
    Box.setText("Socket error:\n" + socket->errorString());
    Box.exec();
    this->socket->close();
}


void MainWindow::readMessage()
{
    MsgAboutGame msg;
    char buf[sizeof(MsgAboutGame)];
    while (this->socket->read(buf, sizeof(MsgAboutGame)) > 0)
    {
        memcpy(&msg, buf, sizeof(MsgAboutGame));

        switch (msg.happened)    // <--------   WHAT MESSAGE RECEIVED FROM SERVER ??
        {
            case MOVE_MAKE:
            {
                game_moveCheck(msg.x_old,msg.y_old,msg.x_new,msg.y_new);
                game_moveDo(msg.x_old,msg.y_old,msg.x_new,msg.y_new);
                game_kingCheck();
                game_gameoverCheck();
            }; break;

            case FULL_SERVER:
            {
                QMessageBox Box;
                Box.setText("Sorry, the server is already full.");
                Box.exec();
                this->socket->disconnectFromHost();
                this->socket->close();
            }; break;

            case ELSE_DISCONNECT:   // nr 3
            {
                if (game_running == 1)
                {
                    QMessageBox Box;
                    Box.setText("Sorry, the opponent left the game.");
                    Box.exec();
                }
                if (game_running == 0)
                {
                    QMessageBox Box;
                    Box.setText("GAME OVER!");
                    Box.exec();
                }
                game_clearBoard();
                this->addressLineEdit->setEnabled(true);
                this->buttonConnect->setText("Connect to the server above");
                this->buttonConnect->setEnabled(true);
                enableDisabledGame(false);
                this->socket->disconnectFromHost();
                this->socket->close();
            }; break;

            case LOST_GAME:
            {
                game_running = 0;
                QMessageBox Box;
                Box.setText("GAME OVER!");
                Box.exec();

                game_clearBoard();
                this->addressLineEdit->setEnabled(true);
                this->buttonConnect->setText("Connect to the server above");
                this->buttonConnect->setEnabled(true);
                enableDisabledGame(false);
                this->socket->disconnectFromHost();
                this->socket->close();
            }; break;

            case CLIENT_CONNECTED:
            {
                this->buttonConnect->setEnabled(false);
                this->addressLineEdit->setEnabled(false);
            }; break;

            case CLIENT_SECOND_CONNECTED:
            {
                this->buttonConnect->setEnabled(false);
                this->addressLineEdit->setEnabled(false);;
                this->game_startNew();
            }; break;

            case YOUR_TURN_IS:
            {
                this->enableDisabledGame(true);
            }; break;
        }
    } /* end while(this->socket->read(.....) ========= */
}


void MainWindow::enableDisabledGame(bool _enable)
{
    for (int i = 0; i < GAME_SIZE; i++)
    {
        for (int j = 0; j < GAME_SIZE; j++)
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

    /*    FOR PLAYING    */
//    for(int i = 0; i < GAME_SIZE; i++)
//    {
//        for(int j = 0; j < GAME_SIZE; j++)
//        {
//            if((i == 0 || i == 2) && (j%2 == 1))
//                xy[i][j]->setText("b");
//            if((i == 1) && (j%2 == 0))
//                xy[i][j]->setText("b");
//            if((i == 6) && (j%2 == 1))
//                xy[i][j]->setText("r");
//            if((i == 5 || i == 7) && (j%2 == 0))
//                xy[i][j]->setText("r");
//        }
//    }

    /*   TESTING ONLY!!!!!!!  */
    xy[2][5]->setText("b");
    xy[2][3]->setText("b");
    xy[5][2]->setText("r");
    xy[5][4]->setText("r");    //*/
}


void MainWindow::game_clearBoard()
{
    buttons_clicked = 0;
    x1 = -1, y1 = -1, x2 = -1, y2 = -1;

    for(int i = 0; i < GAME_SIZE; i++)
    {
        for(int j = 0; j < GAME_SIZE; j++)
        {
             xy[i][j]->setText(" ");
        }
    }
}


bool MainWindow::game_moveCheck(int row1, int column1, int row2, int column2)
{
    // It checks if a non-king piece is moving backwards.
    if (xy[row1][column1]->text() != "B" && xy[row1][column1]->text() != "R")
    {
        if ((turn == 'B' && row2 < row1) || (turn == 'R' && row2 > row1))
        {
            leap = false;
            return false;
        }
    }

    // It checks if the location the piece is moving to is already taken.
    if (xy[row2][column2]->text() != " ")
    {
        leap = false;
        return false;
    }

    // It checks if location entered by the user contains a piece to be moved.
    if (xy[row1][column1]->text() == " ")
    {
        leap = false;
        return false;
    }

    // It checks if the piece isn't moving diagonally.
    if (column1 == column2 || row1 == row2)
    {
        leap = false;
        return false;
    }

    // It checks if the piece is moving by more than 1 column and only 1 row
    if ((column2 > column1 + 1 || column2 < column1 - 1) && (row2 == row1 + 1 || row2 == row1 - 1))
    {
        leap = false;
        return false;
    }

    // It checks if the piece is leaping.
    if (row2 > row1 + 1 || row2 < row1 - 1)
    {
        // It checks if the piece is leaping too far.
        if (row2 > row1 + 2 || row2 < row1 - 2)
        {
            leap = false;
            return false;
        }

        // It checks if the piece isn't moving by exactly 2 columns
        if (column2 != column1 + 2 && column2 != column1 - 2)
        {
            leap = false;
            return false;
        }

        // It checks if the piece is leaping over another piece.
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

        // Piece is not leaping too far.
        leap = true;
        return true;
    }

    // Piece is not leaping.
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
    // It removes the checker piece after (and if was!) leap.
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

/*    // Asking if wanna do another leap, and if can - does it // NOT WORKING FOR NOW!
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Question", "Do you want to do another leap if you can?",
                                QMessageBox::Yes|QMessageBox::No);
    if (reply == QMessageBox::Yes)
    {
        row1 = row2;
        column1 = column2;
        // ====== HERE SHOULD BE CLICKING NEXT LEAP IF WE HAVE CHOSEN TO =====
        for (int i = 0; i < GAME_SIZE; i++) {
            for (int j = 0; j < GAME_SIZE; j++) {
                if(xy[i][j] == QObject::sender())
                {
                    connect(xy[i][j],SIGNAL(clicked()), this,
                            SLOT(onPushButtonGameClickedForLeap(row1, column1)));
                    i = row2;
                    j = column2;
                    qDebug() << "we are 'in LEAP' row,col=" << i << "," << j;
                }
            }
        }
        xy[row1][column1]->clicked(true);
        // ====== /HERE SHOULD BE CLICKING NEXT LEAP IF WE HAVE CHOSEN TO =====

        if (turn == 'B')
            turn = 'R';
        else if (turn == 'R')
            turn = 'B';

        game_moveCheck(row1, column1, row2, column2);
        //here we probably should also not forget to send something to the server!

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
    int remaining_r = 0;
    int remaining_b = 0;

    for (int i = 0; i < GAME_SIZE; i++)
    {
        for (int j = 0; j < GAME_SIZE; j++)
        {
            // counting the remaining checkers on the game-table
            if (xy[i][j]->text() == "r" || xy[i][j]->text() == "R")
                remaining_r++;
            if (xy[i][j]->text() == "b" || xy[i][j]->text() == "B")
                remaining_b++;
        }
    }

    if (remaining_b == 0 || remaining_r == 0)
    {
        game_running = false;

        if (remaining_b == 0 || remaining_r == 0) {
            MsgAboutGame message;
            message.happened = LOST_GAME;
            message.x_old = x1;
            message.y_old = y1;
            message.x_new = x2;
            message.y_new = y2;
            char buffer[sizeof(MsgAboutGame)];
            memcpy(buffer, &message, sizeof(MsgAboutGame));
            this->socket->write(buffer, sizeof(MsgAboutGame));
        }
    }
}
