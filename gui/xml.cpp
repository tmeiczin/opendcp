/*
     : Builds Digital Cinema Packages
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
#include "generate_title.h"
#include <QtGui>
#include <QDir>
#include <stdio.h>
#include <stdlib.h>
#include <opendcp.h>

asset_t pictureAsset;
asset_t soundAsset;
asset_t subtitleAsset;

enum ASSET_INDEX {
    PICTURE = 0,
    SOUND,
    SUBTITLE
};

void MainWindow::connectXmlSlots()
{
    // connect slots
    connect(ui->reelPictureButton,         SIGNAL(clicked()),         this, SLOT(setPictureTrack()));
    connect(ui->reelSoundButton,           SIGNAL(clicked()),         this, SLOT(setSoundTrack()));
    connect(ui->reelSubtitleButton,        SIGNAL(clicked()),         this, SLOT(setSubtitleTrack()));
    connect(ui->reelPictureOffsetSpinBox,  SIGNAL(valueChanged(int)), this, SLOT(updatePictureDuration()));
    connect(ui->reelSubtitleOffsetSpinBox, SIGNAL(valueChanged(int)), this, SLOT(updateSubtitleDuration()));
    connect(ui->reelSoundOffsetSpinBox,    SIGNAL(valueChanged(int)), this, SLOT(updateSoundDuration()));
    connect(ui->createDcpButton,           SIGNAL(clicked()),         this, SLOT(startDcp()));
    connect(ui->cplTitleGenButton,         SIGNAL(clicked()),         this, SLOT(getTitle()));
}

void MainWindow::getTitle() {
    if (generateTitle->exec()) {
        QString title = generateTitle->getTitle();
        ui->cplTitleEdit->setText(title);
    }
}

int MainWindow::mxfCopy(QString source, QString destination) {
    QFileInfo   sourceFileInfo;
    QFileInfo   destinationFileInfo;

    if (source == NULL) {
        return OPENDCP_NO_ERROR;
    }


    // set source/destination files
    sourceFileInfo.setFile(source);
    destinationFileInfo.setFile(destination + "/" + sourceFileInfo.fileName());

    // no need to copy if the source/destination are the same
    if (sourceFileInfo.absoluteFilePath() == destinationFileInfo.absoluteFilePath()) {
        return OPENDCP_NO_ERROR;
    }

    // check if file exists
    if (destinationFileInfo.isFile()) {
        if (!ui->overwriteMxfCB->isChecked()) {
            return OPENDCP_NO_ERROR;
        }
        QFile::remove(destinationFileInfo.absoluteFilePath());
    }

    // copy/move the file
    if (ui->rbMoveMxf->isChecked()) {
        QFile::rename(sourceFileInfo.absoluteFilePath(), destinationFileInfo.absoluteFilePath());
    } else {
        fileCopy(sourceFileInfo.absoluteFilePath(), destinationFileInfo.absoluteFilePath());
    }

    return OPENDCP_NO_ERROR;
}

void MainWindow::startDcp()
{
    QString     path;
    QMessageBox msgBox;
    QString     DCP_FAIL_MSG;
    QString     message; 
    int         rc;

    // get dcp destination directory
    path = QFileDialog::getExistingDirectory(this, tr("Choose destination folder"), lastDir);

    if (path.isEmpty()) {
        return;
    }

    lastDir = path;

    opendcp_t *xmlContext = opendcp_create();

    DCP_FAIL_MSG = tr("DCP Creation Failed");

    // process options
    xmlContext->log_level = 0;

    if (ui->digitalSignatureCheckBox->checkState()) {
        xmlContext->xml_signature.sign = 1;
        xmlContext->xml_signature.use_external = 0;
    }

    opendcp_log_init(xmlContext->log_level);
    xmlContext->ns = XML_NS_UNKNOWN;

    // set XML attribues
    snprintf(xmlContext->dcp.title, sizeof(xmlContext->dcp.title), "%s", ui->cplTitleEdit->text().toUtf8().constData());
    snprintf(xmlContext->dcp.annotation, sizeof(xmlContext->dcp.annotation), "%s", ui->cplAnnotationEdit->text().toUtf8().constData());
    snprintf(xmlContext->dcp.issuer, sizeof(xmlContext->dcp.issuer), "%s", ui->cplIssuerEdit->text().toUtf8().constData());
    snprintf(xmlContext->dcp.kind, sizeof(xmlContext->dcp.kind), "%s", ui->cplKindComboBox->currentText().toUtf8().constData());
    snprintf(xmlContext->dcp.rating, sizeof(xmlContext->dcp.rating), "%s", ui->cplRatingComboBox->currentText().toUtf8().constData());

    // check picture track is supplied
    if (ui->reelPictureEdit->text().isEmpty()) {
        QMessageBox::critical(this, DCP_FAIL_MSG,
                             tr("An MXF picture track is required."));
        goto Done;
    }

    // check durations
    if ((!ui->reelSoundEdit->text().isEmpty() && ui->reelPictureDurationSpinBox->value() != ui->reelSoundDurationSpinBox->value()) ||
        (!ui->reelSubtitleEdit->text().isEmpty() && ui->reelPictureDurationSpinBox->value() != ui->reelSubtitleDurationSpinBox->value())) {
        QMessageBox::critical(this, DCP_FAIL_MSG,
                              tr("The duration of all MXF tracks must be the same."));
        goto Done;
    }

    // add pkl to the DCP (only one PKL currently support)
    pkl_t pkl;
    create_pkl(xmlContext->dcp, &pkl);
    add_pkl_to_dcp(&xmlContext->dcp, pkl);

    // add cpl to the DCP/PKL (only one CPL currently support)
    cpl_t cpl;
    create_cpl(xmlContext->dcp, &cpl);
    add_cpl_to_pkl(&xmlContext->dcp.pkl[0], cpl);

    // add reel
    reel_t reel;
    create_reel(xmlContext->dcp, &reel);
    add_reel_to_cpl(&xmlContext->dcp.pkl[0].cpl[0], reel);

    // add assets
    if (!ui->reelPictureEdit->text().isEmpty()) {
        asset_t asset;
        add_asset(xmlContext, &asset, ui->reelPictureEdit->text().toUtf8().data());
        QString digest = calculateDigest(xmlContext, "Picture", QString::fromStdString(asset.filename));
        if (digest == NULL) {
            QMessageBox::critical(this, DCP_FAIL_MSG,
                                 tr("Calculate Digest Did Not Complete"));
            goto Done;
        }
        //QString digest = xmlCalculateDigestStartThread(xmlContext, asset.filename);
        snprintf(asset.digest, sizeof(asset.digest),"%s", digest.toUtf8().data());
        add_asset_to_reel(xmlContext, &xmlContext->dcp.pkl[0].cpl[0].reel[0], asset);
    }

    if (!ui->reelSoundEdit->text().isEmpty()) {
        asset_t asset;
        add_asset(xmlContext, &asset, ui->reelSoundEdit->text().toUtf8().data());
        QString digest = calculateDigest(xmlContext, "Sound", QString::fromStdString(asset.filename));
        if (digest == NULL) {
            QMessageBox::critical(this, DCP_FAIL_MSG,
                                 tr("Calculate Digest Did Not Complete"));
            goto Done;
        }
        snprintf(asset.digest, sizeof(asset.digest), "%s", digest.toUtf8().data());
        add_asset_to_reel(xmlContext, &xmlContext->dcp.pkl[0].cpl[0].reel[0], asset);
    }

    if (!ui->reelSubtitleEdit->text().isEmpty()) {
        asset_t asset;
        add_asset(xmlContext, &asset, ui->reelSubtitleEdit->text().toUtf8().data());
        QString digest = calculateDigest(xmlContext, "Subtitle", QString::fromStdString(asset.filename));
        if (digest == NULL) {
            QMessageBox::critical(this, DCP_FAIL_MSG,
                                 tr("Calculate Digest Did Not Complete"));
            goto Done;
        }
        snprintf(asset.digest, sizeof(asset.digest), "%s", digest.toUtf8().data());
        add_asset_to_reel(xmlContext, &xmlContext->dcp.pkl[0].cpl[0].reel[0], asset);
    }

    // adjust durations
    xmlContext->dcp.pkl[0].cpl[0].reel[0].main_picture.duration     = ui->reelPictureDurationSpinBox->value();
    xmlContext->dcp.pkl[0].cpl[0].reel[0].main_picture.entry_point  = ui->reelPictureOffsetSpinBox->value();
    xmlContext->dcp.pkl[0].cpl[0].reel[0].main_sound.duration       = ui->reelSoundDurationSpinBox->value();
    xmlContext->dcp.pkl[0].cpl[0].reel[0].main_sound.entry_point    = ui->reelSoundOffsetSpinBox->value();
    xmlContext->dcp.pkl[0].cpl[0].reel[0].main_subtitle.duration    = ui->reelSubtitleDurationSpinBox->value();
    xmlContext->dcp.pkl[0].cpl[0].reel[0].main_subtitle.entry_point = ui->reelSubtitleOffsetSpinBox->value();

    rc = validate_reel(xmlContext, &xmlContext->dcp.pkl[0].cpl[0].reel[0], 0);
    if (rc) {
        message = tr(OPENDCP_ERROR_STRING[rc]);
        QMessageBox::critical(this, DCP_FAIL_MSG, message);
        goto Done;
    }

    snprintf(xmlContext->dcp.pkl[0].cpl[0].filename, sizeof(xmlContext->dcp.pkl[0].cpl[0].filename), "%s/CPL_%s.xml", path.toUtf8().constData(), xmlContext->dcp.pkl[0].cpl[0].uuid);
    snprintf(xmlContext->dcp.pkl[0].filename, sizeof(xmlContext->dcp.pkl[0].filename), "%s/PKL_%s.xml", path.toUtf8().constData(), xmlContext->dcp.pkl[0].uuid);

    if (xmlContext->ns == XML_NS_SMPTE) {
        snprintf(xmlContext->dcp.assetmap.filename, sizeof(xmlContext->dcp.assetmap.filename), "%s/ASSETMAP.xml",path.toUtf8().constData());
        snprintf(xmlContext->dcp.volindex.filename, sizeof(xmlContext->dcp.volindex.filename), "%s/VOLINDEX.xml",path.toUtf8().constData());
    } else {
        snprintf(xmlContext->dcp.assetmap.filename, sizeof(xmlContext->dcp.assetmap.filename), "%s/ASSETMAP",path.toUtf8().constData());
        snprintf(xmlContext->dcp.volindex.filename, sizeof(xmlContext->dcp.volindex.filename), "%s/VOLINDEX",path.toUtf8().constData());
    }

    // write XML Files
    if (write_cpl(xmlContext,&xmlContext->dcp.pkl[0].cpl[0]) != OPENDCP_NO_ERROR) {
        QMessageBox::critical(this, DCP_FAIL_MSG, tr("Failed to create composition playlist."));
        goto Done;
    }
    if (write_pkl(xmlContext,&xmlContext->dcp.pkl[0]) != OPENDCP_NO_ERROR) {
        QMessageBox::critical(this, DCP_FAIL_MSG, tr("Failed to create packaging list."));
        goto Done;
    }
    if (write_volumeindex(xmlContext) != OPENDCP_NO_ERROR) {
        QMessageBox::critical(this, DCP_FAIL_MSG, tr("Failed to create volume index."));
        goto Done;
    }
    if (write_assetmap(xmlContext) != OPENDCP_NO_ERROR) {
        QMessageBox::critical(this, DCP_FAIL_MSG, tr("Failed to create assetmap."));
        goto Done;
    }

    // copy picture mxf
    mxfCopy(QString::fromUtf8(xmlContext->dcp.pkl[0].cpl[0].reel[0].main_picture.filename), path);

    // copy audio mxf
    mxfCopy(QString::fromUtf8(xmlContext->dcp.pkl[0].cpl[0].reel[0].main_sound.filename), path);

    // copy subtitle mxf
    mxfCopy(QString::fromUtf8(xmlContext->dcp.pkl[0].cpl[0].reel[0].main_subtitle.filename), path);

    msgBox.setText("DCP Created successfully");
    msgBox.exec();

Done:
    opendcp_delete(xmlContext);

    return;
}

void MainWindow::updatePictureDuration()
{
    int offset = ui->reelPictureOffsetSpinBox->value();
    ui->reelPictureDurationSpinBox->setMaximum(pictureAsset.intrinsic_duration-offset);
}

void MainWindow::updateSoundDuration()
{
    int offset = ui->reelSoundOffsetSpinBox->value();
    ui->reelSoundDurationSpinBox->setMaximum(soundAsset.intrinsic_duration-offset);
}

void MainWindow::updateSubtitleDuration()
{
    int offset = ui->reelSubtitleOffsetSpinBox->value();
    ui->reelSubtitleDurationSpinBox->setMaximum(subtitleAsset.intrinsic_duration-offset);
}

void MainWindow::setPictureTrack()
{
    QString path;
    QString filter = "*.mxf";

    path = QFileDialog::getOpenFileName(this, tr("Choose a file to open"), lastDir, filter);

    if (path.isEmpty()) {
        return;
    }

    QFileInfo fi(path);
    lastDir = fi.absolutePath();

    if (get_file_essence_class(path.toUtf8().data(), 0) != ACT_PICTURE) {
        QMessageBox::critical(this, tr("Not a Picture Track"),
                              tr("The selected file is not a valid MXF picture track."));
    } else {
        ui->reelPictureEdit->setProperty("text", path);
        snprintf(pictureAsset.filename, sizeof(pictureAsset.filename), "%s", ui->reelPictureEdit->text().toUtf8().constData());
        read_asset_info(&pictureAsset);
        ui->reelPictureDurationSpinBox->setMaximum(pictureAsset.intrinsic_duration);
        ui->reelPictureDurationSpinBox->setValue(pictureAsset.duration);
        ui->reelPictureOffsetSpinBox->setMaximum(pictureAsset.intrinsic_duration-1);
        ui->reelPictureOffsetSpinBox->setValue(0);
    }

    return;
}

void MainWindow::setSoundTrack()
{
    QString path;
    QString filter = "*.mxf";

    path = QFileDialog::getOpenFileName(this, tr("Choose a file to open"), lastDir, filter);

    if (path.isEmpty()) {
        return;
    }

    QFileInfo fi(path);
    lastDir = fi.absolutePath();

    if (get_file_essence_class(path.toUtf8().data(), 0) != ACT_SOUND) {
        QMessageBox::critical(this, tr("Not a Sound Track"),
                             tr("The selected file is not a valid MXF sound track."));
    } else {
        ui->reelSoundEdit->setProperty("text", path);
        snprintf(soundAsset.filename, sizeof(soundAsset.filename), "%s", ui->reelSoundEdit->text().toUtf8().constData());
        read_asset_info(&soundAsset);
        ui->reelSoundDurationSpinBox->setMaximum(soundAsset.intrinsic_duration);
        ui->reelSoundDurationSpinBox->setValue(soundAsset.duration);
        ui->reelSoundOffsetSpinBox->setMaximum(soundAsset.intrinsic_duration-1);
        ui->reelSoundOffsetSpinBox->setValue(0);
    }

    return;
}

void MainWindow::setSubtitleTrack()
{
    QString path;
    QString filter = "*.mxf";

    path = QFileDialog::getOpenFileName(this, tr("Choose an file to open"), lastDir, filter);

    if (path.isEmpty()) {
        return;
    }

    QFileInfo fi(path);
    lastDir = fi.absolutePath();

    if (get_file_essence_class(path.toUtf8().data(), 0) != ACT_TIMED_TEXT) {
        QMessageBox::critical(this, tr("Not a Subtitle Track"),
                              tr("The selected file is not a valid MXF subtitle track."));
    } else {
        ui->reelSubtitleEdit->setProperty("text", path);
        snprintf(subtitleAsset.filename, sizeof(subtitleAsset.filename), "%s", ui->reelSubtitleEdit->text().toUtf8().constData());
        read_asset_info(&subtitleAsset);
        ui->reelSubtitleDurationSpinBox->setMaximum(subtitleAsset.intrinsic_duration);
        ui->reelSubtitleDurationSpinBox->setValue(subtitleAsset.duration);
        ui->reelSubtitleOffsetSpinBox->setMaximum(subtitleAsset.intrinsic_duration-1);
        ui->reelSubtitleOffsetSpinBox->setValue(0);
    }

    return;
}
