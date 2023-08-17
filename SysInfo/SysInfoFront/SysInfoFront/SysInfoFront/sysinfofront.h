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
#include <QProcess>
#include <QTextCodec>

#include <string>
#include <vector>
#include <winsock2.h>

#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable:4996)

#include "zlib.h"
#include "nlohmann/json.hpp"
#include "channel.grpc.pb.h"
#include <grpcpp/grpcpp.h>


using std::string;
using std::vector;

using nlohmann::json;

class NumericSortProxyModel : public QSortFilterProxyModel {
public:
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override {
        QVariant leftData = sourceModel()->data(left);
        QVariant rightData = sourceModel()->data(right);

        // ���������, ��� ������ ����� ������������� � �����
        bool ok1, ok2;
        int leftNumber = leftData.toInt(&ok1);
        int rightNumber = rightData.toInt(&ok2);

        if (ok1 && ok2) {
            return leftNumber < rightNumber;
        }

        // � ������, ���� �� ������� ������������� � �����, ��������� ��������� �����
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
    void DisplayDiskInfo();
    void DisplayProcInfo();
    void DisplayNetworkInfo();
    void DisplayDeviceInfo();


    Ui::SysInfoFrontClass ui;
    QProcess *backend_proc;
    QStandardItemModel* model;
    QTimer* proc_timer, * netw_timer;
    NumericSortProxyModel* proxyModel;
    QTextCodec* utf8Codec;


    std::shared_ptr<grpc::Channel> channel;
    std::unique_ptr<channel::ChannelService::Stub> stub_;
};