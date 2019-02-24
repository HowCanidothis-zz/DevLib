#include "networkpackage.h"

#include <QTcpSocket>

#include "networkconnection.h"

bool NetworkPackage::CheckSum() const
{
    return m_header.Hashsum == GenerateCheckSum();
}

void NetworkPackage::Pack(const QByteArray& data)
{
    m_data = data;
    pack();
}

qint32 NetworkPackage::GenerateCheckSum() const
{
    qint32 hashSum = 0;
    for(const char byte: m_data) {
        hashSum ^= byte;
    }
    return hashSum;
}

void NetworkPackage::write(NetworkConnection* connection) const
{
    connection->m_socket.write((const char*)this, sizeof(NetworkPackageHeader));
    connection->m_socket.write(m_data.data(), m_data.size());
}

void NetworkPackage::pack()
{
    m_header.Size = m_data.size();
    m_header.Hashsum = GenerateCheckSum();
}

