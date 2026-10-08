/********************************************************************************
** Form generated from reading UI file 'Controller.ui'
**
** Created by: Qt User Interface Compiler version 6.10.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CONTROLLER_H
#define UI_CONTROLLER_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include "plotview.h"

QT_BEGIN_NAMESPACE

class Ui_dialog
{
public:
    QVBoxLayout *rootLayout;
    QHBoxLayout *contentLayout;
    PlotView *graphicsView;
    QVBoxLayout *rightColumn;
    QGroupBox *statusGroup;
    QFormLayout *statusForm;
    QLabel *label_6;
    QLabel *valueMotorEncoder;
    QLabel *label_7;
    QLabel *valueGearEncoder;
    QLabel *label_8;
    QLabel *valueCurrentPosition;
    QLabel *label_9;
    QLabel *valueVelocity;
    QLabel *label_10;
    QLabel *valueAcceleration;
    QGroupBox *pidGroup;
    QFormLayout *pidForm;
    QLabel *label_3;
    QLineEdit *lineEdit_3;
    QLabel *label;
    QLineEdit *lineEdit_2;
    QLabel *label_2;
    QLineEdit *lineEdit;
    QGroupBox *targetGroup;
    QHBoxLayout *targetLayout;
    QLabel *label_4;
    QLineEdit *lineEdit_4;
    QLabel *label_5;
    QSpacerItem *rightStretch;
    QHBoxLayout *buttonRow;
    QPushButton *demoButton;
    QPushButton *resetButton;
    QPushButton *pushButton;

    void setupUi(QDialog *dialog)
    {
        if (dialog->objectName().isEmpty())
            dialog->setObjectName("dialog");
        dialog->resize(980, 580);
        dialog->setMinimumSize(QSize(900, 540));
        dialog->setStyleSheet(QString::fromUtf8("QDialog {\n"
"    background: #E3B64D;\n"
"}\n"
"\n"
"QLabel {\n"
"    color: #22262D;\n"
"    font-size: 13px;\n"
"}\n"
"\n"
"QLabel[valueField=\"true\"] {\n"
"    color: #22262D;\n"
"    font-size: 13px;\n"
"    background: #F6E4BB;\n"
"    border: none;\n"
"    border-left: 2px solid #22262D;\n"
"    border-radius: 0px;\n"
"    padding: 6px 12px;\n"
"    min-width: 130px;\n"
"    min-height: 18px;\n"
"}\n"
"\n"
"QGroupBox {\n"
"    color: #22262D;\n"
"    font-size: 12px;\n"
"    border: 1px solid #8A6300;\n"
"    border-radius: 0px;\n"
"    margin-top: 10px;\n"
"}\n"
"\n"
"QGroupBox::title {\n"
"    subcontrol-origin: margin;\n"
"    subcontrol-position: top left;\n"
"    left: 12px;\n"
"    padding: 0px 6px;\n"
"}\n"
"\n"
"QLineEdit {\n"
"    background: #F6E4BB;\n"
"    color: #22262D;\n"
"    border: 1px solid #8A6300;\n"
"    border-radius: 0px;\n"
"    padding: 8px 12px;\n"
"    min-width: 150px;\n"
"    min-height: 20px;\n"
"    selection-background-color: #22262D;\n"
"    selection-color: #F6E4BB;\n"
""
                        "}\n"
"\n"
"QLineEdit:hover {\n"
"    border: 1px solid #22262D;\n"
"}\n"
"\n"
"QLineEdit:focus {\n"
"    border: 1px solid #22262D;\n"
"    background: #FCF6E8;\n"
"}\n"
"\n"
"QPushButton {\n"
"    background: #22262D;\n"
"    color: #E3B64D;\n"
"    border: none;\n"
"    border-radius: 0px;\n"
"    padding: 9px 24px;\n"
"    font-size: 13px;\n"
"}\n"
"\n"
"QPushButton:hover {\n"
"    background: #535A66;\n"
"}\n"
"\n"
"QPushButton:pressed {\n"
"    background: #000000;\n"
"}\n"
"\n"
"QPushButton#resetButton, QPushButton#demoButton {\n"
"    background: transparent;\n"
"    color: #22262D;\n"
"    border: 1px solid #22262D;\n"
"}\n"
"\n"
"QPushButton#resetButton:hover, QPushButton#demoButton:hover {\n"
"    background: #FCF6E8;\n"
"    border: 1px solid #22262D;\n"
"}\n"
"\n"
"QGraphicsView {\n"
"    background: #F6E4BB;\n"
"    border: 1px solid #8A6300;\n"
"    border-radius: 0px;\n"
"}"));
        rootLayout = new QVBoxLayout(dialog);
        rootLayout->setSpacing(16);
        rootLayout->setObjectName("rootLayout");
        rootLayout->setContentsMargins(20, 18, 20, 18);
        contentLayout = new QHBoxLayout();
        contentLayout->setSpacing(18);
        contentLayout->setObjectName("contentLayout");
        graphicsView = new PlotView(dialog);
        graphicsView->setObjectName("graphicsView");
        QSizePolicy sizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
        sizePolicy.setHorizontalStretch(1);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(graphicsView->sizePolicy().hasHeightForWidth());
        graphicsView->setSizePolicy(sizePolicy);
        graphicsView->setMinimumSize(QSize(420, 340));
        graphicsView->setFrameShape(QFrame::NoFrame);

        contentLayout->addWidget(graphicsView);

        rightColumn = new QVBoxLayout();
        rightColumn->setSpacing(14);
        rightColumn->setObjectName("rightColumn");
        statusGroup = new QGroupBox(dialog);
        statusGroup->setObjectName("statusGroup");
        statusGroup->setMinimumSize(QSize(370, 0));
        statusForm = new QFormLayout(statusGroup);
        statusForm->setObjectName("statusForm");
        statusForm->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
        statusForm->setLabelAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
        statusForm->setHorizontalSpacing(12);
        statusForm->setVerticalSpacing(8);
        statusForm->setContentsMargins(14, 24, 14, 14);
        label_6 = new QLabel(statusGroup);
        label_6->setObjectName("label_6");

        statusForm->setWidget(1, QFormLayout::ItemRole::LabelRole, label_6);

        valueMotorEncoder = new QLabel(statusGroup);
        valueMotorEncoder->setObjectName("valueMotorEncoder");
        valueMotorEncoder->setTextInteractionFlags(Qt::TextSelectableByMouse);
        valueMotorEncoder->setProperty("valueField", QVariant(true));
        valueMotorEncoder->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        statusForm->setWidget(1, QFormLayout::ItemRole::FieldRole, valueMotorEncoder);

        label_7 = new QLabel(statusGroup);
        label_7->setObjectName("label_7");

        statusForm->setWidget(2, QFormLayout::ItemRole::LabelRole, label_7);

        valueGearEncoder = new QLabel(statusGroup);
        valueGearEncoder->setObjectName("valueGearEncoder");
        valueGearEncoder->setTextInteractionFlags(Qt::TextSelectableByMouse);
        valueGearEncoder->setProperty("valueField", QVariant(true));
        valueGearEncoder->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        statusForm->setWidget(2, QFormLayout::ItemRole::FieldRole, valueGearEncoder);

        label_8 = new QLabel(statusGroup);
        label_8->setObjectName("label_8");

        statusForm->setWidget(0, QFormLayout::ItemRole::LabelRole, label_8);

        valueCurrentPosition = new QLabel(statusGroup);
        valueCurrentPosition->setObjectName("valueCurrentPosition");
        valueCurrentPosition->setTextInteractionFlags(Qt::TextSelectableByMouse);
        valueCurrentPosition->setProperty("valueField", QVariant(true));
        valueCurrentPosition->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        statusForm->setWidget(0, QFormLayout::ItemRole::FieldRole, valueCurrentPosition);

        label_9 = new QLabel(statusGroup);
        label_9->setObjectName("label_9");

        statusForm->setWidget(3, QFormLayout::ItemRole::LabelRole, label_9);

        valueVelocity = new QLabel(statusGroup);
        valueVelocity->setObjectName("valueVelocity");
        valueVelocity->setTextInteractionFlags(Qt::TextSelectableByMouse);
        valueVelocity->setProperty("valueField", QVariant(true));
        valueVelocity->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        statusForm->setWidget(3, QFormLayout::ItemRole::FieldRole, valueVelocity);

        label_10 = new QLabel(statusGroup);
        label_10->setObjectName("label_10");

        statusForm->setWidget(4, QFormLayout::ItemRole::LabelRole, label_10);

        valueAcceleration = new QLabel(statusGroup);
        valueAcceleration->setObjectName("valueAcceleration");
        valueAcceleration->setTextInteractionFlags(Qt::TextSelectableByMouse);
        valueAcceleration->setProperty("valueField", QVariant(true));
        valueAcceleration->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        statusForm->setWidget(4, QFormLayout::ItemRole::FieldRole, valueAcceleration);


        rightColumn->addWidget(statusGroup);

        pidGroup = new QGroupBox(dialog);
        pidGroup->setObjectName("pidGroup");
        pidForm = new QFormLayout(pidGroup);
        pidForm->setObjectName("pidForm");
        pidForm->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
        pidForm->setLabelAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
        pidForm->setHorizontalSpacing(12);
        pidForm->setVerticalSpacing(8);
        pidForm->setContentsMargins(14, 24, 14, 14);
        label_3 = new QLabel(pidGroup);
        label_3->setObjectName("label_3");

        pidForm->setWidget(0, QFormLayout::ItemRole::LabelRole, label_3);

        lineEdit_3 = new QLineEdit(pidGroup);
        lineEdit_3->setObjectName("lineEdit_3");
        lineEdit_3->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        pidForm->setWidget(0, QFormLayout::ItemRole::FieldRole, lineEdit_3);

        label = new QLabel(pidGroup);
        label->setObjectName("label");

        pidForm->setWidget(1, QFormLayout::ItemRole::LabelRole, label);

        lineEdit_2 = new QLineEdit(pidGroup);
        lineEdit_2->setObjectName("lineEdit_2");
        lineEdit_2->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        pidForm->setWidget(1, QFormLayout::ItemRole::FieldRole, lineEdit_2);

        label_2 = new QLabel(pidGroup);
        label_2->setObjectName("label_2");

        pidForm->setWidget(2, QFormLayout::ItemRole::LabelRole, label_2);

        lineEdit = new QLineEdit(pidGroup);
        lineEdit->setObjectName("lineEdit");
        lineEdit->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        pidForm->setWidget(2, QFormLayout::ItemRole::FieldRole, lineEdit);


        rightColumn->addWidget(pidGroup);

        targetGroup = new QGroupBox(dialog);
        targetGroup->setObjectName("targetGroup");
        targetLayout = new QHBoxLayout(targetGroup);
        targetLayout->setSpacing(10);
        targetLayout->setObjectName("targetLayout");
        targetLayout->setContentsMargins(14, 24, 14, 14);
        label_4 = new QLabel(targetGroup);
        label_4->setObjectName("label_4");

        targetLayout->addWidget(label_4);

        lineEdit_4 = new QLineEdit(targetGroup);
        lineEdit_4->setObjectName("lineEdit_4");
        lineEdit_4->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        targetLayout->addWidget(lineEdit_4);

        label_5 = new QLabel(targetGroup);
        label_5->setObjectName("label_5");

        targetLayout->addWidget(label_5);


        rightColumn->addWidget(targetGroup);

        rightStretch = new QSpacerItem(20, 20, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        rightColumn->addItem(rightStretch);

        buttonRow = new QHBoxLayout();
        buttonRow->setSpacing(10);
        buttonRow->setObjectName("buttonRow");
        demoButton = new QPushButton(dialog);
        demoButton->setObjectName("demoButton");
        demoButton->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));

        buttonRow->addWidget(demoButton);

        resetButton = new QPushButton(dialog);
        resetButton->setObjectName("resetButton");
        resetButton->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));

        buttonRow->addWidget(resetButton);

        pushButton = new QPushButton(dialog);
        pushButton->setObjectName("pushButton");
        QSizePolicy sizePolicy1(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
        sizePolicy1.setHorizontalStretch(1);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(pushButton->sizePolicy().hasHeightForWidth());
        pushButton->setSizePolicy(sizePolicy1);
        pushButton->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));

        buttonRow->addWidget(pushButton);


        rightColumn->addLayout(buttonRow);


        contentLayout->addLayout(rightColumn);


        rootLayout->addLayout(contentLayout);


        retranslateUi(dialog);

        pushButton->setDefault(true);


        QMetaObject::connectSlotsByName(dialog);
    } // setupUi

    void retranslateUi(QDialog *dialog)
    {
        dialog->setWindowTitle(QCoreApplication::translate("dialog", "Motor Controller", nullptr));
        statusGroup->setTitle(QCoreApplication::translate("dialog", "STATUS", nullptr));
        label_6->setText(QCoreApplication::translate("dialog", "Motor Encoder", nullptr));
        valueMotorEncoder->setText(QCoreApplication::translate("dialog", "0", nullptr));
        label_7->setText(QCoreApplication::translate("dialog", "Gear Encoder", nullptr));
        valueGearEncoder->setText(QCoreApplication::translate("dialog", "0", nullptr));
        label_8->setText(QCoreApplication::translate("dialog", "Current Position", nullptr));
        valueCurrentPosition->setText(QCoreApplication::translate("dialog", "0.00 \302\260", nullptr));
        label_9->setText(QCoreApplication::translate("dialog", "Velocity", nullptr));
        valueVelocity->setText(QCoreApplication::translate("dialog", "0.00 \302\260/s", nullptr));
        label_10->setText(QCoreApplication::translate("dialog", "Acceleration", nullptr));
        valueAcceleration->setText(QCoreApplication::translate("dialog", "0.00 \302\260/s\302\262", nullptr));
        pidGroup->setTitle(QCoreApplication::translate("dialog", "PID PARAMETER", nullptr));
        label_3->setText(QCoreApplication::translate("dialog", "P", nullptr));
        lineEdit_3->setPlaceholderText(QCoreApplication::translate("dialog", "0.0", nullptr));
        label->setText(QCoreApplication::translate("dialog", "I", nullptr));
        lineEdit_2->setPlaceholderText(QCoreApplication::translate("dialog", "0.0", nullptr));
        label_2->setText(QCoreApplication::translate("dialog", "D", nullptr));
        lineEdit->setPlaceholderText(QCoreApplication::translate("dialog", "0.0", nullptr));
        targetGroup->setTitle(QCoreApplication::translate("dialog", "ZIELPOSITION", nullptr));
        label_4->setText(QCoreApplication::translate("dialog", "Goal", nullptr));
        lineEdit_4->setPlaceholderText(QCoreApplication::translate("dialog", "0.0", nullptr));
        label_5->setText(QCoreApplication::translate("dialog", "deg", nullptr));
        demoButton->setText(QCoreApplication::translate("dialog", "Probelauf", nullptr));
        resetButton->setText(QCoreApplication::translate("dialog", "Reset", nullptr));
        pushButton->setText(QCoreApplication::translate("dialog", "Apply", nullptr));
    } // retranslateUi

};

namespace Ui {
    class dialog: public Ui_dialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CONTROLLER_H
