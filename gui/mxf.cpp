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

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <stdio.h>
#include <stdlib.h>
#include <opendcp.h>
#include "mxf_writer.h"
#include "conversion_dialog.h"

enum MXF_ESSENCE_TYPE {
    JPEG2000 = 0,
    MPEG2,
    WAV
};

enum MXF_LABEL_TYPE {
    MXF_INTEROP = 0,
    SMPTE
};

enum MXF_CLASS_TYPE {
    PICTURE = 0,
    SOUND
};

enum MXF_STATE {
    DISABLED = 0,
    ENABLED  = 1
};

enum WAV_INPUT_TYPE {
    WAV_INPUT_MONO  = 0,
    WAV_INPUT_MULTI
};

void MainWindow::mxfConnectSlots() {
    // connect slots
    connect(ui->mxfStereoscopicCheckBox, SIGNAL(stateChanged(int)),        this, SLOT(mxfSetStereoscopicState()));
    connect(ui->mxfSoundRadio2,          SIGNAL(clicked()),                this, SLOT(mxfSetSoundState()));
    connect(ui->mxfSoundRadio5,          SIGNAL(clicked()),                this, SLOT(mxfSetSoundState()));
    connect(ui->mxfSoundRadio7,          SIGNAL(clicked()),                this, SLOT(mxfSetSoundState()));
    connect(ui->mxfSoundTypeRadioMono,   SIGNAL(clicked()),                this, SLOT(mxfSetSoundInputTypeState()));
    connect(ui->mxfSoundTypeRadioMulti,  SIGNAL(clicked()),                this, SLOT(mxfSetSoundInputTypeState()));
    connect(ui->mxfHVCheckBox,           SIGNAL(stateChanged(int)),        this, SLOT(mxfSetHVState()));
    connect(ui->mxfSourceTypeComboBox,   SIGNAL(currentIndexChanged(int)), this, SLOT(mxfSourceTypeUpdate()));
    connect(ui->mxfButton,               SIGNAL(clicked()),                this, SLOT(mxfStart()));
    connect(ui->subCreateButton,         SIGNAL(clicked()),                this, SLOT(mxfCreateSubtitle()));
    connect(ui->mxfSlideCheckBox,        SIGNAL(stateChanged(int)),        this, SLOT(mxfSetSlideState()));
    connect(ui->mxfEncryptionCheckBox,   SIGNAL(stateChanged(int)),        this, SLOT(mxfSetEncryptionState()));

    // Picture input
    signalMapper.setMapping(ui->pictureLeftButton,  ui->pictureLeftEdit);
    signalMapper.setMapping(ui->pictureRightButton, ui->pictureRightEdit);

    connect(ui->pictureLeftButton,  SIGNAL(clicked()), &signalMapper, SLOT(map()));
    connect(ui->pictureRightButton, SIGNAL(clicked()), &signalMapper, SLOT(map()));

    // Sound input
    wavSignalMapper.setMapping(ui->aLeftButton,   ui->aLeftEdit);
    wavSignalMapper.setMapping(ui->aRightButton,  ui->aRightEdit);
    wavSignalMapper.setMapping(ui->aCenterButton, ui->aCenterEdit);
    wavSignalMapper.setMapping(ui->aSubButton,    ui->aSubEdit);
    wavSignalMapper.setMapping(ui->aLeftSButton,  ui->aLeftSEdit);
    wavSignalMapper.setMapping(ui->aRightSButton, ui->aRightSEdit);
    wavSignalMapper.setMapping(ui->aLeftCButton,  ui->aLeftCEdit);
    wavSignalMapper.setMapping(ui->aRightCButton, ui->aRightCEdit);
    wavSignalMapper.setMapping(ui->aHIButton,     ui->aHIEdit);
    wavSignalMapper.setMapping(ui->aVIButton,     ui->aVIEdit);
    wavSignalMapper.setMapping(ui->aMultiButton,  ui->aMultiEdit);

    connect(ui->aLeftButton,   SIGNAL(clicked()), &wavSignalMapper, SLOT(map()));
    connect(ui->aRightButton,  SIGNAL(clicked()), &wavSignalMapper, SLOT(map()));
    connect(ui->aCenterButton, SIGNAL(clicked()), &wavSignalMapper, SLOT(map()));
    connect(ui->aSubButton,    SIGNAL(clicked()), &wavSignalMapper, SLOT(map()));
    connect(ui->aLeftSButton,  SIGNAL(clicked()), &wavSignalMapper, SLOT(map()));
    connect(ui->aRightSButton, SIGNAL(clicked()), &wavSignalMapper, SLOT(map()));
    connect(ui->aLeftCButton,  SIGNAL(clicked()), &wavSignalMapper, SLOT(map()));
    connect(ui->aRightCButton, SIGNAL(clicked()), &wavSignalMapper, SLOT(map()));
    connect(ui->aHIButton,     SIGNAL(clicked()), &wavSignalMapper, SLOT(map()));
    connect(ui->aVIButton,     SIGNAL(clicked()), &wavSignalMapper, SLOT(map()));
    connect(ui->aMultiButton,  SIGNAL(clicked()), &wavSignalMapper, SLOT(map()));

    connect(&wavSignalMapper, SIGNAL(mapped(QWidget*)), this, SLOT(wavInputSlot(QWidget*)));

    // Subtitle input
    signalMapper.setMapping(ui->subInButton, ui->subInEdit);

    connect(ui->subInButton,        SIGNAL(clicked()), &signalMapper, SLOT(map()));

    // MXF Sound output file
    connect(ui->aMxfOutButton,      SIGNAL(clicked()), this, SLOT(mxfSoundOutputSlot()));
    connect(ui->aMxfOutButtonMulti, SIGNAL(clicked()), this, SLOT(mxfSoundOutputSlot()));

    // MXF Picture/Subtitle output file
    signalMapper.setMapping(ui->pMxfOutButton, ui->pMxfOutEdit);
    signalMapper.setMapping(ui->sMxfOutButton, ui->sMxfOutEdit);

    connect(ui->pMxfOutButton,      SIGNAL(clicked()), &signalMapper, SLOT(map()));
    connect(ui->sMxfOutButton,      SIGNAL(clicked()), &signalMapper, SLOT(map()));
}

void MainWindow::mxfSetInitialState() {
    ui->mxfSoundTypeRadioMono->setChecked(true);
    mxfSetHVState();
    mxfSetSoundState();
    mxfSetSlideState();
    mxfSetEncryptionState();
}

void MainWindow::mxfSoundOutputSlot() {
    QString path;
    QString filter;

    filter = "*.mxf";

    path = QFileDialog::getSaveFileName(this, tr("Save MXF as"),lastDir,filter);

    if (path.isEmpty()) {
        return;
    }

    QFileInfo fi(path);
    lastDir = fi.absolutePath();

    ui->aMxfOutEdit->setText(path);
    ui->aMxfOutEditMulti->setText(path);
}

void MainWindow::wavInputSlot(QWidget *w)
{
    QString path;
    QString filter;
    QFileInfoList inputList;
    int     frameRate;
    int     rc;

    filter = "*.wav";
    path = QFileDialog::getOpenFileName(this, tr("Choose a wav file to open"), lastDir, filter);

    if (path.isEmpty()) {
        return;
    }

    QFileInfo fi(path);
    lastDir = fi.absolutePath();

    frameRate = ui->mxfFrameRateComboBox->currentText().toInt();
    inputList.append(path);
    rc = checkWavInfo(inputList, frameRate);

    if (rc == OPENDCP_INVALID_WAV_CHANNELS) {
        if (ui->mxfSoundTypeRadioMono->isChecked()) {
            QMessageBox::warning(this, tr("Invalid Wav"),
                                       tr("The selected wav file is not Mono"));
        } else {
            QMessageBox::warning(this, tr("Invalid Wav"),
                                       tr("The selected wav file is not Stereo, 5.1, or 7.1"));
        }
        return;
    }

    if (rc == OPENDCP_INVALID_WAV_BITDEPTH) {
        QMessageBox::warning(this, tr("Invalid Wav"),
                                   tr("The selected wav file is not 24-bit"));
        return;
    }

    if (rc) {
        QMessageBox::warning(this, tr("Invalid Wav"),
                                   tr("The selected wav file is invalid"));
        return;
    }

    w->setProperty("text", path);
}

void MainWindow::mxfSetSlideState() {
    int value;

    if (ui->mxfSourceTypeComboBox->currentIndex() == JPEG2000) {
        value = ui->mxfSlideCheckBox->checkState();
        ui->mxfSlideCheckBox->setEnabled(ENABLED);
    } else {
        ui->mxfSlideCheckBox->setEnabled(DISABLED);
        value = DISABLED;
    }

    ui->mxfSlideSpinBox->setEnabled(value);
    ui->mxfSlideDurationLabel->setEnabled(value);
}

void MainWindow::mxfSetEncryptionState() {
    int value;

    value = ui->mxfEncryptionCheckBox->checkState();
    ui->mxfKeyEdit->setEnabled(value);
    ui->mxfKeyIdEdit->setEnabled(value);
}

void MainWindow::mxfSourceTypeUpdate() {
    // JPEG2000
    if (ui->mxfSourceTypeComboBox->currentIndex() == JPEG2000) {
        ui->mxfStereoscopicCheckBox->setEnabled(ENABLED);
        ui->mxfInputStack->setCurrentIndex(PICTURE);
    }
    // MPEG2
    if (ui->mxfSourceTypeComboBox->currentIndex() == MPEG2) {
        ui->mxfStereoscopicCheckBox->setEnabled(DISABLED);
        ui->mxfStereoscopicCheckBox->setChecked(DISABLED);
        ui->mxfTypeComboBox->setCurrentIndex(MXF_INTEROP);
        ui->mxfInputStack->setCurrentIndex(PICTURE);
    }
    // WAV
    if (ui->mxfSourceTypeComboBox->currentIndex() == WAV) {
        ui->mxfInputStack->setCurrentIndex(SOUND);
        mxfSetSoundInputTypeState();
    }

    // update
    mxfSetStereoscopicState();
    mxfSetSlideState();
}

void MainWindow::mxfSetHVState() {
    int value = ui->mxfHVCheckBox->checkState();
    ui->mxfSoundRadio2->setEnabled(!value);
    if (value) {
        ui->mxfSoundRadio5->setChecked(ENABLED);
    }
    mxfSetSoundState();
    ui->aHILabel->setEnabled(value);
    ui->aHIEdit->setEnabled(value);
    ui->aHIButton->setEnabled(value);
    ui->aVILabel->setEnabled(value);
    ui->aVIEdit->setEnabled(value);
    ui->aVIButton->setEnabled(value);
}

void MainWindow::mxfSetStereoscopicState() {
    int value = ui->mxfStereoscopicCheckBox->checkState();
    QStringList stereoScopicFrameRates;
    QStringList standardFrameRates;
    stereoScopicFrameRates << "24" << "25" << "30" << "48";
    standardFrameRates << "24" << "25" << "30" << "48" << "50" << "60" << "96";

    if (value) {
        ui->pictureLeftLabel->setText(tr("Left:"));
        ui->pictureRightLabel->show();
        ui->pictureRightEdit->show();
        ui->pictureRightButton->show();
        ui->mxfFrameRateComboBox->clear();
        ui->mxfFrameRateComboBox->insertItems(1, stereoScopicFrameRates);

    } else {
        if (ui->mxfSourceTypeComboBox->currentIndex() == JPEG2000) {
            ui->pictureLeftLabel->setText(tr("Directory:"));
        } else {
            ui->pictureLeftLabel->setText(tr("M2V:"));
        }
        ui->pictureRightLabel->hide();
        ui->pictureRightEdit->hide();
        ui->pictureRightButton->hide();
        ui->mxfFrameRateComboBox->clear();
        ui->mxfFrameRateComboBox->insertItems(1, standardFrameRates);
    }
}

void MainWindow::mxfSetSound51State(int state)
{
    ui->aCenterLabel->setEnabled(state);
    ui->aCenterEdit->setEnabled(state);
    ui->aCenterButton->setEnabled(state);
    ui->aLeftSLabel->setEnabled(state);
    ui->aLeftSEdit->setEnabled(state);
    ui->aLeftSButton->setEnabled(state);
    ui->aRightSLabel->setEnabled(state);
    ui->aRightSEdit->setEnabled(state);
    ui->aRightSButton->setEnabled(state);
    ui->aSubLabel->setEnabled(state);
    ui->aSubEdit->setEnabled(state);
    ui->aSubButton->setEnabled(state);
}

void MainWindow::mxfSetSound71State(int state)
{
    if (state) {
        mxfSetSound51State(1);
    }

    ui->aLeftCLabel->setEnabled(state);
    ui->aLeftCEdit->setEnabled(state);
    ui->aLeftCButton->setEnabled(state);
    ui->aRightCLabel->setEnabled(state);
    ui->aRightCEdit->setEnabled(state);
    ui->aRightCButton->setEnabled(state);
}

void MainWindow::mxfSetSoundInputTypeState() {
    if (ui->mxfSoundTypeRadioMono->isChecked()) {
        ui->mxfSoundInputStack->setCurrentIndex(WAV_INPUT_MONO);
    } else {
        ui->mxfSoundInputStack->setCurrentIndex(WAV_INPUT_MULTI);
    }
}

void MainWindow::mxfSetSoundState()
{
    mxfSetSound51State(ui->mxfSoundRadio5->isChecked());
    mxfSetSound71State(ui->mxfSoundRadio7->isChecked());
}

int MainWindow::mxfCheckSoundInput20() {
    if (ui->aLeftEdit->text().isEmpty() ||
        ui->aRightEdit->text().isEmpty()) {
        return 1;
    } else {
        return 0;
    }
}

int MainWindow::mxfCheckSoundInput51() {
    if (mxfCheckSoundInput20()) {
        return 1;
    }

    if (ui->aCenterEdit->text().isEmpty() ||
        ui->aSubEdit->text().isEmpty() ||
        ui->aLeftSEdit->text().isEmpty() ||
        ui->aRightSEdit->text().isEmpty()) {
        return 1;
    } else {
        return 0;
    }
}

int MainWindow::mxfCheckSoundInput71() {
    if (mxfCheckSoundInput20() ||
        mxfCheckSoundInput51()) {
        return 1;
    }

    if (ui->aLeftCEdit->text().isEmpty() ||
        ui->aRightCEdit->text().isEmpty()) {
        return 1;
    } else {
        return 0;
    }
}

int MainWindow::checkWavInfo(QFileInfoList filelist, int frameRate) {
    QFileInfo  s;
    wav_info_t wav;
    int        rc;

    foreach(s, filelist) {
        rc = get_wav_info(s.absoluteFilePath().toUtf8().data(), frameRate, &wav);

        if (ui->mxfSoundTypeRadioMono->isChecked()) {
            if (wav.nchannels != 1) {
                return OPENDCP_INVALID_WAV_CHANNELS;
            }
        } else {
            if (wav.nchannels != 2 &&
                wav.nchannels != 6 &&
                wav.nchannels != 8) {
                return OPENDCP_INVALID_WAV_CHANNELS;
            }
        }

        if (wav.bitdepth != 24) {
            return OPENDCP_INVALID_WAV_BITDEPTH;
        }

        if (rc) {
            return rc;
        }
    }

    return OPENDCP_NO_ERROR;
}

void MainWindow::mxfAddInputWavFiles(QFileInfoList *inputList) {
    if (ui->mxfSoundTypeRadioMono->isChecked()) {
        // stereo files
        inputList->append(QFileInfo(ui->aLeftEdit->text()));
        inputList->append(QFileInfo(ui->aRightEdit->text()));

        // add 5.1
        if (ui->mxfSoundRadio5->isChecked() || ui->mxfSoundRadio7->isChecked()) {
            inputList->append(QFileInfo(ui->aCenterEdit->text()));
            inputList->append(QFileInfo(ui->aSubEdit->text()));
            inputList->append(QFileInfo(ui->aLeftSEdit->text()));
            inputList->append(QFileInfo(ui->aRightSEdit->text()));
        }

        if (ui->mxfSoundRadio7->isChecked()) {
            inputList->append(QFileInfo(ui->aLeftCEdit->text()));
            inputList->append(QFileInfo(ui->aRightCEdit->text()));
        }

        // add HI/VI
        if (ui->mxfHVCheckBox->checkState()) {
            inputList->append(QFileInfo(ui->aHIEdit->text()));
            inputList->append(QFileInfo(ui->aVIEdit->text()));
        }
    } else {
        inputList->append(QFileInfo(ui->aMultiEdit->text()));
    }
}

void MainWindow::mxfStart() {
    // JPEG2000
    if(ui->mxfSourceTypeComboBox->currentIndex()== JPEG2000) {
        if (ui->pMxfOutEdit->text().isEmpty()) {
            QMessageBox::critical(this, tr("Destination file needed"),tr("Please select a destination picture MXF file."));
            return;
        }
        if (ui->mxfStereoscopicCheckBox->checkState()) {
            if ((ui->pictureLeftEdit->text().isEmpty() || ui->pictureRightEdit->text().isEmpty())) {
                QMessageBox::critical(this, tr("Source content needed"),tr("Please select left and right source image directories."));
                return;
            }
        } else {
            if ((ui->pictureLeftEdit->text().isEmpty())) {
                QMessageBox::critical(this, tr("Source content needed"),tr("Please select a source image directory."));
                return;
             }
        }
    }

    // MPEG2
    if(ui->mxfSourceTypeComboBox->currentIndex() == MPEG2) {
        if (ui->pMxfOutEdit->text().isEmpty()) {
            QMessageBox::critical(this, tr("Destination file needed"),tr("Please select a destination picture MXF file."));
            return;
        }
        if ((ui->pictureLeftEdit->text().isEmpty())) {
            QMessageBox::critical(this, tr("Source content needed"),tr("Please select a source MPEG2 file."));
            return;
        }
    }

    // WAV
    if (ui->mxfSourceTypeComboBox->currentIndex() == WAV) {
        if (ui->aMxfOutEdit->text().isEmpty()) {
            QMessageBox::critical(this, tr("Destination file needed"),tr("Please select a destination sound MXF file."));
            return;
        }
        if (ui->mxfSoundTypeRadioMono->isChecked()) {
            if (ui->mxfSoundRadio2->isChecked()) {
                if (mxfCheckSoundInput20()) {
                    QMessageBox::critical(this, tr("Source content needed"),tr("Please specify stereo wav files."));
                    return;
                }
            }
            if (ui->mxfSoundRadio5->isChecked()) {
                if (mxfCheckSoundInput51()) {
                    QMessageBox::critical(this, tr("Source content needed"),tr("Please specify 5.1 wav files."));
                        return;
                }
            }
            if (ui->mxfSoundRadio7->isChecked()) {
                if (mxfCheckSoundInput71()) {
                    QMessageBox::critical(this, tr("Source content needed"),tr("Please specify 7.1 wav files."));
                       return;
                }
            }
        } else {
            if (ui->aMultiEdit->text().isEmpty()) {
                QMessageBox::critical(this, tr("Source content needed"),tr("Please select a multi-channel wav file."));
                   return;
            }
        }
    }

    // set log level
    opendcp_log_init(4);

    // create picture mxf file
    if (ui->mxfSourceTypeComboBox->currentIndex() == JPEG2000 || ui->mxfSourceTypeComboBox->currentIndex() == MPEG2) {
        mxfCreatePicture();
    }

    // create sound mxf file
    if (ui->mxfSourceTypeComboBox->currentIndex() == WAV) {
        mxfCreateAudio();
    }
}

void MainWindow::mxfCreateSubtitle() {
    QFileInfoList inputList;
    QString       outputFile;

    if (ui->subInEdit->text().isEmpty()) {
        QMessageBox::critical(this, tr("Source subtitle needed"),tr("Please specify an input subtitle XML file."));
        return;
    }

    if (ui->sMxfOutEdit->text().isEmpty()) {
        QMessageBox::critical(this, tr("Destination file needed"),tr("Please specify a destinaton subtitle MXF file."));
        return;
    }

    opendcp_t *opendcp = opendcp_create();

    opendcp->ns = XML_NS_SMPTE;

    inputList.append(QFileInfo(ui->subInEdit->text()));
    outputFile = ui->sMxfOutEdit->text();

    // copy keys if encrypted, and set flag
    if (ui->mxfEncryptionCheckBox->isChecked()) {
        QString keyId = ui->mxfKeyIdEdit->text().replace("-", "");
        QString keyValue = ui->mxfKeyIdEdit->text();
        memcpy(opendcp->mxf.key_id, QByteArray::fromHex(keyId.toLatin1()), 16);
        memcpy(opendcp->mxf.key_value, QByteArray::fromHex(keyValue.toLatin1()), 16);
        opendcp->mxf.key_flag = 1;
    }

    mxfStartThread(opendcp, inputList, outputFile);

    opendcp_delete(opendcp);

    return;
}

void MainWindow::mxfCreateAudio() {
    QFileInfoList inputList;
    QString       outputFile;

    opendcp_t     *opendcp = opendcp_create();

    if (ui->mxfTypeComboBox->currentIndex() == MXF_INTEROP) {
        opendcp->ns = XML_NS_INTEROP;
    } else {
        opendcp->ns = XML_NS_SMPTE;
    }

    opendcp->frame_rate = ui->mxfFrameRateComboBox->currentText().toInt();
    opendcp->stereoscopic = DISABLED;

    // build input list
    mxfAddInputWavFiles(&inputList);

    if (checkWavInfo(inputList, opendcp->frame_rate)) {
        QMessageBox::critical(this, tr("Invalid Wav Files"),tr("Input WAV files must be 24-bit"));
        return;
    }

    // get wav duration
    opendcp->mxf.duration = get_wav_duration(inputList.at(0).absoluteFilePath().toUtf8().data(),
                                             opendcp->frame_rate);

    outputFile = ui->aMxfOutEdit->text();

    // copy keys if encrypted, and set flag
    if (ui->mxfEncryptionCheckBox->isChecked()) {
        QString keyId = ui->mxfKeyIdEdit->text().replace("-", "");
        QString keyValue = ui->mxfKeyIdEdit->text();
        memcpy(opendcp->mxf.key_id, QByteArray::fromHex(keyId.toLatin1()), 16);
        memcpy(opendcp->mxf.key_value, QByteArray::fromHex(keyValue.toLatin1()), 16);
        opendcp->mxf.key_flag = 1;
    }

    mxfStartThread(opendcp, inputList, outputFile);

    opendcp_delete(opendcp);

    return;
}

void MainWindow::mxfCreatePicture() {
    QDir          pLeftDir;
    QDir          pRightDir;
    QFileInfoList pLeftList;
    QFileInfoList pRightList;
    QFileInfoList inputList;
    QString       outputFile;
    QString       msg;

    opendcp_t *opendcp = opendcp_create();

    if (ui->mxfTypeComboBox->currentIndex() == MXF_INTEROP) {
        opendcp->ns = XML_NS_INTEROP;
    } else {
        opendcp->ns = XML_NS_SMPTE;
    }

    opendcp->frame_rate = ui->mxfFrameRateComboBox->currentText().toInt();
    opendcp->stereoscopic = DISABLED;

    if (ui->mxfSourceTypeComboBox->currentIndex() == JPEG2000) {
        pLeftDir.cd(ui->pictureLeftEdit->text());
        pLeftDir.setNameFilters(QStringList() << "*.j2c");
        pLeftDir.setFilter(QDir::Files | QDir::NoSymLinks);
        pLeftDir.setSorting(QDir::Name);
        pLeftList = pLeftDir.entryInfoList();
        if (checkFileSequence(pLeftList) != OPENDCP_NO_ERROR) {
            goto Done;
        }
    } else {
        pLeftList.append(ui->pictureLeftEdit->text());
    }

    if (ui->mxfStereoscopicCheckBox->checkState()) {
        opendcp->stereoscopic = ENABLED;
        pRightDir.cd(ui->pictureRightEdit->text());
        pRightDir.setNameFilters(QStringList() << "*.j2c");
        pRightDir.setFilter(QDir::Files | QDir::NoSymLinks);
        pRightDir.setSorting(QDir::Name);
        pRightList = pRightDir.entryInfoList();

        if (checkFileSequence(pRightList) != OPENDCP_NO_ERROR) {
            goto Done;
        }

        if (pLeftList.size() != pRightList.size()) {
            QMessageBox::critical(this, tr("File Count Mismatch"),
                                 tr("The left and right image directories have different file counts. They must be the same. Please fix and try again."));
            goto Done;
        }
    }

    for (int x=0;x<pLeftList.size();x++) {
        inputList.append(pLeftList.at(x));
        if (ui->mxfStereoscopicCheckBox->checkState()) {
            inputList.append(pRightList.at(x));
        }
    }

    outputFile = ui->pMxfOutEdit->text();

    if (ui->mxfSlideCheckBox->checkState()) {
        opendcp->mxf.slide = (ui->mxfSlideCheckBox->checkState());
        opendcp->mxf.duration = ui->mxfSlideSpinBox->value() * opendcp->frame_rate * inputList.size();
    } else {
        opendcp->mxf.duration = inputList.size();
    }

    // copy keys if encrypted, and set flag
    if (ui->mxfEncryptionCheckBox->isChecked()) {
        QString keyId = ui->mxfKeyIdEdit->text().replace("-", "");
        QString keyValue = ui->mxfKeyIdEdit->text();
        memcpy(opendcp->mxf.key_id, QByteArray::fromHex(keyId.toLatin1()), 16);
        memcpy(opendcp->mxf.key_value, QByteArray::fromHex(keyValue.toLatin1()), 16);
        opendcp->mxf.key_flag = 1;
    }

    if (inputList.size() < 1) {
        QMessageBox::critical(this, tr("MXF Creation Error"), tr("No input files found."));
        goto Done;
    } else {
        mxfStartThread(opendcp, inputList, outputFile);
    }

Done:

    opendcp_delete(opendcp);

    return;
}

void MainWindow::mxfStartThread(opendcp_t *opendcp, QFileInfoList inputList, QString outputFile)
{
    ConversionDialog *dialog = new ConversionDialog();
    MxfWriter        *writer = new MxfWriter(this);

    connect(writer, SIGNAL(frameDone()),    dialog,  SLOT(update()));
    connect(writer, SIGNAL(setResult(int)), dialog,  SLOT(setResult(int)));
    connect(writer, SIGNAL(finished()),     dialog,  SLOT(finished()));
    connect(dialog, SIGNAL(cancel()),       writer,  SLOT(cancel()));

    writer->init(opendcp, inputList, outputFile);

    dialog->init(opendcp->mxf.duration, 1);
    dialog->setWindowTitle("MXF Conversion");

    writer->start();
    dialog->exec();

    delete dialog;
    delete writer;
}
