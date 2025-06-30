#include <iostream>
#include <QApplication>
#include <QFile>
#include <QByteArray>
#include <QDebug>
#include <QThread>
#include <QList>

#include "TestWidget.h"
#include "ui_TestWidget.h"
#include "OpenGLDisplay.h"
#include "../include/pcie_camera.h"


/*将大写字母转换成小写字母*/
int tolower(int c)
{
    if (c >= 'A' && c <= 'Z')
    {
        return c + 'a' - 'A';
    }
    else
    {
        return c;
    }
}

int _htoi(char s[])
{
    int i;
    int n = 0;
    if (s[0] == '0' && (s[1]=='x' || s[1]=='X'))
    {
        i = 2;
    }
    else
    {
        i = 0;
    }
    for (; (s[i] >= '0' && s[i] <= '9') || (s[i] >= 'a' && s[i] <= 'z') || (s[i] >='A' && s[i] <= 'Z');++i)
    {
        if (tolower(s[i]) > '9')
        {
            n = 16 * n + (10 + tolower(s[i]) - 'a');
        }
        else
        {
            n = 16 * n + (tolower(s[i]) - '0');
        }
    }
    return n;
}

struct TestWidget::TestWidgetImpl
{
    TestWidgetImpl()
        : ui(new Ui::TestWidget)
    {}

    Ui::TestWidget*             ui;

    QList<OpenGLDisplay*>       mPlayers;
};

TestWidget::TestWidget(QWidget *parent)
    : QWidget(parent)
    , impl(new TestWidgetImpl)
{
    impl->ui->setupUi(this);
    impl->ui->playAllButton->setEnabled(true);
    impl->ui->input_channel->setText("0xff");
    impl->ui->input_group->setText("0");

    connect(impl->ui->playAllButton, &QPushButton::clicked, this, &TestWidget::playStop);
}

void TestWidget::initVideoWall(int image_w, int image_h){

    QGridLayout* wallLayout = new QGridLayout;

    this->image_w = image_w;
    this->image_h = image_h;

    for (int i = 0; i < 2; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            OpenGLDisplay* player = new OpenGLDisplay(this);
            wallLayout->addWidget(player, i, j);
            impl->mPlayers.append(player);
        }
    }

    for (int i = 0; i < impl->mPlayers.count(); ++i)
        impl->mPlayers[i]->InitDrawBuffer(image_w * image_h * 2);


    wallLayout->setSpacing(2);
    wallLayout->setMargin(0);

    impl->ui->videoWall->setLayout(wallLayout);
}

TestWidget::~TestWidget()
{
    printf("TestWidget::~TestWidget\n");
}

void TestWidget::flashYuvData(int channel,unsigned char* data)
{
    impl->mPlayers[channel]->DisplayVideoFrame(data, image_w, image_h);
}

void TestWidget::playVideoWall()
{
    qApp->processEvents();
}
extern bool main_exit;
extern camera_info info;
void TestWidget::playStop(){
    if(main_exit == false){
        info.group = impl->ui->input_group->text().toInt();
        QByteArray ba = impl->ui->input_channel->text().toLatin1();
        info.channel_mask = _htoi(ba.data());
        std::cout<<"input_group: "<<info.group<<" input_mask: "<< info.channel_mask<<std::endl;

        /*when not input ,use defined 0xff*/
        if(info.channel_mask == 0)
            info.channel_mask = 0xff;

        impl->ui->playAllButton->setText("PlayStop");

        main_exit = true;
    }else{
        impl->ui->playAllButton->setText("PlayStart");
        main_exit = false;
    }

}

