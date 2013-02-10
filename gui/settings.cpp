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

#include <QMessageBox>
#include "settings.h"
#include "translator.h"

Settings::Settings(QWidget *parent) :
    QDialog(parent)
{
    setupUi(this);

    connect(pushOk, SIGNAL(clicked()), this, SLOT(save()));
    connect(pushCancel, SIGNAL(clicked()), this, SLOT(close()));

    QStringList languageList = translator.languageList();

    for (int i = 0; i < languageList.count(); i++) {
        comboLanguages->addItem(languageList[i]);
    }

    load();
}

void Settings::load()
{
    QString language = translator.currentLanguage();
    
    int idx = comboLanguages->findText(language, Qt::MatchExactly);
    comboLanguages->setCurrentIndex(idx);
}
      
void Settings::save()
{
    QSettings settings;

    QString language = comboLanguages->currentText();
    translator.saveSettings(language);

    settings.setValue("user","Terrence Meiczinger");

    close();
}
