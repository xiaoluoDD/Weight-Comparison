#include "tcpclient.h"
#include <QDebug>

TcpClient::TcpClient(QObject *parent)
    : QObject(parent)
    , m_socket(nullptr)
    , m_serverPort(0)
{
    setupSocket();
}

TcpClient::~TcpClient()
{
    if (m_socket) {
        disconnectFromServer();
        m_socket->deleteLater();
    }
}

void TcpClient::setupSocket()
{
    m_socket = new QTcpSocket(this);
    
    // 连接信号和槽
    connect(m_socket, &QTcpSocket::connected, this, &TcpClient::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &TcpClient::onDisconnected);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
            this, &TcpClient::onError);
    connect(m_socket, &QTcpSocket::readyRead, this, &TcpClient::onReadyRead);
    connect(m_socket, &QTcpSocket::stateChanged, this, &TcpClient::onStateChanged);
}

void TcpClient::connectToServer(const QString &address, quint16 port)
{
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        emit errorOccurred("已经连接到服务器，请先断开连接");
        return;
    }
    
    m_serverAddress = QHostAddress(address);
    m_serverPort = port;
    
    m_socket->connectToHost(m_serverAddress, m_serverPort);
}

void TcpClient::disconnectFromServer()
{
    if (m_socket && m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->disconnectFromHost();
        if (m_socket->state() == QAbstractSocket::ConnectedState) {
            m_socket->waitForDisconnected(3000);
        }
    }
}

void TcpClient::sendData(const QByteArray &data)
{
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState) {
        emit errorOccurred("未连接到服务器，无法发送数据");
        return;
    }
    
    qint64 bytesWritten = m_socket->write(data);
    if (bytesWritten == -1) {
        emit errorOccurred(QString("发送数据失败: %1").arg(m_socket->errorString()));
    } else if (bytesWritten < data.size()) {
        emit errorOccurred("数据未完全发送");
    }
}

bool TcpClient::isConnected() const
{
    return m_socket && m_socket->state() == QAbstractSocket::ConnectedState;
}

QHostAddress TcpClient::serverAddress() const
{
    return m_serverAddress;
}

quint16 TcpClient::serverPort() const
{
    return m_serverPort;
}

void TcpClient::onConnected()
{
    emit connected();
}

void TcpClient::onDisconnected()
{
    emit disconnected();
}

void TcpClient::onError(QAbstractSocket::SocketError error)
{
    QString errorString;
    switch (error) {
    case QAbstractSocket::ConnectionRefusedError:
        errorString = "连接被拒绝";
        break;
    case QAbstractSocket::RemoteHostClosedError:
        errorString = "远程主机关闭了连接";
        break;
    case QAbstractSocket::HostNotFoundError:
        errorString = "找不到主机";
        break;
    case QAbstractSocket::NetworkError:
        errorString = "网络错误";
        break;
    // Qt6中已移除TimeoutError，超时错误会在default分支中通过errorString()处理
    default:
        // 使用errorString()获取详细错误信息，包括超时等错误
        errorString = QString("Socket错误: %1").arg(m_socket->errorString());
        break;
    }
    
    emit errorOccurred(errorString);
}

void TcpClient::onReadyRead()
{
    QByteArray data = m_socket->readAll();
    if (!data.isEmpty()) {
        emit dataReceived(data);
    }
}

void TcpClient::onStateChanged(QAbstractSocket::SocketState state)
{
    // 可以在这里添加状态变化的日志
    Q_UNUSED(state)
}
