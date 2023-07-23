#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <sstream>

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
#include <QComboBox>
#include <QTextEdit>
#include <QPoint>
#include <QSignalMapper>
#include <QMenu>
#include <QCloseEvent>

#include "ui_mainwindow.h"
#include "cryptographer.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Crypt32.lib")
#pragma comment (lib, "advapi32")

#define BUFFER_SIZE 8192

enum Command { kCreateNew, kDelete, kWrite, kRead, kSaveAll, kLoadAll, kGetActualNoteList, kGetNoteTypeInfo, kChangeType, kLogout };
enum NoteType { kShared = 0, kEncrypted, kSpecialEncrypted };

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    friend class NoteWindow;
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    void run();

private:
    int Message(string msg);
    int StartClient();
    int StopClient();
    int SendData(string data);
    string RecvData();

    bool Authorization();

    void UpdateNoteList();

    NoteType GetNoteTypeInfo(string note_name);

    Ui::MainWindowClass ui;
    Cryptographer crypt_;
    SOCKET socket_;
    string port_, ip_address_, public_key_,
        login_, password_,
        error_message_;

    QWidget* window_;
    QLineEdit* line_edit1_, *line_edit2_;
    QFormLayout* layout_;
    QPushButton* button_;
    QTextEdit* text_edit_;
    QComboBox* combo_box_;
    QLabel* label_;

public slots:
    void LoadAllNotes();
    void SaveAllNotes();
    void CreateNewNote();
    void DeleteNote(string note_name);
    void ChangeType(string note_name);
    void OpenNote(QListWidgetItem* item);
    void ShowNoteMenu(const QPoint& pos);
    void Logout();
};

