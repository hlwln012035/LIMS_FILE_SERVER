#include "signalingparsemodule.h"

SignalingParseModule::SignalingParseModule(QObject *parent) : QObject(parent)
{
    log = Log::getInstance();
    udpSocket = new QUdpSocket(this);
    connect(udpSocket,SIGNAL(readyRead()),this,SLOT(processPendingDatagrams()));
}

SignalingParseModule::~SignalingParseModule()
{
    delete udpSocket;
}

void SignalingParseModule::setLocalAddr(QHostAddress local)
{
    localAddr = local;
    udpSocket->bind(local,SERVER_SIGNALING_PORT_UDP,QUdpSocket::ShareAddress|QUdpSocket::ReuseAddressHint);
}

void SignalingParseModule::processPendingDatagrams()
{
    QByteArray datagram;
    do{
        QHostAddress clientAddr;
        datagram.resize(udpSocket->pendingDatagramSize());
        udpSocket->readDatagram(datagram.data(),datagram.size(),&clientAddr);
        QTextCodec *utf8codec = QTextCodec::codecForName("UTF-8");
        QString utf8str = utf8codec->toUnicode(datagram);
        datagram = utf8str.toStdString().data();
        processSignaling(datagram,clientAddr);
    }while(udpSocket->hasPendingDatagrams());
}

void SignalingParseModule::processSignaling(QByteArray signaling, QHostAddress addr)
{
    QJsonParseError jpe;
    QJsonDocument jd = QJsonDocument::fromJson(signaling,&jpe);
    QJsonObject jo;

    if(jpe.error != QJsonParseError::NoError)
    {
        return;
    }
    jo = jd.object();

    QString signalingType = jo.find("SIGNALING_TYPE").value().toString();
    if(signalingType == "PUSH_FILE"){
        detectClient();
    }else if(signalingType == "I_AM_ALIVE"){
        emit clientFound(addr);
    }else if(signalingType == "FILE_RECV_OVER"){
        QString fileName = jo.find("FILE_NAME").value().toString();
        emit tempServerJoin(addr,fileName);
    }else if(signalingType == "FILE_SEND_OVER"){
        emit releaseSendThread(addr);
    }
}

void SignalingParseModule::detectClient()
{
    QJsonObject jo;
    QJsonDocument jd;
    jo.insert("SIGNALING_TYPE","ARE_YOU_OK");
    jo.insert("FILE_NUM",FileManagement::getInstance()->getFileNum());
    QVariant size = FileManagement::getInstance()->getTotalSize();
    jo.insert("FILE_TOTAL_SIZE",size.toString());
    jd.setObject(jo);
    udpSocket->writeDatagram(jd.toJson(),QHostAddress::Broadcast,CLIENT_SIGNALING_PORT_UDP);
}

void SignalingParseModule::sendSignaling(QJsonObject s, QHostAddress dst)
{
    QJsonDocument jd;
    jd.setObject(s);
    udpSocket->writeDatagram(jd.toJson(),dst,CLIENT_SIGNALING_PORT_UDP);
}
