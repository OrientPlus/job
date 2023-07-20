/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.14.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindowClass
{
public:
    QAction *actionCreate_new_note;
    QWidget *centralWidget;
    QPushButton *LoadAllNotes;
    QPushButton *SaveAllNotes;
    QListWidget *NotesList;
    QMenuBar *menuBar;
    QMenu *menuMenu;
    QToolBar *mainToolBar;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *MainWindowClass)
    {
        if (MainWindowClass->objectName().isEmpty())
            MainWindowClass->setObjectName(QString::fromUtf8("MainWindowClass"));
        MainWindowClass->resize(285, 329);
        actionCreate_new_note = new QAction(MainWindowClass);
        actionCreate_new_note->setObjectName(QString::fromUtf8("actionCreate_new_note"));
        centralWidget = new QWidget(MainWindowClass);
        centralWidget->setObjectName(QString::fromUtf8("centralWidget"));
        LoadAllNotes = new QPushButton(centralWidget);
        LoadAllNotes->setObjectName(QString::fromUtf8("LoadAllNotes"));
        LoadAllNotes->setGeometry(QRect(10, 250, 75, 23));
        SaveAllNotes = new QPushButton(centralWidget);
        SaveAllNotes->setObjectName(QString::fromUtf8("SaveAllNotes"));
        SaveAllNotes->setGeometry(QRect(110, 250, 75, 23));
        NotesList = new QListWidget(centralWidget);
        NotesList->setObjectName(QString::fromUtf8("NotesList"));
        NotesList->setGeometry(QRect(10, 10, 261, 231));
        MainWindowClass->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(MainWindowClass);
        menuBar->setObjectName(QString::fromUtf8("menuBar"));
        menuBar->setGeometry(QRect(0, 0, 285, 21));
        menuMenu = new QMenu(menuBar);
        menuMenu->setObjectName(QString::fromUtf8("menuMenu"));
        MainWindowClass->setMenuBar(menuBar);
        mainToolBar = new QToolBar(MainWindowClass);
        mainToolBar->setObjectName(QString::fromUtf8("mainToolBar"));
        MainWindowClass->addToolBar(Qt::TopToolBarArea, mainToolBar);
        statusBar = new QStatusBar(MainWindowClass);
        statusBar->setObjectName(QString::fromUtf8("statusBar"));
        MainWindowClass->setStatusBar(statusBar);

        menuBar->addAction(menuMenu->menuAction());
        menuMenu->addAction(actionCreate_new_note);

        retranslateUi(MainWindowClass);

        QMetaObject::connectSlotsByName(MainWindowClass);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindowClass)
    {
        MainWindowClass->setWindowTitle(QCoreApplication::translate("MainWindowClass", "MainWindow", nullptr));
        actionCreate_new_note->setText(QCoreApplication::translate("MainWindowClass", "Create new note", nullptr));
        LoadAllNotes->setText(QCoreApplication::translate("MainWindowClass", "Load all notes", nullptr));
        SaveAllNotes->setText(QCoreApplication::translate("MainWindowClass", "Save all notes", nullptr));
        menuMenu->setTitle(QCoreApplication::translate("MainWindowClass", "Menu", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindowClass: public Ui_MainWindowClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
