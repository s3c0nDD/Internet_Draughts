#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow)
{
    ui->setupUi(this);
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
    for (int i = 0; i < 3; i++) {
        if (i < 2 && (socket[i] == NULL)) {
            this->socket[i] = this->server->nextPendingConnection();
            connect(this->socket[i], SIGNAL(readyRead()), this, SLOT(read()));
            connect(this->socket[i], SIGNAL(disconnected()), this, SLOT(onDisconnected()));
            break;
        }
        else if (i == 2) {
            MsgAboutGame message;
            message.happened = FULL;
            char buf[sizeof(message)];
            memcpy(buf, &message, sizeof(message));
            QTcpSocket *tmp = this->server->nextPendingConnection();
            tmp->write(buf);
            tmp->disconnectFromHost();
        }
    }
    /*if(this->socket[0] == NULL)
    {
        this->socket[0] = this->server->nextPendingConnection();
    }
    else
    {
        //this->socket->disconnectFromHost();
        this->socket[1] = this->server->nextPendingConnection();
    }
    */
    //connect(this->socket, SIGNAL(readyRead()), this, SLOT(Read()));
    //connect(this->socket, SIGNAL(disconnected()), this, SLOT(onDisconnected()));
}

void MainWindow::onDisconnected()
{
    for(int i = 0; i < 2; i++) {
        if( this->socket[i] == QObject::sender() ) {
            this->socket[i] = NULL;
            break;
        }

    }
}

void MainWindow::read()
{
    char message[3];
    //this->socket->readLine(message, 3);


    QMessageBox Box;

    Box.setText(message);

    Box.exec();
    //this->socket = this->server->nextPendingConnection();

    //TODO operacje na grze

    //this->socket->write(message, strlen(message));
}
