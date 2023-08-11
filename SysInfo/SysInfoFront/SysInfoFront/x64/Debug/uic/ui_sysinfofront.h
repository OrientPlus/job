/********************************************************************************
** Form generated from reading UI file 'sysinfofront.ui'
**
** Created by: Qt User Interface Compiler version 5.14.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SYSINFOFRONT_H
#define UI_SYSINFOFRONT_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTableView>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_SysInfoFrontClass
{
public:
    QWidget *centralWidget;
    QPushButton *device_button;
    QPushButton *netw_button;
    QPushButton *disk_button;
    QPushButton *proc_button;
    QTableView *view_table;
    QMenuBar *menuBar;
    QToolBar *mainToolBar;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *SysInfoFrontClass)
    {
        if (SysInfoFrontClass->objectName().isEmpty())
            SysInfoFrontClass->setObjectName(QString::fromUtf8("SysInfoFrontClass"));
        SysInfoFrontClass->resize(1082, 463);
        centralWidget = new QWidget(SysInfoFrontClass);
        centralWidget->setObjectName(QString::fromUtf8("centralWidget"));
        device_button = new QPushButton(centralWidget);
        device_button->setObjectName(QString::fromUtf8("device_button"));
        device_button->setGeometry(QRect(10, 250, 101, 71));
        netw_button = new QPushButton(centralWidget);
        netw_button->setObjectName(QString::fromUtf8("netw_button"));
        netw_button->setGeometry(QRect(10, 170, 101, 71));
        disk_button = new QPushButton(centralWidget);
        disk_button->setObjectName(QString::fromUtf8("disk_button"));
        disk_button->setGeometry(QRect(10, 90, 101, 71));
        proc_button = new QPushButton(centralWidget);
        proc_button->setObjectName(QString::fromUtf8("proc_button"));
        proc_button->setGeometry(QRect(10, 10, 101, 71));
        view_table = new QTableView(centralWidget);
        view_table->setObjectName(QString::fromUtf8("view_table"));
        view_table->setGeometry(QRect(120, 10, 951, 381));
        SysInfoFrontClass->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(SysInfoFrontClass);
        menuBar->setObjectName(QString::fromUtf8("menuBar"));
        menuBar->setGeometry(QRect(0, 0, 1082, 26));
        SysInfoFrontClass->setMenuBar(menuBar);
        mainToolBar = new QToolBar(SysInfoFrontClass);
        mainToolBar->setObjectName(QString::fromUtf8("mainToolBar"));
        SysInfoFrontClass->addToolBar(Qt::TopToolBarArea, mainToolBar);
        statusBar = new QStatusBar(SysInfoFrontClass);
        statusBar->setObjectName(QString::fromUtf8("statusBar"));
        SysInfoFrontClass->setStatusBar(statusBar);

        retranslateUi(SysInfoFrontClass);

        QMetaObject::connectSlotsByName(SysInfoFrontClass);
    } // setupUi

    void retranslateUi(QMainWindow *SysInfoFrontClass)
    {
        SysInfoFrontClass->setWindowTitle(QCoreApplication::translate("SysInfoFrontClass", "SysInfoFront", nullptr));
        device_button->setText(QCoreApplication::translate("SysInfoFrontClass", "Devices", nullptr));
        netw_button->setText(QCoreApplication::translate("SysInfoFrontClass", "Network", nullptr));
        disk_button->setText(QCoreApplication::translate("SysInfoFrontClass", "Disks", nullptr));
        proc_button->setText(QCoreApplication::translate("SysInfoFrontClass", "Processes", nullptr));
    } // retranslateUi

};

namespace Ui {
    class SysInfoFrontClass: public Ui_SysInfoFrontClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SYSINFOFRONT_H
