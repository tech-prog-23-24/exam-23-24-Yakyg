#ifndef MYTCPSERVER_H
#define MYTCPSERVER_H
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QList>
#include <QMap>
#include <QString>

class MyTcpServer : public QObject
{
    Q_OBJECT
public:
    explicit MyTcpServer(QObject *parent = nullptr);
    ~MyTcpServer();

public slots:
    void slotNewConnection();
    void slotClientDisconnected();
    void slotServerRead();

private:
    QTcpServer *mTcpServer;
    QList<QTcpSocket*> clients;
    QMap<QTcpSocket*, QString> clientVotes;
    int maxClients = 7;

    QString votingQuestion = "Голосование: Вы за продолжение? (A - да, B - нет)";
    int voteYes = 0;
    int voteNo = 0;
    void sendToAll(const QString& msg);
    void startVoting();
    void finishVoting();

    bool waitingForContinue = false;
    QMap<QTcpSocket*, QString> continueVotes; // Y/N
    void askContinue();
    void handleContinueVote(QTcpSocket* socket, const QString& msg);
};

#endif // MYTCPSERVER_H
