#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMessageBox>
#include <QDebug>
#

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->plainTextEdit->clear();
    this->socket[0] = NULL;
    this->socket[1] = NULL;
    this->server = new QTcpServer;
    connect(this->server, SIGNAL(newConnection()), this, SLOT(onNewConnection()));
    this->server->listen(QHostAddress::Any, (qint16)GAME_PORT);
}

MainWindow::~MainWindow()
{
    delete this->server;
    delete ui;
}

void MainWindow::onNewConnection()
{
    MsgAboutGame message;                   /* <--  here we have variables */
    char buffer[sizeof(MsgAboutGame)];      /* <--  for whole procedure    */
    for (int i = 0; i < 3; i++)
    {
        if (i < 2 && (socket[i] == NULL))
        {
            this->socket[i] = this->server->nextPendingConnection();
            connect(this->socket[i], SIGNAL(readyRead()), this, SLOT(readAnswer()));
            connect(this->socket[i], SIGNAL(disconnected()), this, SLOT(onDisconnected()));

            message.happened = CLIENT_CONNECTED;
            memcpy(buffer, &message, sizeof(MsgAboutGame));
            this->socket[i]->write(buffer, sizeof(MsgAboutGame));

            ui->plainTextEdit->appendPlainText(QString("CLIENT CONNECTED: ") + QString::number(i));

            if(socket[0] != NULL && socket[1] != NULL)
            {
                // for both of players we have started the game
                message.happened = CLIENT_SECOND_CONNECTED;

                memcpy(buffer, &message, sizeof(MsgAboutGame));
                this->socket[0]->write(buffer, sizeof(MsgAboutGame));
                this->socket[1]->write(buffer, sizeof(MsgAboutGame));

                // for ONE player - it's his/her turn
                message.happened = YOUR_TURN_IS;
                memcpy(buffer, &message, sizeof(MsgAboutGame));
                this->socket[1]->write(buffer, sizeof(MsgAboutGame));
            }
            break;
        }
        else if (i == 2)    //if there are already 2 CLIENTs in game
        {
            ui->plainTextEdit->appendPlainText(QString("CLIENT WANTS TO CONNECT, BUT ALREADY FULL"));
            message.happened = FULL_SERVER;
            memcpy(buffer, &message, sizeof(MsgAboutGame));
            QTcpSocket *tmp = this->server->nextPendingConnection();
            tmp->write(buffer, sizeof(MsgAboutGame));
            tmp = NULL;
        }
    }
}

void MainWindow::onDisconnected()   /* on disconnection of client 0 or 1 */
{
    for(int i = 0; i < 2; i++) {
        if( this->socket[i] == QObject::sender() ) {
            this->socket[i] = NULL;
            ui->plainTextEdit->appendPlainText(QString("CLIENT DISCONNECTED: ") + QString::number(i));
            if(i != 2)  /* do only do if disconnecting is client 0 or 1 */
            {
                int who_2_send = (i ? 0 : 1);
                // do only, when there is second player connected and we CAN send him a signal/message
                if(socket[who_2_send] != NULL)
                {
                    ui->plainTextEdit->appendPlainText(QString("SENDING OTHER-CLIENT-DISCONNECTED SIGNAL TO: ") + QString::number(who_2_send));
                    MsgAboutGame message;
                    message.happened = ELSE_DISCONNECT;
                    char buffer[sizeof(MsgAboutGame)];
                    memcpy(buffer, &message, sizeof(MsgAboutGame));
                    this->socket[who_2_send]->write(buffer, sizeof(MsgAboutGame));
                    this->socket[who_2_send]->disconnect();
                    this->socket[who_2_send] = NULL;
                }
                else    // if we had only one client connected
                {
                    ui->plainTextEdit->appendPlainText(QString("NO 2nd PLAYER TO SEND OTHER-CLIENT-DISCONNECTED SIGNAL"));
                }
            }
            break;
        }
    }
}

bool MainWindow::game_moveCheck_simpleserverside(int row1, int column1, int row2, int column2)
{
    // It checks if the piece isn't moving diagonally.
    if (column1 == column2 || row1 == row2)
    {
        return false;
    }

    // It checks if the piece is moving by more than 1 column and only 1 row
    if ((column2 > column1 + 1 || column2 < column1 - 1) && (row2 == row1 + 1 || row2 == row1 - 1))
    {
        return false;
    }

    // It checks if the piece is leaping.
    if (row2 > row1 + 1 || row2 < row1 - 1)
    {
        // It checks if the piece is leaping too far.
        if (row2 > row1 + 2 || row2 < row1 - 2)
        {
            return false;
        }

        // It checks if the piece isn't moving by exactly 2 columns
        if (column2 != column1 + 2 && column2 != column1 - 2)
        {
            return false;
        }

        // Piece is not leaping too far.
        return true;
    }

    // Piece is not leaping.
    return true;
}



void MainWindow::readAnswer()   /* reads data sent from clients */
{
    int id_sender = -1, id_opponent = -1;
    for(int i = 0; i < 2; i++) {
        if( this->socket[i] == QObject::sender() )
        {
            id_sender = i;

            if(id_sender == 0)
                id_opponent = 1;
            else if (id_sender == 1)
                id_opponent = 0;
        }
    }

    MsgAboutGame msg;
    char buf[sizeof(MsgAboutGame)];
    while(this->socket[id_sender]->read(buf,sizeof(MsgAboutGame)) > 0)
    {
        memcpy(&msg, buf, sizeof(MsgAboutGame));
        switch(msg.happened)
        {
            case MOVE_MAKE:
            {
                if(game_moveCheck_simpleserverside(msg.x_old, msg.y_old, msg.x_new, msg.y_new) == true)
                {
                    //HERE IS LEGAL MOVE DONE
                    this->socket[id_opponent]->write(buf, sizeof(MsgAboutGame));
                    /* here we print info on server's plainTextEdit */
                    if(msg.x_old < 0)
                    {
                        ui->plainTextEdit->appendPlainText(QString("END OF GAME! SOMEONE WON!"));
                    } else {
                        ui->plainTextEdit->appendPlainText(QString("moved from  ") + QString::number(msg.x_old)
                                                           + QString(":") + QString::number(msg.y_old)
                                                           + QString("  =>  ") + QString::number(msg.x_new)
                                                           + QString(":") + QString::number(msg.y_new));
                    }
                    MsgAboutGame msg2;
                    msg2.happened = YOUR_TURN_IS;
                    memcpy(buf, &msg2, sizeof(MsgAboutGame));
                    this->socket[id_opponent]->write(buf, sizeof(MsgAboutGame));
                }
                else
                {
                    //HERE IS NOT LEGAL
                    ui->plainTextEdit->appendPlainText(QString("ILLEGAL MOVE! SENDING SIGNALS TO DISCONNECT TO BOTH: "));
                    // NOT REALLY ELSE_DISCONNECT, BUT "YOU_CHEATING"
                    MsgAboutGame msg2;
                    msg2.happened = ELSE_DISCONNECT;
                    char buffer[sizeof(MsgAboutGame)];
                    memcpy(buffer, &msg2, sizeof(MsgAboutGame));
                    this->socket[id_sender]->write(buffer, sizeof(MsgAboutGame));
                    this->socket[id_sender]->disconnect();
                    this->socket[id_sender] = NULL;
                    // NOW SAME TO SECOND ONE - which was waiting for it's turn
                    MsgAboutGame msg3;
                    msg3.happened = ELSE_DISCONNECT;    // NOT REALLY ELSE_DISCONNECT, BUT "YOU_CHEATING"
                    char buffer3[sizeof(MsgAboutGame)];
                    memcpy(buffer3, &msg3, sizeof(MsgAboutGame));
                    this->socket[id_sender]->write(buffer3, sizeof(MsgAboutGame));
                    this->socket[id_sender]->disconnect();
                    this->socket[id_sender] = NULL;
                }
                /* OLD VERSION WITHOUT SEEING IF THERE IS A CHEAT from sender --- BELOW */
//                this->socket[id_opponent]->write(buf, sizeof(MsgAboutGame));
//                /* here we print info on server's plainTextEdit */
//                if(msg.x_old < 0)
//                {
//                    ui->plainTextEdit->appendPlainText(QString("END OF GAME! SOMEONE WON!"));
//                } else {
//                    ui->plainTextEdit->appendPlainText(QString("moved from  ") + QString::number(msg.x_old)
//                                                       + QString(":") + QString::number(msg.y_old)
//                                                       + QString("  =>  ") + QString::number(msg.x_new)
//                                                       + QString(":") + QString::number(msg.y_new));
//                }
//                MsgAboutGame msg2;
//                msg2.happened = YOUR_TURN_IS;
//                memcpy(buf, &msg2, sizeof(MsgAboutGame));
//                this->socket[id_opponent]->write(buf, sizeof(MsgAboutGame));
            }; break;

            case LOST_GAME:
            {
                this->socket[id_opponent]->write(buf, sizeof(MsgAboutGame));
                /* here we print info on server's plainTextEdit */
                if(msg.x_old < 0)
                {
                    ui->plainTextEdit->appendPlainText(QString("GAME OVER! SENT FROM ")
                                                       + QString::number(id_sender));
                } else {
                    ui->plainTextEdit->appendPlainText(QString("moved from  ") + QString::number(msg.x_old)
                                                       + QString(":") + QString::number(msg.y_old)
                                                       + QString("  =>  ") + QString::number(msg.x_new)
                                                       + QString(":") + QString::number(msg.y_new));
                }
                MsgAboutGame msg2;
                msg2.happened = ELSE_DISCONNECT;
                memcpy(buf, &msg2, sizeof(MsgAboutGame));
                this->socket[id_opponent]->write(buf, sizeof(MsgAboutGame));
            }; break;

            case ELSE_DISCONNECT:
            {
                //nothing
            }; break;

            case FULL_SERVER:
            {
                //nothing
            }; break;

            case CLIENT_CONNECTED:
            {
                //nothing
            }; break;

            case CLIENT_SECOND_CONNECTED:
            {
                //nothing
            }; break;

            case YOUR_TURN_IS:
            {
                //nothing
            }; break;
        }
    }
}
