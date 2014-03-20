/*
     OpenDCP: Builds Digital Cinema Packages
     Copyright (c) 2010-2012 Terrence Meiczinger, All Rights Reserved

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
#include "conversion_dialog.h"
#include <QtGui>
#include <QDir>
#include <QPixmap>
#include <stdio.h>
#include <opendcp.h>
#include <opendcp_encoder.h>

enum J2K_STATE {
    DISABLED = 0,
    ENABLED  = 1
};

void MainWindow::j2kConnectSlots()
{
    QSignalMapper *j2kInSignalMapper = new QSignalMapper(this);

    // connect slots
    connect(ui->stereoscopicCheckBox, SIGNAL(stateChanged(int)),        this, SLOT(j2kSetStereoscopicState()));
    connect(ui->bwSlider,             SIGNAL(valueChanged(int)),        this, SLOT(j2kBwSliderUpdate()));
    connect(ui->encodeButton,         SIGNAL(clicked()),                this, SLOT(j2kStart()));
    connect(ui->profileComboBox,      SIGNAL(currentIndexChanged(int)), this, SLOT(j2kCinemaProfileUpdate()));
    connect(ui->xyzCheckBox,          SIGNAL(stateChanged(int)),        this, SLOT(j2kSetXyzState()));

    // connect j2k input signals
    j2kInSignalMapper->setMapping(ui->inImageLeftButton,  ui->inImageLeftEdit);
    j2kInSignalMapper->setMapping(ui->inImageRightButton, ui->inImageRightEdit);

    connect(ui->inImageLeftButton,  SIGNAL(clicked()), j2kInSignalMapper, SLOT(map()));
    connect(ui->inImageRightButton, SIGNAL(clicked()), j2kInSignalMapper, SLOT(map()));

    connect(j2kInSignalMapper, SIGNAL(mapped(QWidget*)),this, SLOT(j2kInputSlot(QWidget*)));

    // connect j2k output signals
    signalMapper.setMapping(ui->outJ2kLeftButton,   ui->outJ2kLeftEdit);
    signalMapper.setMapping(ui->outJ2kRightButton,  ui->outJ2kRightEdit);

    connect(ui->outJ2kLeftButton,   SIGNAL(clicked()), &signalMapper, SLOT(map()));
    connect(ui->outJ2kRightButton,  SIGNAL(clicked()), &signalMapper, SLOT(map()));

    // update file
    connect(ui->endSpinBox,       SIGNAL(valueChanged(int)),    this,SLOT(j2kUpdateEndSpinBox()));
}

void MainWindow::j2kInputSlot(QWidget *w)
{
    QString path;
    QString filter = "*.tif;*.tiff;*.dpx;*.bmp;*.j2c;*.jp2;*.j2k";

    path = QFileDialog::getExistingDirectory(this, tr("Choose a directory of images"), lastDir);

    if (path == NULL) {
        return;
    }

    lastDir = path;

    QDir inDir(path);
    inDir.setFilter(QDir::Files | QDir::NoSymLinks);
    inDir.setNameFilters(filter.split(';'));
    inDir.setSorting(QDir::Name);
    QFileInfoList fileList = inDir.entryInfoList();

    if (fileList.size() < 1) {
        QMessageBox::warning(this, tr("No image files found"),
                                   tr("No valid image files were found in the selected directory"));
        return;
    }

    if (checkFileSequence(fileList) == OPENDCP_ERROR) {
       return;
    }

    ui->endSpinBox->setMaximum(fileList.size());
    ui->endSpinBox->setValue(fileList.size());
    ui->startSpinBox->setMaximum(ui->endSpinBox->value());

    w->setProperty("text", path);

    preview(fileList.at(0).absoluteFilePath());
}

void MainWindow::j2kSetStereoscopicState() {
    int value = ui->stereoscopicCheckBox->checkState();

    if (value) {
        ui->inImageLeft->setText(tr("Left:"));
        ui->outJ2kLeft->setText(tr("Left:"));
        ui->inImageRight->show();
        ui->inImageRightEdit->show();
        ui->inImageRightButton->show();
        ui->outJ2kRight->show();
        ui->outJ2kRightEdit->show();
        ui->outJ2kRightButton->show();

        ui->bwSlider->setValue(250);
    } else {
        ui->inImageLeft->setText(tr("Directory:"));
        ui->outJ2kLeft->setText(tr("Directory:"));
        ui->inImageRight->hide();
        ui->inImageRightEdit->hide();
        ui->inImageRightButton->hide();
        ui->outJ2kRight->hide();
        ui->outJ2kRightEdit->hide();
        ui->outJ2kRightButton->hide();
        ui->bwSlider->setValue(125);
    }
}

void MainWindow::j2kSetXyzState() {
    int value = ui->xyzCheckBox->checkState();
    
    if (value) {
        ui->colorComboBox->setEnabled(ENABLED);
    } else {
        ui->colorComboBox->setEnabled(DISABLED);
    }
}

void MainWindow::j2kCinemaProfileUpdate() {
    return;
}

void MainWindow::j2kBwSliderUpdate() {
    QString bwValueLabel;

    int bw = ui->bwSlider->value();

    bwValueLabel.sprintf("%d mb/s",bw);
    ui->bwValueLabel->setText(bwValueLabel);
}

// globals for threads
opendcp_t *context;
int iterations = 0;

void MainWindow::preview(QString filename)
{
    QImage image;

    if (!image.load(filename)) {
        ui->previewLabel->setText(tr("Image preview not supported for this file"));
    } else {
        QPixmap pixmap(QPixmap::fromImage(image).scaled(ui->previewLabel->size(), Qt::KeepAspectRatio));
        ui->previewLabel->setPixmap(pixmap);
    }
}

int j2kEncode(QStringList pair) {
    int rc = convert_to_j2k(context,pair.at(0).toAscii().data(),pair.at(1).toAscii().data());

    return rc;
}

void MainWindow::j2kConvert() {
    int threadCount = 0;
    QString detailText;
    QString inFile;
    QString outFile;
    QList<QStringList> list;
    QFileInfoList inLeftList;
    QFileInfoList inRightList;

    // reset iterations
    iterations = 0;

    // set thread limit
    QThreadPool::globalInstance()->setMaxThreadCount(ui->threadsSpinBox->value());

    QString outLeftDir = ui->outJ2kLeftEdit->text();
    QString outRightDir = ui->outJ2kRightEdit->text();

    QString filter = "*.tif;*.tiff;*.dpx;*.bmp;*.j2c;*.j2k;*.jp2";
    QDir inDir(ui->inImageLeftEdit->text());
    inDir.setFilter(QDir::Files | QDir::NoSymLinks);
    inDir.setNameFilters(filter.split(';'));
    inDir.setSorting(QDir::Name);
    inLeftList = inDir.entryInfoList();

    if (context->stereoscopic) {
        QDir inDir(ui->inImageRightEdit->text());
        inDir.setFilter(QDir::Files | QDir::NoSymLinks);
        inDir.setNameFilters(filter.split(';'));
        inDir.setSorting(QDir::Name);
        inRightList = inDir.entryInfoList();
    }

    // build conversion list
    for (int i = ui->startSpinBox->value() - 1; i < ui->endSpinBox->value(); i++) {
        QStringList pair;

        inFile  = inLeftList.at(i).absoluteFilePath();
        outFile = outLeftDir % "/" % inLeftList.at(i).completeBaseName() % ".j2c";
        pair << inFile << outFile;

        if (is_filename_ascii(inFile.toUtf8().data()) == 0) {
            QString message = tr("Unicode is not supported. Filenames must contain only ASCII characters. File: ") + inFile.toUtf8().data();
            QMessageBox::critical(this, tr("Invalid Characters in filename"), message);
            return;
        }

        if (!QFileInfo(outFile).exists() || context->j2k.no_overwrite == 0) {
            list.append(pair);
            iterations++;
        }

        if (context->stereoscopic) {
            pair.clear();
            inFile  = inRightList.at(i).absoluteFilePath();
            outFile = outRightDir % "/" % inRightList.at(i).completeBaseName() % ".j2c";
            pair << inFile << outFile ;

            if (!QFileInfo(outFile).exists() || context->j2k.no_overwrite == 0) {
                list.append(pair);
                iterations++;
            }
        }
    }

    if (iterations < 1) {
        QMessageBox::warning(this, tr("No images to encode"),
                                   tr("No images need to be encoded"));
        return;
    }

    threadCount = QThreadPool::globalInstance()->maxThreadCount();

    ConversionDialog *dialog = new ConversionDialog();
    dialog->init(iterations, threadCount);

    // Create a QFutureWatcher and conncect signals and slots.
    QFutureWatcher<int>  futureWatcher;
    QFuture<int>         result = QtConcurrent::mapped(list, j2kEncode);
    QObject::connect(dialog,         SIGNAL(cancel()), &futureWatcher, SLOT(cancel()));
    QObject::connect(&futureWatcher, SIGNAL(progressValueChanged(int)), dialog, SLOT(update()));
    QObject::connect(&futureWatcher, SIGNAL(finished()), dialog, SLOT(finished()));

    // Start the computation
    futureWatcher.setFuture(result);

    // open conversion dialog box
    dialog->exec();

    // wait to ensure all threads are finished
    futureWatcher.waitForFinished();

    for (int i = 0; i < result.resultCount(); i++) {
        if (futureWatcher.resultAt(i)) {
            QFileInfo filename = list.at(i).at(0);
            detailText.append(filename.fileName());
            detailText.append("\n");
        }
    }

    if (!detailText.isEmpty()) {
        QMessageBox msgBox;
        msgBox.setText(tr("JPEG2000 Encoding Failure"));
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setInformativeText(tr("Some files were unable to be encoded. Click 'show details' for a list.\n"));
        msgBox.setDetailedText(detailText);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();
    }

    delete dialog;

    return;
}

void MainWindow::j2kUpdateEndSpinBox() {
    ui->startSpinBox->setMaximum(ui->endSpinBox->value());
}

void MainWindow::processOptions(opendcp_t *opendcp) {
    opendcp->log_level = 0;

    opendcp_log_init(opendcp->log_level);

    if (ui->profileComboBox->currentIndex() == 0) {
        opendcp->cinema_profile = DCP_CINEMA2K;
    } else {
        opendcp->cinema_profile = DCP_CINEMA4K;
    }

    int index = ui->encoderComboBox->currentIndex();
    opendcp->j2k.encoder = ui->encoderComboBox->itemData(index).toInt();

    opendcp->j2k.lut    = ui->colorComboBox->currentIndex();
    opendcp->j2k.resize = ui->resizeComboBox->currentIndex();

    if (ui->dpxLogCheckBox->checkState()) {
        opendcp->j2k.dpx = 1;
    } else {
        opendcp->j2k.dpx = 0;
    }

    if (ui->stereoscopicCheckBox->checkState()) {
        opendcp->stereoscopic = 1;
    } else {
        opendcp->stereoscopic = 0;
    }

    if (ui->xyzCheckBox->checkState()) {
        opendcp->j2k.xyz = 1;
    } else {
        opendcp->j2k.xyz = 0;
    }

    if (ui->overwritej2kCB->checkState())
        opendcp->j2k.no_overwrite = 0;
    else {
        opendcp->j2k.no_overwrite = 1;
    }

    opendcp->frame_rate = ui->frameRateComboBox->currentText().toInt();
    opendcp->j2k.bw = ui->bwSlider->value() * 1000000;
}

void MainWindow::j2kStart() {
    // create opendcp context
    context = opendcp_create();


    // get all options
    processOptions(context);

    // validate destination directories
    if (context->stereoscopic) {
        if (ui->inImageLeftEdit->text().isEmpty() || ui->inImageRightEdit->text().isEmpty()) {
            QMessageBox::warning(this, tr("Source Directories Needed"),tr("Please select source directories"));
            goto Done;
        }
        if (ui->outJ2kLeftEdit->text().isEmpty() || ui->outJ2kRightEdit->text().isEmpty()) {
            QMessageBox::warning(this, tr("Destination Directories Needed"),tr("Please select destination directories"));
            goto Done;
        }
    } else {
        if (ui->inImageLeftEdit->text().isEmpty()) {
            QMessageBox::warning(this, tr("Source Directory Needed"),tr("Please select a source directory"));
            goto Done;
        }
        if (ui->outJ2kLeftEdit->text().isEmpty()) {
            QMessageBox::warning(this, tr("Destination Directory Needed"),tr("Please select a destination directory"));
            goto Done;
        }
    }

    // check left and right files are equal
    /*
    if (context->stereoscopic) {
        if (inLeftList.size() != inRightList.size()) {
            QMessageBox::critical(this, tr("File Count Mismatch"),
                                 tr("The left and right image directories have different file counts. They must be the same. Please fix and try again."));
            goto Done;
        }
    }

    // check images
    if (context->j2k.resize == SAMPLE_NONE) {
        if (check_image_compliance(context->cinema_profile, NULL, inLeftList.at(0).absoluteFilePath().toAscii().data()) != OPENDCP_NO_ERROR) {
            QMessageBox::warning(this, tr("Invalid DCI Resolution"),
                                 tr("Images are not DCI compliant, select DCI resize to automatically resize or supply DCI compliant images"));
            return;
        }
    }
    */

    j2kConvert();

Done:
    opendcp_delete(context);
}
