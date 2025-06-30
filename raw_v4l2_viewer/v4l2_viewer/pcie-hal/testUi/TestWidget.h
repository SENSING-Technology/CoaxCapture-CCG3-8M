#ifndef TESTWIDGET_H
#define TESTWIDGET_H

#include <QWidget>
#include <QScopedPointer>

namespace Ui {
class TestWidget;
}

class TestWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TestWidget(QWidget *parent = 0);
    ~TestWidget();

    void initVideoWall(int image_w, int image_h);
    void flashYuvData(int channel, unsigned char* data);
    void playVideoWall();

public slots:
    void playStop();

private:
    struct TestWidgetImpl;
    QScopedPointer<TestWidgetImpl> impl;
    int mCameraNum;
    int image_w, image_h;
};

#endif // TESTWIDGET_H
