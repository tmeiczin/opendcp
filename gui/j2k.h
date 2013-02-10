#ifndef J2K_H
#define J2K_H

#include <QDialog>
#include <QtGui>
#include <opendcp.h>

namespace Ui {
    class j2k;
}

class j2k : public QDialog
{
    Q_OBJECT

public:
    explicit j2k(QWidget *parent = 0);
    ~j2k();

public slots:
    void setStereoscopicState();
    void getPath(QWidget *w);
    void start();
    void qualitySliderUpdate();

protected:
    void setInitialUiState();
    void connectSlots();
    void convert();

private:
    Ui::Dialog *ui;
    QSignalMapper signalMapper;
};

#endif // J2K_H
