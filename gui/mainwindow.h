/*
     OpenDCP: Builds Digital Cinema Packages
     Copyright (c) 2010-2011 Terrence Meiczinger, All Rights Reserved

     This program is free software: you can redistribute it and/or modify
     it under the terms of the GNU General Public License as published by
     the Free Software Foundation, either version 3 of the License, or
     (at your option) any later version.

     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtGui>
#include <opendcp.h>
#include "settings.h"

class GenerateTitle;

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public slots:
    void getPath(QWidget *w);
    void wavInputSlot(QWidget*);
    void j2kInputSlot(QWidget *w);
    void mxfSoundOutputSlot();
    void setPictureTrack();
    void setSoundTrack();
    void setSubtitleTrack();

    void j2kStart();
    void j2kBwSliderUpdate();
    void j2kCinemaProfileUpdate();
    void j2kUpdateEndSpinBox();
    void j2kSetStereoscopicState();

    void mxfSetInitialState();
    void mxfStart();
    void mxfSourceTypeUpdate();
    void mxfSetStereoscopicState();
    void mxfSetSlideState();
    void mxfSetHVState();
    void mxfSetSoundState();
    void mxfSetSound71State(int state);
    void mxfSetSound51State(int state);
    int  mxfCheckSoundInput20();
    int  mxfCheckSoundInput51();
    int  mxfCheckSoundInput71();
    void mxfSetSoundInputTypeState();

    void mxfCreateSubtitle();

    void startDcp();
    void updatePictureDuration();
    void updateSoundDuration();
    void updateSubtitleDuration();

    void getTitle();
    void preview(QString filename);

private slots:
    void newFile();
    void open();
    bool save();
    bool saveAs();
    void preferences();
    void about();

protected:
    void setInitialUiState();
    void connectSlots();
    void j2kConnectSlots();
    void j2kConvert();
    void mxfConnectSlots();
    void mxfCreatePicture();
    void mxfCreateAudio();

    void connectXmlSlots();
    void showImage(QImage image);
    int  checkFileSequence(QFileInfoList list);
    int  checkSequential(const char str1[], const char str2[]);

protected slots:

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void fileCopy(QString source, QString destination);
    QString calculateDigest(opendcp_t *opendcp, QString text, QString filename);

private:
    void createActions();
    void createMenus();

    void loadLanguage(const QString& rLanguage);
    void createLanguageMenu(void);
    void mxfStartThread(opendcp_t *opendcp, QFileInfoList inputList, QString outputFile);
    void processOptions(opendcp_t *opendcp);
    void mxfAddInputWavFiles(QFileInfoList *inputList);
    int  mxfCopy(QString source, QString destination);

    filelist_t *QStringToFilelist(QFileInfoList list);
    int checkWavInfo(QFileInfoList filelist, int frameRate);

    Ui::MainWindow      *ui;
    Settings            *settings;

    QSignalMapper       signalMapper;
    QSignalMapper       wavSignalMapper;

    GenerateTitle       *generateTitle;
    QString             lastDir;

    // copy/paste
    QPlainTextEdit *textEdit;

    // menu information
    QMenu *fileMenu;
    QMenu *editMenu;
    QMenu *helpMenu;
    QToolBar *fileToolBar;
    QToolBar *editToolBar;
    QAction *newAct;
    QAction *openAct;
    QAction *saveAct;
    QAction *saveAsAct;
    QAction *exitAct;
    QAction *cutAct;
    QAction *copyAct;
    QAction *pasteAct;
    QAction *aboutAct;
    QAction *preferencesAct;
};

#endif // MAINWINDOW_H
