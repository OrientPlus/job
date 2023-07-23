#include <iostream>
#include <string>
#include <amqpcpp.h>
#include <amqpcpp/libboostasio.h>

class InetConnect {
public:
    InetConnect(const std::string& serverIp, int serverPort)
        : serverIp_(serverIp), serverPort_(serverPort), socket_(ioService_) {
        // ��������� ���������� � RabbitMQ ��������
        connection_.connect();
    }

    // ����� ��� �������� ������ ������������� ������� ����� RabbitMQ
    void SendData(const std::string& data, const std::string& exchange, const std::string& routingKey) {
        // �������� ������ ����� RabbitMQ
        connection_.channel()->publish(exchange, routingKey, data);
    }

    // ����� ��� ��������� ������ ������������� ������� ����� RabbitMQ
    std::string RecvData(const std::string& exchange, const std::string& routingKey) {
        std::string receivedData;
        // ��������� �������� ��������� �� RabbitMQ
        connection_.consume(exchange, routingKey, [&](const AMQP::Message& message, uint64_t deliveryTag, bool redelivered) {
            receivedData = std::string(message.body(), message.bodySize());
            });
        ioService_.poll();
        return receivedData;
    }

private:
    std::string serverIp_;
    int serverPort_;
    boost::asio::io_service ioService_;
    AMQP::LibBoostAsioHandler handler_{ ioService_ };
    AMQP::TcpConnection connection_{ &handler_, AMQP::Address(serverIp_, serverPort_) };
};