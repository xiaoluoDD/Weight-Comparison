/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.7.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QVBoxLayout *verticalLayout;
    QTabWidget *tabWidget;
    QWidget *settingsTab;
    QVBoxLayout *verticalLayout_settings;
    QGroupBox *connectionGroup;
    QFormLayout *formLayout_connection;
    QLabel *label_serverAddress;
    QLineEdit *serverAddressEdit;
    QLabel *label_serverPort;
    QLineEdit *serverPortEdit;
    QLabel *label_operation;
    QHBoxLayout *horizontalLayout_buttons;
    QPushButton *connectBtn;
    QPushButton *disconnectBtn;
    QSpacerItem *horizontalSpacer_buttons;
    QLabel *label_status;
    QLabel *connectionStatusLabel;
    QSpacerItem *verticalSpacer_settings;
    QWidget *bindingTab;
    QVBoxLayout *verticalLayout_binding;
    QGroupBox *inputGroup;
    QFormLayout *formLayout_binding;
    QLabel *label_command;
    QLineEdit *commandEdit;
    QLabel *label_itemName;
    QLineEdit *itemNameEdit;
    QLabel *label_bindingOperation;
    QHBoxLayout *horizontalLayout_bindingButtons;
    QPushButton *addBindingBtn;
    QPushButton *removeBindingBtn;
    QSpacerItem *horizontalSpacer_binding;
    QGroupBox *listGroup;
    QVBoxLayout *verticalLayout_list;
    QListWidget *bindingListWidget;
    QWidget *weightDataTab;
    QGridLayout *gridLayout_weightData;
    QGroupBox *leftTopGroup;
    QGridLayout *gridLayout_leftTop;
    QLabel *slot1_5;
    QLabel *slot1_6;
    QLabel *slot1_7;
    QLabel *slot1_8;
    QLabel *slot1_1;
    QLabel *slot1_2;
    QLabel *slot1_3;
    QLabel *slot1_4;
    QGroupBox *rightGroup;
    QVBoxLayout *verticalLayout_right;
    QHBoxLayout *horizontalLayout_ngButtons;
    QPushButton *productionSupplementBtn;
    QPushButton *ngDeleteBtn;
    QPushButton *ngUseBtn;
    QSpacerItem *horizontalSpacer_ng;
    QTableWidget *ngTable;
    QGroupBox *leftBottomGroup;
    QGridLayout *gridLayout_leftBottom;
    QLabel *slot2_5;
    QLabel *slot2_6;
    QLabel *slot2_7;
    QLabel *slot2_8;
    QLabel *slot2_1;
    QLabel *slot2_2;
    QLabel *slot2_3;
    QLabel *slot2_4;
    QWidget *historyTab;
    QVBoxLayout *verticalLayout_history;
    QGroupBox *historyLeftGroup;
    QVBoxLayout *verticalLayout_historyLeft;
    QTableWidget *weightTable1;
    QHBoxLayout *horizontalLayout_table1;
    QPushButton *clearTable1Btn;
    QPushButton *exportTable1Btn;
    QSpacerItem *horizontalSpacer_table1;
    QGroupBox *historyRightGroup;
    QVBoxLayout *verticalLayout_historyRight;
    QTableWidget *weightTable2;
    QHBoxLayout *horizontalLayout_table2;
    QPushButton *clearTable2Btn;
    QPushButton *exportTable2Btn;
    QSpacerItem *horizontalSpacer_table2;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(1000, 700);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        verticalLayout = new QVBoxLayout(centralwidget);
        verticalLayout->setObjectName("verticalLayout");
        tabWidget = new QTabWidget(centralwidget);
        tabWidget->setObjectName("tabWidget");
        settingsTab = new QWidget();
        settingsTab->setObjectName("settingsTab");
        verticalLayout_settings = new QVBoxLayout(settingsTab);
        verticalLayout_settings->setObjectName("verticalLayout_settings");
        connectionGroup = new QGroupBox(settingsTab);
        connectionGroup->setObjectName("connectionGroup");
        formLayout_connection = new QFormLayout(connectionGroup);
        formLayout_connection->setObjectName("formLayout_connection");
        label_serverAddress = new QLabel(connectionGroup);
        label_serverAddress->setObjectName("label_serverAddress");

        formLayout_connection->setWidget(0, QFormLayout::LabelRole, label_serverAddress);

        serverAddressEdit = new QLineEdit(connectionGroup);
        serverAddressEdit->setObjectName("serverAddressEdit");

        formLayout_connection->setWidget(0, QFormLayout::FieldRole, serverAddressEdit);

        label_serverPort = new QLabel(connectionGroup);
        label_serverPort->setObjectName("label_serverPort");

        formLayout_connection->setWidget(1, QFormLayout::LabelRole, label_serverPort);

        serverPortEdit = new QLineEdit(connectionGroup);
        serverPortEdit->setObjectName("serverPortEdit");

        formLayout_connection->setWidget(1, QFormLayout::FieldRole, serverPortEdit);

        label_operation = new QLabel(connectionGroup);
        label_operation->setObjectName("label_operation");

        formLayout_connection->setWidget(2, QFormLayout::LabelRole, label_operation);

        horizontalLayout_buttons = new QHBoxLayout();
        horizontalLayout_buttons->setObjectName("horizontalLayout_buttons");
        connectBtn = new QPushButton(connectionGroup);
        connectBtn->setObjectName("connectBtn");

        horizontalLayout_buttons->addWidget(connectBtn);

        disconnectBtn = new QPushButton(connectionGroup);
        disconnectBtn->setObjectName("disconnectBtn");
        disconnectBtn->setEnabled(false);

        horizontalLayout_buttons->addWidget(disconnectBtn);

        horizontalSpacer_buttons = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_buttons->addItem(horizontalSpacer_buttons);


        formLayout_connection->setLayout(2, QFormLayout::FieldRole, horizontalLayout_buttons);

        label_status = new QLabel(connectionGroup);
        label_status->setObjectName("label_status");

        formLayout_connection->setWidget(3, QFormLayout::LabelRole, label_status);

        connectionStatusLabel = new QLabel(connectionGroup);
        connectionStatusLabel->setObjectName("connectionStatusLabel");

        formLayout_connection->setWidget(3, QFormLayout::FieldRole, connectionStatusLabel);


        verticalLayout_settings->addWidget(connectionGroup);

        verticalSpacer_settings = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout_settings->addItem(verticalSpacer_settings);

        tabWidget->addTab(settingsTab, QString());
        bindingTab = new QWidget();
        bindingTab->setObjectName("bindingTab");
        verticalLayout_binding = new QVBoxLayout(bindingTab);
        verticalLayout_binding->setObjectName("verticalLayout_binding");
        inputGroup = new QGroupBox(bindingTab);
        inputGroup->setObjectName("inputGroup");
        formLayout_binding = new QFormLayout(inputGroup);
        formLayout_binding->setObjectName("formLayout_binding");
        label_command = new QLabel(inputGroup);
        label_command->setObjectName("label_command");

        formLayout_binding->setWidget(0, QFormLayout::LabelRole, label_command);

        commandEdit = new QLineEdit(inputGroup);
        commandEdit->setObjectName("commandEdit");

        formLayout_binding->setWidget(0, QFormLayout::FieldRole, commandEdit);

        label_itemName = new QLabel(inputGroup);
        label_itemName->setObjectName("label_itemName");

        formLayout_binding->setWidget(1, QFormLayout::LabelRole, label_itemName);

        itemNameEdit = new QLineEdit(inputGroup);
        itemNameEdit->setObjectName("itemNameEdit");

        formLayout_binding->setWidget(1, QFormLayout::FieldRole, itemNameEdit);

        label_bindingOperation = new QLabel(inputGroup);
        label_bindingOperation->setObjectName("label_bindingOperation");

        formLayout_binding->setWidget(2, QFormLayout::LabelRole, label_bindingOperation);

        horizontalLayout_bindingButtons = new QHBoxLayout();
        horizontalLayout_bindingButtons->setObjectName("horizontalLayout_bindingButtons");
        addBindingBtn = new QPushButton(inputGroup);
        addBindingBtn->setObjectName("addBindingBtn");

        horizontalLayout_bindingButtons->addWidget(addBindingBtn);

        removeBindingBtn = new QPushButton(inputGroup);
        removeBindingBtn->setObjectName("removeBindingBtn");

        horizontalLayout_bindingButtons->addWidget(removeBindingBtn);

        horizontalSpacer_binding = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_bindingButtons->addItem(horizontalSpacer_binding);


        formLayout_binding->setLayout(2, QFormLayout::FieldRole, horizontalLayout_bindingButtons);


        verticalLayout_binding->addWidget(inputGroup);

        listGroup = new QGroupBox(bindingTab);
        listGroup->setObjectName("listGroup");
        verticalLayout_list = new QVBoxLayout(listGroup);
        verticalLayout_list->setObjectName("verticalLayout_list");
        bindingListWidget = new QListWidget(listGroup);
        bindingListWidget->setObjectName("bindingListWidget");

        verticalLayout_list->addWidget(bindingListWidget);


        verticalLayout_binding->addWidget(listGroup);

        tabWidget->addTab(bindingTab, QString());
        weightDataTab = new QWidget();
        weightDataTab->setObjectName("weightDataTab");
        gridLayout_weightData = new QGridLayout(weightDataTab);
        gridLayout_weightData->setObjectName("gridLayout_weightData");
        leftTopGroup = new QGroupBox(weightDataTab);
        leftTopGroup->setObjectName("leftTopGroup");
        gridLayout_leftTop = new QGridLayout(leftTopGroup);
        gridLayout_leftTop->setObjectName("gridLayout_leftTop");
        slot1_5 = new QLabel(leftTopGroup);
        slot1_5->setObjectName("slot1_5");
        slot1_5->setFrameShape(QFrame::Box);
        slot1_5->setFrameShadow(QFrame::Raised);

        gridLayout_leftTop->addWidget(slot1_5, 0, 0, 1, 1);

        slot1_6 = new QLabel(leftTopGroup);
        slot1_6->setObjectName("slot1_6");
        slot1_6->setFrameShape(QFrame::Box);
        slot1_6->setFrameShadow(QFrame::Raised);

        gridLayout_leftTop->addWidget(slot1_6, 1, 0, 1, 1);

        slot1_7 = new QLabel(leftTopGroup);
        slot1_7->setObjectName("slot1_7");
        slot1_7->setFrameShape(QFrame::Box);
        slot1_7->setFrameShadow(QFrame::Raised);

        gridLayout_leftTop->addWidget(slot1_7, 2, 0, 1, 1);

        slot1_8 = new QLabel(leftTopGroup);
        slot1_8->setObjectName("slot1_8");
        slot1_8->setFrameShape(QFrame::Box);
        slot1_8->setFrameShadow(QFrame::Raised);

        gridLayout_leftTop->addWidget(slot1_8, 3, 0, 1, 1);

        slot1_1 = new QLabel(leftTopGroup);
        slot1_1->setObjectName("slot1_1");
        slot1_1->setFrameShape(QFrame::Box);
        slot1_1->setFrameShadow(QFrame::Raised);

        gridLayout_leftTop->addWidget(slot1_1, 0, 1, 1, 1);

        slot1_2 = new QLabel(leftTopGroup);
        slot1_2->setObjectName("slot1_2");
        slot1_2->setFrameShape(QFrame::Box);
        slot1_2->setFrameShadow(QFrame::Raised);

        gridLayout_leftTop->addWidget(slot1_2, 1, 1, 1, 1);

        slot1_3 = new QLabel(leftTopGroup);
        slot1_3->setObjectName("slot1_3");
        slot1_3->setFrameShape(QFrame::Box);
        slot1_3->setFrameShadow(QFrame::Raised);

        gridLayout_leftTop->addWidget(slot1_3, 2, 1, 1, 1);

        slot1_4 = new QLabel(leftTopGroup);
        slot1_4->setObjectName("slot1_4");
        slot1_4->setFrameShape(QFrame::Box);
        slot1_4->setFrameShadow(QFrame::Raised);

        gridLayout_leftTop->addWidget(slot1_4, 3, 1, 1, 1);


        gridLayout_weightData->addWidget(leftTopGroup, 0, 0, 1, 1);

        rightGroup = new QGroupBox(weightDataTab);
        rightGroup->setObjectName("rightGroup");
        verticalLayout_right = new QVBoxLayout(rightGroup);
        verticalLayout_right->setObjectName("verticalLayout_right");
        horizontalLayout_ngButtons = new QHBoxLayout();
        horizontalLayout_ngButtons->setObjectName("horizontalLayout_ngButtons");
        productionSupplementBtn = new QPushButton(rightGroup);
        productionSupplementBtn->setObjectName("productionSupplementBtn");
        productionSupplementBtn->setMaximumWidth(90);

        horizontalLayout_ngButtons->addWidget(productionSupplementBtn);

        ngDeleteBtn = new QPushButton(rightGroup);
        ngDeleteBtn->setObjectName("ngDeleteBtn");
        ngDeleteBtn->setMaximumWidth(70);

        horizontalLayout_ngButtons->addWidget(ngDeleteBtn);

        ngUseBtn = new QPushButton(rightGroup);
        ngUseBtn->setObjectName("ngUseBtn");
        ngUseBtn->setMaximumWidth(90);

        horizontalLayout_ngButtons->addWidget(ngUseBtn);

        horizontalSpacer_ng = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_ngButtons->addItem(horizontalSpacer_ng);


        verticalLayout_right->addLayout(horizontalLayout_ngButtons);

        ngTable = new QTableWidget(rightGroup);
        if (ngTable->columnCount() < 5)
            ngTable->setColumnCount(5);
        QTableWidgetItem *__qtablewidgetitem = new QTableWidgetItem();
        ngTable->setHorizontalHeaderItem(0, __qtablewidgetitem);
        QTableWidgetItem *__qtablewidgetitem1 = new QTableWidgetItem();
        ngTable->setHorizontalHeaderItem(1, __qtablewidgetitem1);
        QTableWidgetItem *__qtablewidgetitem2 = new QTableWidgetItem();
        ngTable->setHorizontalHeaderItem(2, __qtablewidgetitem2);
        QTableWidgetItem *__qtablewidgetitem3 = new QTableWidgetItem();
        ngTable->setHorizontalHeaderItem(3, __qtablewidgetitem3);
        QTableWidgetItem *__qtablewidgetitem4 = new QTableWidgetItem();
        ngTable->setHorizontalHeaderItem(4, __qtablewidgetitem4);
        ngTable->setObjectName("ngTable");
        ngTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        ngTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

        verticalLayout_right->addWidget(ngTable);


        gridLayout_weightData->addWidget(rightGroup, 0, 1, 2, 1);

        leftBottomGroup = new QGroupBox(weightDataTab);
        leftBottomGroup->setObjectName("leftBottomGroup");
        gridLayout_leftBottom = new QGridLayout(leftBottomGroup);
        gridLayout_leftBottom->setObjectName("gridLayout_leftBottom");
        slot2_5 = new QLabel(leftBottomGroup);
        slot2_5->setObjectName("slot2_5");
        slot2_5->setFrameShape(QFrame::Box);
        slot2_5->setFrameShadow(QFrame::Raised);

        gridLayout_leftBottom->addWidget(slot2_5, 0, 0, 1, 1);

        slot2_6 = new QLabel(leftBottomGroup);
        slot2_6->setObjectName("slot2_6");
        slot2_6->setFrameShape(QFrame::Box);
        slot2_6->setFrameShadow(QFrame::Raised);

        gridLayout_leftBottom->addWidget(slot2_6, 1, 0, 1, 1);

        slot2_7 = new QLabel(leftBottomGroup);
        slot2_7->setObjectName("slot2_7");
        slot2_7->setFrameShape(QFrame::Box);
        slot2_7->setFrameShadow(QFrame::Raised);

        gridLayout_leftBottom->addWidget(slot2_7, 2, 0, 1, 1);

        slot2_8 = new QLabel(leftBottomGroup);
        slot2_8->setObjectName("slot2_8");
        slot2_8->setFrameShape(QFrame::Box);
        slot2_8->setFrameShadow(QFrame::Raised);

        gridLayout_leftBottom->addWidget(slot2_8, 3, 0, 1, 1);

        slot2_1 = new QLabel(leftBottomGroup);
        slot2_1->setObjectName("slot2_1");
        slot2_1->setFrameShape(QFrame::Box);
        slot2_1->setFrameShadow(QFrame::Raised);

        gridLayout_leftBottom->addWidget(slot2_1, 0, 1, 1, 1);

        slot2_2 = new QLabel(leftBottomGroup);
        slot2_2->setObjectName("slot2_2");
        slot2_2->setFrameShape(QFrame::Box);
        slot2_2->setFrameShadow(QFrame::Raised);

        gridLayout_leftBottom->addWidget(slot2_2, 1, 1, 1, 1);

        slot2_3 = new QLabel(leftBottomGroup);
        slot2_3->setObjectName("slot2_3");
        slot2_3->setFrameShape(QFrame::Box);
        slot2_3->setFrameShadow(QFrame::Raised);

        gridLayout_leftBottom->addWidget(slot2_3, 2, 1, 1, 1);

        slot2_4 = new QLabel(leftBottomGroup);
        slot2_4->setObjectName("slot2_4");
        slot2_4->setFrameShape(QFrame::Box);
        slot2_4->setFrameShadow(QFrame::Raised);

        gridLayout_leftBottom->addWidget(slot2_4, 3, 1, 1, 1);


        gridLayout_weightData->addWidget(leftBottomGroup, 1, 0, 1, 1);

        tabWidget->addTab(weightDataTab, QString());
        historyTab = new QWidget();
        historyTab->setObjectName("historyTab");
        verticalLayout_history = new QVBoxLayout(historyTab);
        verticalLayout_history->setObjectName("verticalLayout_history");
        historyLeftGroup = new QGroupBox(historyTab);
        historyLeftGroup->setObjectName("historyLeftGroup");
        verticalLayout_historyLeft = new QVBoxLayout(historyLeftGroup);
        verticalLayout_historyLeft->setObjectName("verticalLayout_historyLeft");
        weightTable1 = new QTableWidget(historyLeftGroup);
        if (weightTable1->columnCount() < 18)
            weightTable1->setColumnCount(18);
        QTableWidgetItem *__qtablewidgetitem5 = new QTableWidgetItem();
        weightTable1->setHorizontalHeaderItem(0, __qtablewidgetitem5);
        QTableWidgetItem *__qtablewidgetitem6 = new QTableWidgetItem();
        weightTable1->setHorizontalHeaderItem(1, __qtablewidgetitem6);
        QTableWidgetItem *__qtablewidgetitem7 = new QTableWidgetItem();
        weightTable1->setHorizontalHeaderItem(2, __qtablewidgetitem7);
        QTableWidgetItem *__qtablewidgetitem8 = new QTableWidgetItem();
        weightTable1->setHorizontalHeaderItem(3, __qtablewidgetitem8);
        QTableWidgetItem *__qtablewidgetitem9 = new QTableWidgetItem();
        weightTable1->setHorizontalHeaderItem(4, __qtablewidgetitem9);
        QTableWidgetItem *__qtablewidgetitem10 = new QTableWidgetItem();
        weightTable1->setHorizontalHeaderItem(5, __qtablewidgetitem10);
        QTableWidgetItem *__qtablewidgetitem11 = new QTableWidgetItem();
        weightTable1->setHorizontalHeaderItem(6, __qtablewidgetitem11);
        QTableWidgetItem *__qtablewidgetitem12 = new QTableWidgetItem();
        weightTable1->setHorizontalHeaderItem(7, __qtablewidgetitem12);
        QTableWidgetItem *__qtablewidgetitem13 = new QTableWidgetItem();
        weightTable1->setHorizontalHeaderItem(8, __qtablewidgetitem13);
        QTableWidgetItem *__qtablewidgetitem14 = new QTableWidgetItem();
        weightTable1->setHorizontalHeaderItem(9, __qtablewidgetitem14);
        QTableWidgetItem *__qtablewidgetitem15 = new QTableWidgetItem();
        weightTable1->setHorizontalHeaderItem(10, __qtablewidgetitem15);
        QTableWidgetItem *__qtablewidgetitem16 = new QTableWidgetItem();
        weightTable1->setHorizontalHeaderItem(11, __qtablewidgetitem16);
        QTableWidgetItem *__qtablewidgetitem17 = new QTableWidgetItem();
        weightTable1->setHorizontalHeaderItem(12, __qtablewidgetitem17);
        QTableWidgetItem *__qtablewidgetitem18 = new QTableWidgetItem();
        weightTable1->setHorizontalHeaderItem(13, __qtablewidgetitem18);
        QTableWidgetItem *__qtablewidgetitem19 = new QTableWidgetItem();
        weightTable1->setHorizontalHeaderItem(14, __qtablewidgetitem19);
        QTableWidgetItem *__qtablewidgetitem20 = new QTableWidgetItem();
        weightTable1->setHorizontalHeaderItem(15, __qtablewidgetitem20);
        QTableWidgetItem *__qtablewidgetitem21 = new QTableWidgetItem();
        weightTable1->setHorizontalHeaderItem(16, __qtablewidgetitem21);
        QTableWidgetItem *__qtablewidgetitem22 = new QTableWidgetItem();
        weightTable1->setHorizontalHeaderItem(17, __qtablewidgetitem22);
        weightTable1->setObjectName("weightTable1");
        weightTable1->setSelectionBehavior(QAbstractItemView::SelectRows);
        weightTable1->setEditTriggers(QAbstractItemView::NoEditTriggers);

        verticalLayout_historyLeft->addWidget(weightTable1);

        horizontalLayout_table1 = new QHBoxLayout();
        horizontalLayout_table1->setObjectName("horizontalLayout_table1");
        clearTable1Btn = new QPushButton(historyLeftGroup);
        clearTable1Btn->setObjectName("clearTable1Btn");

        horizontalLayout_table1->addWidget(clearTable1Btn);

        exportTable1Btn = new QPushButton(historyLeftGroup);
        exportTable1Btn->setObjectName("exportTable1Btn");

        horizontalLayout_table1->addWidget(exportTable1Btn);

        horizontalSpacer_table1 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_table1->addItem(horizontalSpacer_table1);


        verticalLayout_historyLeft->addLayout(horizontalLayout_table1);


        verticalLayout_history->addWidget(historyLeftGroup);

        historyRightGroup = new QGroupBox(historyTab);
        historyRightGroup->setObjectName("historyRightGroup");
        verticalLayout_historyRight = new QVBoxLayout(historyRightGroup);
        verticalLayout_historyRight->setObjectName("verticalLayout_historyRight");
        weightTable2 = new QTableWidget(historyRightGroup);
        if (weightTable2->columnCount() < 18)
            weightTable2->setColumnCount(18);
        QTableWidgetItem *__qtablewidgetitem23 = new QTableWidgetItem();
        weightTable2->setHorizontalHeaderItem(0, __qtablewidgetitem23);
        QTableWidgetItem *__qtablewidgetitem24 = new QTableWidgetItem();
        weightTable2->setHorizontalHeaderItem(1, __qtablewidgetitem24);
        QTableWidgetItem *__qtablewidgetitem25 = new QTableWidgetItem();
        weightTable2->setHorizontalHeaderItem(2, __qtablewidgetitem25);
        QTableWidgetItem *__qtablewidgetitem26 = new QTableWidgetItem();
        weightTable2->setHorizontalHeaderItem(3, __qtablewidgetitem26);
        QTableWidgetItem *__qtablewidgetitem27 = new QTableWidgetItem();
        weightTable2->setHorizontalHeaderItem(4, __qtablewidgetitem27);
        QTableWidgetItem *__qtablewidgetitem28 = new QTableWidgetItem();
        weightTable2->setHorizontalHeaderItem(5, __qtablewidgetitem28);
        QTableWidgetItem *__qtablewidgetitem29 = new QTableWidgetItem();
        weightTable2->setHorizontalHeaderItem(6, __qtablewidgetitem29);
        QTableWidgetItem *__qtablewidgetitem30 = new QTableWidgetItem();
        weightTable2->setHorizontalHeaderItem(7, __qtablewidgetitem30);
        QTableWidgetItem *__qtablewidgetitem31 = new QTableWidgetItem();
        weightTable2->setHorizontalHeaderItem(8, __qtablewidgetitem31);
        QTableWidgetItem *__qtablewidgetitem32 = new QTableWidgetItem();
        weightTable2->setHorizontalHeaderItem(9, __qtablewidgetitem32);
        QTableWidgetItem *__qtablewidgetitem33 = new QTableWidgetItem();
        weightTable2->setHorizontalHeaderItem(10, __qtablewidgetitem33);
        QTableWidgetItem *__qtablewidgetitem34 = new QTableWidgetItem();
        weightTable2->setHorizontalHeaderItem(11, __qtablewidgetitem34);
        QTableWidgetItem *__qtablewidgetitem35 = new QTableWidgetItem();
        weightTable2->setHorizontalHeaderItem(12, __qtablewidgetitem35);
        QTableWidgetItem *__qtablewidgetitem36 = new QTableWidgetItem();
        weightTable2->setHorizontalHeaderItem(13, __qtablewidgetitem36);
        QTableWidgetItem *__qtablewidgetitem37 = new QTableWidgetItem();
        weightTable2->setHorizontalHeaderItem(14, __qtablewidgetitem37);
        QTableWidgetItem *__qtablewidgetitem38 = new QTableWidgetItem();
        weightTable2->setHorizontalHeaderItem(15, __qtablewidgetitem38);
        QTableWidgetItem *__qtablewidgetitem39 = new QTableWidgetItem();
        weightTable2->setHorizontalHeaderItem(16, __qtablewidgetitem39);
        QTableWidgetItem *__qtablewidgetitem40 = new QTableWidgetItem();
        weightTable2->setHorizontalHeaderItem(17, __qtablewidgetitem40);
        weightTable2->setObjectName("weightTable2");
        weightTable2->setSelectionBehavior(QAbstractItemView::SelectRows);
        weightTable2->setEditTriggers(QAbstractItemView::NoEditTriggers);

        verticalLayout_historyRight->addWidget(weightTable2);

        horizontalLayout_table2 = new QHBoxLayout();
        horizontalLayout_table2->setObjectName("horizontalLayout_table2");
        clearTable2Btn = new QPushButton(historyRightGroup);
        clearTable2Btn->setObjectName("clearTable2Btn");

        horizontalLayout_table2->addWidget(clearTable2Btn);

        exportTable2Btn = new QPushButton(historyRightGroup);
        exportTable2Btn->setObjectName("exportTable2Btn");

        horizontalLayout_table2->addWidget(exportTable2Btn);

        horizontalSpacer_table2 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_table2->addItem(horizontalSpacer_table2);


        verticalLayout_historyRight->addLayout(horizontalLayout_table2);


        verticalLayout_history->addWidget(historyRightGroup);

        tabWidget->addTab(historyTab, QString());

        verticalLayout->addWidget(tabWidget);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 1000, 22));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        tabWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "\347\247\260\351\207\215\345\257\271\346\257\224\350\275\257\344\273\266", nullptr));
        connectionGroup->setTitle(QCoreApplication::translate("MainWindow", "TCP\350\277\236\346\216\245\350\256\276\347\275\256", nullptr));
        label_serverAddress->setText(QCoreApplication::translate("MainWindow", "\346\234\215\345\212\241\345\231\250\345\234\260\345\235\200:", nullptr));
        serverAddressEdit->setPlaceholderText(QCoreApplication::translate("MainWindow", "\344\276\213\345\246\202: 192.168.1.100", nullptr));
        serverAddressEdit->setText(QCoreApplication::translate("MainWindow", "127.0.0.1", nullptr));
        label_serverPort->setText(QCoreApplication::translate("MainWindow", "\347\253\257\345\217\243:", nullptr));
        serverPortEdit->setPlaceholderText(QCoreApplication::translate("MainWindow", "\344\276\213\345\246\202: 8080", nullptr));
        serverPortEdit->setText(QCoreApplication::translate("MainWindow", "8080", nullptr));
        label_operation->setText(QCoreApplication::translate("MainWindow", "\346\223\215\344\275\234:", nullptr));
        connectBtn->setText(QCoreApplication::translate("MainWindow", "\350\277\236\346\216\245", nullptr));
        disconnectBtn->setText(QCoreApplication::translate("MainWindow", "\346\226\255\345\274\200", nullptr));
        label_status->setText(QCoreApplication::translate("MainWindow", "\350\277\236\346\216\245\347\212\266\346\200\201:", nullptr));
        connectionStatusLabel->setText(QCoreApplication::translate("MainWindow", "\346\234\252\350\277\236\346\216\245", nullptr));
        connectionStatusLabel->setStyleSheet(QCoreApplication::translate("MainWindow", "color: red; font-weight: bold;", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(settingsTab), QCoreApplication::translate("MainWindow", "\347\263\273\347\273\237\350\256\276\347\275\256", nullptr));
        inputGroup->setTitle(QCoreApplication::translate("MainWindow", "\346\267\273\345\212\240\347\273\221\345\256\232", nullptr));
        label_command->setText(QCoreApplication::translate("MainWindow", "\350\275\246\345\236\213\344\273\243\347\240\201:", nullptr));
        commandEdit->setPlaceholderText(QCoreApplication::translate("MainWindow", "\350\276\223\345\205\245\350\275\246\345\236\213\344\273\243\347\240\201", nullptr));
        label_itemName->setText(QCoreApplication::translate("MainWindow", "\350\275\246\345\236\213\345\220\215\347\247\260:", nullptr));
        itemNameEdit->setPlaceholderText(QCoreApplication::translate("MainWindow", "\350\276\223\345\205\245\350\275\246\345\236\213\345\220\215\347\247\260", nullptr));
        label_bindingOperation->setText(QCoreApplication::translate("MainWindow", "\346\223\215\344\275\234:", nullptr));
        addBindingBtn->setText(QCoreApplication::translate("MainWindow", "\346\267\273\345\212\240\347\273\221\345\256\232", nullptr));
        removeBindingBtn->setText(QCoreApplication::translate("MainWindow", "\345\210\240\351\231\244\351\200\211\344\270\255", nullptr));
        listGroup->setTitle(QCoreApplication::translate("MainWindow", "\347\273\221\345\256\232\345\210\227\350\241\250", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(bindingTab), QCoreApplication::translate("MainWindow", "\350\275\246\345\236\213\347\273\221\345\256\232", nullptr));
        leftTopGroup->setTitle(QCoreApplication::translate("MainWindow", "\347\254\254\344\270\200\346\211\230\345\217\257\350\247\206\345\214\226", nullptr));
        slot1_5->setText(QString());
        slot1_6->setText(QString());
        slot1_7->setText(QString());
        slot1_8->setText(QString());
        slot1_1->setText(QString());
        slot1_2->setText(QString());
        slot1_3->setText(QString());
        slot1_4->setText(QString());
        rightGroup->setTitle(QCoreApplication::translate("MainWindow", "NG\345\223\201\350\241\250\346\240\274", nullptr));
        productionSupplementBtn->setText(QCoreApplication::translate("MainWindow", "\347\224\237\344\272\247\350\241\245\345\205\205", nullptr));
        ngDeleteBtn->setText(QCoreApplication::translate("MainWindow", "\345\210\240\351\231\244", nullptr));
        ngUseBtn->setText(QCoreApplication::translate("MainWindow", "\345\244\207\347\224\250\350\260\203\345\205\245", nullptr));
        QTableWidgetItem *___qtablewidgetitem = ngTable->horizontalHeaderItem(0);
        ___qtablewidgetitem->setText(QCoreApplication::translate("MainWindow", "id", nullptr));
        QTableWidgetItem *___qtablewidgetitem1 = ngTable->horizontalHeaderItem(1);
        ___qtablewidgetitem1->setText(QCoreApplication::translate("MainWindow", "\350\275\246\345\236\213\345\220\215\347\247\260", nullptr));
        QTableWidgetItem *___qtablewidgetitem2 = ngTable->horizontalHeaderItem(2);
        ___qtablewidgetitem2->setText(QCoreApplication::translate("MainWindow", "\346\235\241\347\240\201", nullptr));
        QTableWidgetItem *___qtablewidgetitem3 = ngTable->horizontalHeaderItem(3);
        ___qtablewidgetitem3->setText(QCoreApplication::translate("MainWindow", "\351\207\215\351\207\217(g)", nullptr));
        QTableWidgetItem *___qtablewidgetitem4 = ngTable->horizontalHeaderItem(4);
        ___qtablewidgetitem4->setText(QCoreApplication::translate("MainWindow", "\346\227\266\351\227\264", nullptr));
        leftBottomGroup->setTitle(QCoreApplication::translate("MainWindow", "\347\254\254\344\272\214\346\211\230\345\217\257\350\247\206\345\214\226", nullptr));
        slot2_5->setText(QString());
        slot2_6->setText(QString());
        slot2_7->setText(QString());
        slot2_8->setText(QString());
        slot2_1->setText(QString());
        slot2_2->setText(QString());
        slot2_3->setText(QString());
        slot2_4->setText(QString());
        tabWidget->setTabText(tabWidget->indexOf(weightDataTab), QCoreApplication::translate("MainWindow", "\347\247\260\351\207\215\346\225\260\346\215\256", nullptr));
        historyLeftGroup->setTitle(QCoreApplication::translate("MainWindow", "\347\254\254\344\270\200\346\211\230\347\247\260\351\207\215\350\241\250\346\240\274", nullptr));
        QTableWidgetItem *___qtablewidgetitem5 = weightTable1->horizontalHeaderItem(0);
        ___qtablewidgetitem5->setText(QCoreApplication::translate("MainWindow", "\350\275\246\345\236\213\345\220\215\347\247\260", nullptr));
        QTableWidgetItem *___qtablewidgetitem6 = weightTable1->horizontalHeaderItem(1);
        ___qtablewidgetitem6->setText(QCoreApplication::translate("MainWindow", "\351\207\215\351\207\2171", nullptr));
        QTableWidgetItem *___qtablewidgetitem7 = weightTable1->horizontalHeaderItem(2);
        ___qtablewidgetitem7->setText(QCoreApplication::translate("MainWindow", "\346\235\241\347\240\2011", nullptr));
        QTableWidgetItem *___qtablewidgetitem8 = weightTable1->horizontalHeaderItem(3);
        ___qtablewidgetitem8->setText(QCoreApplication::translate("MainWindow", "\351\207\215\351\207\2172", nullptr));
        QTableWidgetItem *___qtablewidgetitem9 = weightTable1->horizontalHeaderItem(4);
        ___qtablewidgetitem9->setText(QCoreApplication::translate("MainWindow", "\346\235\241\347\240\2012", nullptr));
        QTableWidgetItem *___qtablewidgetitem10 = weightTable1->horizontalHeaderItem(5);
        ___qtablewidgetitem10->setText(QCoreApplication::translate("MainWindow", "\351\207\215\351\207\2173", nullptr));
        QTableWidgetItem *___qtablewidgetitem11 = weightTable1->horizontalHeaderItem(6);
        ___qtablewidgetitem11->setText(QCoreApplication::translate("MainWindow", "\346\235\241\347\240\2013", nullptr));
        QTableWidgetItem *___qtablewidgetitem12 = weightTable1->horizontalHeaderItem(7);
        ___qtablewidgetitem12->setText(QCoreApplication::translate("MainWindow", "\351\207\215\351\207\2174", nullptr));
        QTableWidgetItem *___qtablewidgetitem13 = weightTable1->horizontalHeaderItem(8);
        ___qtablewidgetitem13->setText(QCoreApplication::translate("MainWindow", "\346\235\241\347\240\2014", nullptr));
        QTableWidgetItem *___qtablewidgetitem14 = weightTable1->horizontalHeaderItem(9);
        ___qtablewidgetitem14->setText(QCoreApplication::translate("MainWindow", "\351\207\215\351\207\2175", nullptr));
        QTableWidgetItem *___qtablewidgetitem15 = weightTable1->horizontalHeaderItem(10);
        ___qtablewidgetitem15->setText(QCoreApplication::translate("MainWindow", "\346\235\241\347\240\2015", nullptr));
        QTableWidgetItem *___qtablewidgetitem16 = weightTable1->horizontalHeaderItem(11);
        ___qtablewidgetitem16->setText(QCoreApplication::translate("MainWindow", "\351\207\215\351\207\2176", nullptr));
        QTableWidgetItem *___qtablewidgetitem17 = weightTable1->horizontalHeaderItem(12);
        ___qtablewidgetitem17->setText(QCoreApplication::translate("MainWindow", "\346\235\241\347\240\2016", nullptr));
        QTableWidgetItem *___qtablewidgetitem18 = weightTable1->horizontalHeaderItem(13);
        ___qtablewidgetitem18->setText(QCoreApplication::translate("MainWindow", "\351\207\215\351\207\2177", nullptr));
        QTableWidgetItem *___qtablewidgetitem19 = weightTable1->horizontalHeaderItem(14);
        ___qtablewidgetitem19->setText(QCoreApplication::translate("MainWindow", "\346\235\241\347\240\2017", nullptr));
        QTableWidgetItem *___qtablewidgetitem20 = weightTable1->horizontalHeaderItem(15);
        ___qtablewidgetitem20->setText(QCoreApplication::translate("MainWindow", "\351\207\215\351\207\2178", nullptr));
        QTableWidgetItem *___qtablewidgetitem21 = weightTable1->horizontalHeaderItem(16);
        ___qtablewidgetitem21->setText(QCoreApplication::translate("MainWindow", "\346\235\241\347\240\2018", nullptr));
        QTableWidgetItem *___qtablewidgetitem22 = weightTable1->horizontalHeaderItem(17);
        ___qtablewidgetitem22->setText(QCoreApplication::translate("MainWindow", "\346\227\266\351\227\264", nullptr));
        clearTable1Btn->setText(QCoreApplication::translate("MainWindow", "\346\270\205\347\251\272", nullptr));
        exportTable1Btn->setText(QCoreApplication::translate("MainWindow", "\345\257\274\345\207\272", nullptr));
        historyRightGroup->setTitle(QCoreApplication::translate("MainWindow", "\347\254\254\344\272\214\346\211\230\347\247\260\351\207\215\350\241\250\346\240\274", nullptr));
        QTableWidgetItem *___qtablewidgetitem23 = weightTable2->horizontalHeaderItem(0);
        ___qtablewidgetitem23->setText(QCoreApplication::translate("MainWindow", "\350\275\246\345\236\213\345\220\215\347\247\260", nullptr));
        QTableWidgetItem *___qtablewidgetitem24 = weightTable2->horizontalHeaderItem(1);
        ___qtablewidgetitem24->setText(QCoreApplication::translate("MainWindow", "\351\207\215\351\207\2171", nullptr));
        QTableWidgetItem *___qtablewidgetitem25 = weightTable2->horizontalHeaderItem(2);
        ___qtablewidgetitem25->setText(QCoreApplication::translate("MainWindow", "\346\235\241\347\240\2011", nullptr));
        QTableWidgetItem *___qtablewidgetitem26 = weightTable2->horizontalHeaderItem(3);
        ___qtablewidgetitem26->setText(QCoreApplication::translate("MainWindow", "\351\207\215\351\207\2172", nullptr));
        QTableWidgetItem *___qtablewidgetitem27 = weightTable2->horizontalHeaderItem(4);
        ___qtablewidgetitem27->setText(QCoreApplication::translate("MainWindow", "\346\235\241\347\240\2012", nullptr));
        QTableWidgetItem *___qtablewidgetitem28 = weightTable2->horizontalHeaderItem(5);
        ___qtablewidgetitem28->setText(QCoreApplication::translate("MainWindow", "\351\207\215\351\207\2173", nullptr));
        QTableWidgetItem *___qtablewidgetitem29 = weightTable2->horizontalHeaderItem(6);
        ___qtablewidgetitem29->setText(QCoreApplication::translate("MainWindow", "\346\235\241\347\240\2013", nullptr));
        QTableWidgetItem *___qtablewidgetitem30 = weightTable2->horizontalHeaderItem(7);
        ___qtablewidgetitem30->setText(QCoreApplication::translate("MainWindow", "\351\207\215\351\207\2174", nullptr));
        QTableWidgetItem *___qtablewidgetitem31 = weightTable2->horizontalHeaderItem(8);
        ___qtablewidgetitem31->setText(QCoreApplication::translate("MainWindow", "\346\235\241\347\240\2014", nullptr));
        QTableWidgetItem *___qtablewidgetitem32 = weightTable2->horizontalHeaderItem(9);
        ___qtablewidgetitem32->setText(QCoreApplication::translate("MainWindow", "\351\207\215\351\207\2175", nullptr));
        QTableWidgetItem *___qtablewidgetitem33 = weightTable2->horizontalHeaderItem(10);
        ___qtablewidgetitem33->setText(QCoreApplication::translate("MainWindow", "\346\235\241\347\240\2015", nullptr));
        QTableWidgetItem *___qtablewidgetitem34 = weightTable2->horizontalHeaderItem(11);
        ___qtablewidgetitem34->setText(QCoreApplication::translate("MainWindow", "\351\207\215\351\207\2176", nullptr));
        QTableWidgetItem *___qtablewidgetitem35 = weightTable2->horizontalHeaderItem(12);
        ___qtablewidgetitem35->setText(QCoreApplication::translate("MainWindow", "\346\235\241\347\240\2016", nullptr));
        QTableWidgetItem *___qtablewidgetitem36 = weightTable2->horizontalHeaderItem(13);
        ___qtablewidgetitem36->setText(QCoreApplication::translate("MainWindow", "\351\207\215\351\207\2177", nullptr));
        QTableWidgetItem *___qtablewidgetitem37 = weightTable2->horizontalHeaderItem(14);
        ___qtablewidgetitem37->setText(QCoreApplication::translate("MainWindow", "\346\235\241\347\240\2017", nullptr));
        QTableWidgetItem *___qtablewidgetitem38 = weightTable2->horizontalHeaderItem(15);
        ___qtablewidgetitem38->setText(QCoreApplication::translate("MainWindow", "\351\207\215\351\207\2178", nullptr));
        QTableWidgetItem *___qtablewidgetitem39 = weightTable2->horizontalHeaderItem(16);
        ___qtablewidgetitem39->setText(QCoreApplication::translate("MainWindow", "\346\235\241\347\240\2018", nullptr));
        QTableWidgetItem *___qtablewidgetitem40 = weightTable2->horizontalHeaderItem(17);
        ___qtablewidgetitem40->setText(QCoreApplication::translate("MainWindow", "\346\227\266\351\227\264", nullptr));
        clearTable2Btn->setText(QCoreApplication::translate("MainWindow", "\346\270\205\347\251\272", nullptr));
        exportTable2Btn->setText(QCoreApplication::translate("MainWindow", "\345\257\274\345\207\272", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(historyTab), QCoreApplication::translate("MainWindow", "\345\216\206\345\217\262\350\256\260\345\275\225", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
