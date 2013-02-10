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

#include "generateTitle.h"
#include "ui_generatetitle.h"
#include <QtGui>
#include <QFile>
#include <QTextStream>

GenerateTitle::GenerateTitle(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::GenerateTitle)
{
    ui->setupUi(this);
    load(":naming.xml");

    QDateTime now = QDateTime::currentDateTime();
    ui->dateYearSB->setValue(now.date().year());
    ui->dateMonthSB->setValue(now.date().month());
    ui->dateDaySB->setValue(now.date().day());

    connect(ui->aspectComboBox,           SIGNAL(currentIndexChanged(int)), this, SLOT(updateTitle()));
    connect(ui->audioComboBox,            SIGNAL(currentIndexChanged(int)), this, SLOT(updateTitle()));
    connect(ui->languageAudioComboBox,    SIGNAL(currentIndexChanged(int)), this, SLOT(updateTitle()));
    connect(ui->languageSubtitleComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(updateTitle()));
    connect(ui->narrativeComboBox,        SIGNAL(currentIndexChanged(int)), this, SLOT(updateTitle()));
    connect(ui->ratingComboBox,           SIGNAL(currentIndexChanged(int)), this, SLOT(updateTitle()));
    connect(ui->resolutionComboBox,       SIGNAL(currentIndexChanged(int)), this, SLOT(updateTitle()));
    connect(ui->studioComboBox,           SIGNAL(currentIndexChanged(int)), this, SLOT(updateTitle()));
    connect(ui->territoryComboBox,        SIGNAL(currentIndexChanged(int)), this, SLOT(updateTitle()));
    connect(ui->typeComboBox,             SIGNAL(currentIndexChanged(int)), this, SLOT(updateTitle()));
    connect(ui->stereoscopicComboBox,     SIGNAL(currentIndexChanged(int)), this, SLOT(updateTitle()));
    connect(ui->packageComboBox,          SIGNAL(currentIndexChanged(int)), this, SLOT(updateTitle()));
    connect(ui->filmEdit,                 SIGNAL(textChanged(QString)),     this, SLOT(updateTitle()));
    connect(ui->dateYearSB,               SIGNAL(valueChanged(int)),        this, SLOT(updateTitle()));
    connect(ui->dateMonthSB,              SIGNAL(valueChanged(int)),        this, SLOT(updateTitle()));
    connect(ui->dateDaySB,                SIGNAL(valueChanged(int)),        this, SLOT(updateTitle()));
    connect(ui->facilityComboBox,         SIGNAL(editTextChanged(const QString &)), this, SLOT(updateTitle()));
}

GenerateTitle::~GenerateTitle()
{
    delete ui;
}

QString loadForm()
{
     return "foo";
}

void GenerateTitle::updateTitle()
{
    QString title = getTitle();
    ui->contentEdit->setText(title);
}

QString GenerateTitle::getTitle()
{
    QString text;

    text.append(ui->filmEdit->text().split(" ").first());
    if (ui->typeComboBox->currentIndex()) {
        text.append("_" + ui->typeComboBox->currentText().split(" ").first());
    }
    if (ui->aspectComboBox->currentIndex()) {
        text.append("_" + ui->aspectComboBox->currentText().split(" ").first());
    }
    if (ui->languageAudioComboBox->currentIndex()) {
        text.append("_" + ui->languageAudioComboBox->currentText().split(" ").first());
        if (ui->languageSubtitleComboBox->currentIndex() == 0) {
            text.append("-XX");
        } else {
            text.append("-" + ui->languageSubtitleComboBox->currentText().split(" ").first());
        }
    }
    if (ui->territoryComboBox->currentIndex()) {
        text.append("_" + ui->territoryComboBox->currentText().split(" ").first());
    }
    if (ui->ratingComboBox->currentIndex()) {
        text.append("-" + ui->ratingComboBox->currentText().split(" ").first());
    }
    if (ui->audioComboBox->currentIndex()) {
        text.append("_" + ui->audioComboBox->currentText().split(" ").first());
        if (ui->narrativeComboBox->currentIndex()) {
            text.append("-" + ui->narrativeComboBox->currentText().split(" ").first());
        }
    }
    if (ui->resolutionComboBox->currentIndex()) {
        text.append("_" + ui->resolutionComboBox->currentText().split(" ").first());
    }
    if (ui->studioComboBox->currentIndex()) {
        text.append("_" + ui->studioComboBox->currentText().split(" ").first());
    }
    text.append("_" + ui->dateYearSB->text() + QString().sprintf("%02d%02d",ui->dateMonthSB->text().toInt(),ui->dateDaySB->text().toInt()));
    if (ui->facilityComboBox->currentIndex()) {
        text.append("_" + ui->facilityComboBox->currentText().split(" ").first().left(3));
    }
    if (ui->stereoscopicComboBox->currentIndex()) {
        text.append("_" + ui->stereoscopicComboBox->currentText().split(" ").first());
    }
    if (ui->packageComboBox->currentIndex()) {
        text.append("_" + ui->packageComboBox->currentText().split(" ").first());
    }

    return text;
}

void GenerateTitle::load(const QString& filename)
{
    QFile file( filename );
    QString line;
    int inContent    = 0;
    int inAspect     = 0;
    int inLanguage   = 0;
    int inTerritory  = 0;
    int inRating     = 0;
    int inAudio      = 0;
    int inResolution = 0;
    int inStudio     = 0;
    int inFacility   = 0;
    int in3d         = 0;
    int inPackage    = 0;

    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "could not open file: " << filename;
        return;
    }

    QTextStream ts( &file );
    while ( !ts.atEnd() ) {
        line = ts.readLine();
        if (line == "<content>") {
            inContent = 1;
        } else if (line == "</content>") {
            inContent = 0;
        } else if (line == "<aspect>") {
            inAspect = 1;
        } else if (line == "</aspect>") {
            inAspect = 0;
        } else if (line == "<language>") {
            inLanguage = 1;
        } else if (line == "</language>") {
            inLanguage = 0;
        } else if (line == "<territory>") {
            inTerritory = 1;
        } else if (line == "</territory>") {
            inTerritory = 0;
        } else if (line == "<rating>") {
            inRating = 1;
        } else if (line == "</rating>") {
            inRating = 0;
        } else if (line == "<audio>") {
            inAudio = 1;
        } else if (line == "</audio>") {
            inAudio = 0;
        } else if (line == "<resolution>") {
            inResolution = 1;
        } else if (line == "</resolution>") {
            inResolution = 0;
        } else if (line == "<studio>") {
            inStudio = 1;
        } else if (line == "</studio>") {
            inStudio = 0;
        } else if (line == "<facility>") {
            inFacility = 1;
        } else if (line == "</facility>") {
            inFacility = 0;
        } else if (line == "<3d>") {
            in3d = 1;
        } else if (line == "</3d>") {
            in3d = 0;
        } else if (line == "<package>") {
            inPackage = 1;
        } else if (line == "</package>") {
            inPackage = 0;
        } else {
            if (inContent) {
                ui->typeComboBox->addItem(line.trimmed());
            } else if (inAspect) {
                    ui->aspectComboBox->addItem(line.trimmed());
            } else if (inLanguage) {
                    ui->languageAudioComboBox->addItem(line.trimmed());
                    ui->languageSubtitleComboBox->addItem(line.trimmed());
                    ui->narrativeComboBox->addItem(line.trimmed());
            } else if (inTerritory) {
                    ui->territoryComboBox->addItem(line.trimmed());
            } else if (inRating) {
                    ui->ratingComboBox->addItem((line.trimmed()));
            } else if (inAudio) {
                    ui->audioComboBox->addItem(line.trimmed());
            } else if (inResolution) {
                    ui->resolutionComboBox->addItem(line.trimmed());
            } else if (inStudio) {
                    ui->studioComboBox->addItem(line.trimmed());
            } else if (in3d) {
                    ui->stereoscopicComboBox->addItem(line.trimmed());
            } else if (inPackage) {
                    ui->packageComboBox->addItem(line.trimmed());
            } else if (inFacility) {
                    ui->facilityComboBox->addItem(line.trimmed());
            }
        }
    }

    file.close();
}
