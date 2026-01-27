#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QHostAddress>

class TcpClient : public QObject
{
    Q_OBJECT

public:
    explicit TcpClient(QObject *parent = nullptr);
    ~TcpClient();
    
    // 连接服务器
    void connectToServer(const QString &address, quint16 port);
    
    // 断开连接
    void disconnectFromServer();
    
    // 发送数据
    void sendData(const QByteArray &data);
    
    // 获取连接状态
    bool isConnected() const;
    
    // 获取服务器地址和端口
    QHostAddress serverAddress() const;
    quint16 serverPort() const;

signals:
    // 连接成功
    void connected();
    
    // 断开连接
    void disconnected();
    
    // 错误发生
    void errorOccurred(const QString &error);
    
    // 数据接收
    void dataReceived(const QByteArray &data);

private slots:
    // TCP Socket槽函数
    void onConnected();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError error);
    void onReadyRead();
    void onStateChanged(QAbstractSocket::SocketState state);

private:
    QTcpSocket *m_socket;
    QHostAddress m_serverAddress;
    quint16 m_serverPort;
    
    // 初始化Socket
    void setupSocket();
};

#endif // TCPCLIENT_H
