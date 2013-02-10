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

#ifndef TRANSLATOR_H
#define TRANSLATOR_H

#include <QSettings>
#include <QTranslator>
#include <QMainWindow>
#include <QtGui>

class Translator : public QObject
{
    Q_OBJECT

public:
    Translator();

    QTranslator *Qtranslation();
    void        loadSettings(void);
    void        saveSettings(QString language);
    void        loadLanguages(void);
    QStringList languageList(void);
    QString     filenameToLanguage(const QString &filename);
    QString     currentLanguage();
    void        setCurrentLanguage(const QString &language);

protected:
    void loadLanguage(const QString& rLanguage);

private:
    
    void switchTranslator(QTranslator& translator, const QString& filename);

    QHash<QString, QString> m_langHash; /* language code hash */

    QTranslator     m_translator;       /* contains the translations for this application */
    QTranslator     m_translatorQt;     /* contains the translations for qt */
    QString         m_currLang;         /* contains the currently loaded language */
    QString         m_langPath;         /* path of language files. This is always fixed to /translation. */
    QString         m_langFile;         /* current language file */
};

#endif // TRANSLATOR_H
