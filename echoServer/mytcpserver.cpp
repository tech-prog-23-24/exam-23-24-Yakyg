#include "mytcpserver.h"
#include <QDebug>
#include <QCoreApplication>

MyTcpServer::~MyTcpServer()
{
    foreach(QTcpSocket* s, clients)
    s->close();
    mTcpServer->close();
}

MyTcpServer::MyTcpServer(QObject *parent) : QObject(parent) {
    mTcpServer = new QTcpServer(this);

    connect(mTcpServer, &QTcpServer::newConnection, this, &MyTcpServer::slotNewConnection);

    if(!mTcpServer->listen(QHostAddress::Any, 65432)){
        qDebug() << "server is not started";
    } else {
        qDebug() << "server is started";
    }
}

void MyTcpServer::slotNewConnection() {
    QTcpSocket* socket = mTcpServer->nextPendingConnection();
    if (clients.size() >= maxClients) {
        qDebug() << "Попытка подключения лишнего клиента, соединение оставлено открытым.";
        socket->write("Сервер занят, попробуйте позже\n");
        connect(socket, &QTcpSocket::readyRead, [socket]() {
            socket->write("Сервер занят, попробуйте позже\n");
        });
        return;
    }

    clients.append(socket);
    clientVotes[socket] = "";

    socket->write("Добро пожаловать! Вы подключены к серверу голосования.\n");
    sendToAll(QString("Сейчас подключено клиентов: %1\n").arg(clients.size()));

    connect(socket, &QTcpSocket::readyRead, this, &MyTcpServer::slotServerRead);
    connect(socket, &QTcpSocket::disconnected, this, &MyTcpServer::slotClientDisconnected);

    // Если все 7 подключились — начинаем голосование
    if (clients.size() == maxClients) {
        startVoting();
    }
}

void MyTcpServer::slotClientDisconnected() {
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    clients.removeAll(socket);
    clientVotes.remove(socket);
    continueVotes.remove(socket);
    sendToAll(QString("Отключился клиент. Сейчас подключено: %1\n").arg(clients.size()));
    socket->deleteLater();
}

void MyTcpServer::slotServerRead() {
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    while(socket->bytesAvailable() > 0) {
        QString msg = QString::fromUtf8(socket->readLine()).trimmed().toUpper();
        qDebug() << "Получено от клиента:" << msg;

        if (waitingForContinue) {
            handleContinueVote(socket, msg);
            continue;
        }

        if ((msg == "A" || msg == "B") && clientVotes[socket].isEmpty()) {
            clientVotes[socket] = msg;
            if (msg == "A") voteYes++;
            else if (msg == "B") voteNo++;
            socket->write(QString("Ваш голос '%1' принят!\n").arg(msg).toUtf8());

            // Проверяем: все ли проголосовали?
            bool allVoted = true;
            for (auto v : clientVotes) {
                if (v.isEmpty()) {
                    allVoted = false;
                    break;
                }
            }
            if (allVoted) {
                finishVoting();
            }
        } else if (!clientVotes[socket].isEmpty() && !waitingForContinue) {
            socket->write("Вы уже голосовали!\n");
        } else if (!waitingForContinue) {
            socket->write("Пожалуйста, введите 'A'  или 'B'  для голосования.\n");
        }
    }
}

void MyTcpServer::sendToAll(const QString& msg) {
    for (QTcpSocket* s : clients) {
        s->write(msg.toUtf8());
    }
}

void MyTcpServer::startVoting() {
    voteYes = 0;
    voteNo = 0;
    waitingForContinue = false;
    continueVotes.clear();
    for (auto& v : clientVotes)
        v = "";
    sendToAll("Проголосуйте: A или B\n");
}

void MyTcpServer::finishVoting() {
    QString result = QString("Голосование завершено!\nЗа A: %1\nЗа B: %2\n").arg(voteYes).arg(voteNo);
    sendToAll(result);

    askContinue();
}

void MyTcpServer::askContinue() {
    waitingForContinue = true;
    continueVotes.clear();
    sendToAll("Продолжить голосование? (Y/N)\n");
}

void MyTcpServer::handleContinueVote(QTcpSocket* socket, const QString& msg) {
    if (continueVotes.contains(socket)) {
        socket->write("Вы уже ответили на вопрос о продолжении.\n");
        return;
    }
    if (msg != "Y" && msg != "N") {
        socket->write("Пожалуйста, ответьте 'Y' (да) или 'N' (нет)\n");
        return;
    }
    continueVotes[socket] = msg;

    // Как только кто-то пишет "N" — всех отключаем!
    if (msg == "N") {
        sendToAll("Голосование завершено. Сервер завершает работу.\n");
        for (QTcpSocket* s : clients) {
            s->disconnectFromHost();
        }
        clients.clear();
        clientVotes.clear();
        continueVotes.clear();
        waitingForContinue = false;
        return;
    }
    // Если все ответили — если никто не против, запускаем новое голосование
    if (continueVotes.size() == clients.size()) {
        bool allYes = true;
        for (const auto& v : continueVotes) {
            if (v != "Y") {
                allYes = false;
                break;
            }
        }
        if (allYes) {
            sendToAll("Начинаем новое голосование!\n");
            startVoting();
        }
    }
}
