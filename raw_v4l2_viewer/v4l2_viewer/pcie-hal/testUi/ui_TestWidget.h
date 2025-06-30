/********************************************************************************
** Form generated from reading UI file 'TestWidget.ui'
**
** Created by: Qt User Interface Compiler version 5.8.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_TESTWIDGET_H
#define UI_TESTWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_TestWidget
{
public:
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout_2;
    QLabel *label;
    QLineEdit *input_group;
    QLabel *label_2;
    QLineEdit *input_channel;
    QSpacerItem *horizontalSpacer_2;
    QPushButton *playAllButton;
    QWidget *videoWall;

    void setupUi(QWidget *TestWidget)
    {
        if (TestWidget->objectName().isEmpty())
            TestWidget->setObjectName(QStringLiteral("TestWidget"));
        TestWidget->resize(785, 558);
        verticalLayout = new QVBoxLayout(TestWidget);
        verticalLayout->setSpacing(2);
        verticalLayout->setContentsMargins(11, 11, 11, 11);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        verticalLayout->setContentsMargins(2, 2, 2, 2);
        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setSpacing(6);
        horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
        label = new QLabel(TestWidget);
        label->setObjectName(QStringLiteral("label"));

        horizontalLayout_2->addWidget(label);

        input_group = new QLineEdit(TestWidget);
        input_group->setObjectName(QStringLiteral("input_group"));

        horizontalLayout_2->addWidget(input_group);

        label_2 = new QLabel(TestWidget);
        label_2->setObjectName(QStringLiteral("label_2"));

        horizontalLayout_2->addWidget(label_2);

        input_channel = new QLineEdit(TestWidget);
        input_channel->setObjectName(QStringLiteral("input_channel"));

        horizontalLayout_2->addWidget(input_channel);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer_2);

        playAllButton = new QPushButton(TestWidget);
        playAllButton->setObjectName(QStringLiteral("playAllButton"));

        horizontalLayout_2->addWidget(playAllButton);


        verticalLayout->addLayout(horizontalLayout_2);

        videoWall = new QWidget(TestWidget);
        videoWall->setObjectName(QStringLiteral("videoWall"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(videoWall->sizePolicy().hasHeightForWidth());
        videoWall->setSizePolicy(sizePolicy);

        verticalLayout->addWidget(videoWall);


        retranslateUi(TestWidget);

        QMetaObject::connectSlotsByName(TestWidget);
    } // setupUi

    void retranslateUi(QWidget *TestWidget)
    {
        TestWidget->setWindowTitle(QApplication::translate("TestWidget", "TestWidget", Q_NULLPTR));
        label->setText(QApplication::translate("TestWidget", "Group", Q_NULLPTR));
        label_2->setText(QApplication::translate("TestWidget", "Channels", Q_NULLPTR));
        playAllButton->setText(QApplication::translate("TestWidget", "Play", Q_NULLPTR));
    } // retranslateUi

};

namespace Ui {
    class TestWidget: public Ui_TestWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_TESTWIDGET_H
