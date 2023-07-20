#pragma once

#include <iostream>
#include <string>
#include <vector>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <wincrypt.h>
#include <windows.h>

#include <QtWidgets/QMainWindow>
#include <QApplication>
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QFormLayout>
#include <QDialog>
#include <QLabel>
#include <QVBoxLayout>

#include "ui_mainwindow.h"
#include "cryptographer.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Crypt32.lib")
#pragma comment (lib, "advapi32")

#define RECV_BUFFER_SIZE 4096

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    //MainWindow(string ip_address_ = "localhost", string port_ = "8888", QWidget* parent = nullptr) {};
    MainWindow(string ip_address, string port, QWidget* parent = nullptr) : port_(port), ip_address_(ip_address) {};
    ~MainWindow();

    void run();

private:
    int Message(string msg);
    int StartClient();
    int StopClient();
    int SendData(string data);
    string RecvData();

    bool Authorization();

    Ui::MainWindowClass ui;
    Cryptographer crypt_;
    SOCKET socket_;
    string port_, ip_address_, public_key_,
        login_, password_,
        error_message_;

public slots:
    void LoadAllNotes();
    void SaveAllNotes();
};

