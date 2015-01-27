#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QGridLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QLineEdit>
#include <string>

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
                controlsLayout->addWidget(xy[i][j], i, j);
                connect(xy[i][j], SIGNAL(clicked()), this, SLOT(onPushButtonGameClicked()));
                temp++;
            }
        }
    }

    addressLineEdit = new QLineEdit;
    addressLineEdit->resize(GAME_SIZE * BUTTON_SIZE, BUTTON_SIZE);
    addressLineEdit->move(0, GAME_SIZE * BUTTON_SIZE);

    buttonConnect = new QPushButton;
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
    //delete* xy;   // <= this makes programme crash
                    // ... so how to repair it ???
    delete addressLineEdit;
    delete buttonConnect;
    delete ui;
}


void MainWindow::onPushButtonGameClicked()
{
//    ??? message2server[]; // this is what to be sent to server
//
//    for(int i = 0; i < GAME_SIZE; i++)
//      {
//        for(int j = 0; j < GAME_SIZE; j++)
//          {
//            if(xy[i][j]==QObject::sender())
//              {
//                xy[i][j]->setText(/*checker*/);//set letter[wWbB] on button
//                message[0] = i+65;       // cause there are f.e. 8x8 places
//                message[1] = j+65;       // and we need to "know" who is who
//                message[2] = 'X';        // so modified and "sent"2serv is this
//               }
//            //button[i][j]->setEnabled(false); // disable the button for a while..
//        }
//    }
//    this->socket->write(message, strlen(message));  // write to socket
//    //enableDisable(false); // block the game for a while....
}


void MainWindow::onPushButtonConnectClicked()
{
    this->socket = new QTcpSocket;

    connect(this->socket, SIGNAL(connected()), this, SLOT(onConnect()));
    connect(this->socket, SIGNAL(disconnected()), this, SLOT(onDisconnect()));
    connect(this->socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(socketError()));
    connect(this->socket, SIGNAL(readyRead()), this, SLOT(onReadyRead()));

    this->socket->connectToHost(this->addressLineEdit->text(), (qint16)GAME_PORT);
}


void MainWindow::onConnect()
{
    this->addressLineEdit->setEnabled(false);   // block address line
    this->buttonConnect->setEnabled(false);     // block connect button
    //disconect(........)
    enableDisabledGame(true);                   // enable the game
}


void MainWindow::onDisconnect()
{
    this->addressLineEdit->setEnabled(false);   // block address line
    this->buttonConnect->setEnabled(false);     // block connect button
    //disconect(........)
    enableDisabledGame(false);                  // disable the game
    this->socket = NULL;
}


void MainWindow::socketError()
{
    QMessageBox Box;

    Box.setText("Error while trying to connect: \n" + socket->errorString());
    Box.exec();

    delete this->socket;
    this->buttonConnect->setEnabled(true);
}


void MainWindow::readMessage()
{
    MsgToServer message;
    char buf[sizeof(message)];
    while(this->socket->read(buf, sizeof(message)) > 0)
    {
        memcpy(&message, buf, sizeof(message));
    }

    //TODO - message readed - here processing....
    /*switch(message.type)
    {
        case FULL:
        {
            // f.e. show QMessageBox that 'server is full'
        }
    } */
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
