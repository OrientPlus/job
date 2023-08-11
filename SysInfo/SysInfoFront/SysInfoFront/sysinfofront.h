#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_sysinfofront.h"

#include <QtWidgets/QMainWindow>
#include <QApplication>
#include <QWidget>
#include <QPushButton>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QSignalMapper>
#include <QMenu>
#include <QStandardItemModel>
#include <QTableView>
#include <QTableWidget>
#include <QStringList>
#include <QDebug>
#include <QStyledItemDelegate>
#include <QLineEdit>
#include <QIntValidator>
#include <QSortFilterProxyModel>
#include <QTimer>

#include <string>
#include <vector>
#include <winsock2.h>

#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable:4996)

#include "zlib.h"
#include "nlohmann/json.hpp"

#define DEFAULT_PORT 8888
#define DEFAULT_IP "127.0.0.1"
#define BUFFER_SIZE 512

using std::string;
using std::vector;

using nlohmann::json;

class NumericSortProxyModel : public QSortFilterProxyModel {
public:
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override {
        QVariant leftData = sourceModel()->data(left);
        QVariant rightData = sourceModel()->data(right);

        // Проверяем, что данные можно преобразовать в числа
        bool ok1, ok2;
        int leftNumber = leftData.toInt(&ok1);
        int rightNumber = rightData.toInt(&ok2);

        if (ok1 && ok2) {
            return leftNumber < rightNumber;
        }

        // В случае, если не удалось преобразовать в числа, выполняем сравнение строк
        return QString::localeAwareCompare(leftData.toString(), rightData.toString()) < 0;
    }
};

class SysInfoFront : public QMainWindow
{
    Q_OBJECT

public:
    SysInfoFront(QWidget* parent = nullptr);
    ~SysInfoFront();

private:
    int OpenSocket();
    int CloseSocket();
    int SendData(string data);
    int RecvData(string& data);
    unsigned GetCRC32(string data);

    void DisplayDiskInfo();
    void DisplayProcInfo();
    void DisplayNetworkInfo();
    void DisplayDeviceInfo();


    Ui::SysInfoFrontClass ui;
    QStandardItemModel* model;
    QTimer* proc_timer, *netw_timer;
    NumericSortProxyModel* proxyModel;

    sockaddr_in server_address_, addr_;
    WSADATA wsa_data_;
    SOCKET socket_;
};