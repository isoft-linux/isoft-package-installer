/*
 * Copyright (C) 2015 Leslie Zhai <xiang.zhai@i-soft.com.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "welcomewidget.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QTextBrowser>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QCoreApplication>
#include <QSettings>
#include <QDir>

#include <KLocalizedString>

WelcomeWidget::WelcomeWidget(QWidget *parent, Qt::WindowFlags f)
    : QWidget(parent, f)
{
    QVBoxLayout *vbox = new QVBoxLayout;
    QLabel *label = new QLabel(i18n("Welcome to use iSOFT Package Installer"));
    label->setAlignment(Qt::AlignCenter);
    vbox->addWidget(label);
    QTextBrowser *intro = new QTextBrowser;
    intro->append(i18n("<p style=\"line-height: 150%;\">"
                "iSOFT Package Installer provides easy and simple "
                "software install experience, it supports rpm and deb package. "
                "We have responsibility to remind you the package not built by "
                "iSOFT Linux distribution might not be installed successfully. "
                "Debian's package, will be converted to rpm, but never run its' "
                "post/prescript.</p>"));
    vbox->addWidget(intro);
    QCheckBox *showBox = new QCheckBox(i18n("The next time no longer appears"));
    connect(showBox, &QCheckBox::clicked, [=](){
        QSettings settings(PROJECT_NAME, "settings");
        settings.setValue("greeter/showme", !showBox->isChecked());
    });
    vbox->addWidget(showBox);
    QHBoxLayout *hbox = new QHBoxLayout;
    QPushButton *nextButton = new QPushButton(i18n("Next"));
    nextButton->setMaximumWidth(100);
    connect(nextButton, &QPushButton::clicked, [this](){
        Q_EMIT next();
    });
    QPushButton *cancelButton = new QPushButton(i18n("Cancel"));
    cancelButton->setMaximumWidth(100);
    connect(cancelButton, &QPushButton::clicked, [this](){
        QCoreApplication::quit();
    });
    hbox->addWidget(nextButton);
    hbox->addWidget(cancelButton);
    vbox->addLayout(hbox);
    setLayout(vbox);
}

WelcomeWidget::~WelcomeWidget()
{
}

bool WelcomeWidget::isShowMe() const 
{
    QSettings settings(PROJECT_NAME, "settings");

    QString settingsDir = QDir::homePath() + "/.config/" + PROJECT_NAME;
    QDir dir(settingsDir);
    if (!dir.exists()) {
        dir.mkdir(settingsDir);
        return true;
    }

    QString settingsPath = settingsDir + "/settings.conf";
    QFile file(settingsPath);
    if (!file.exists())
        return true;

    return settings.value("greeter/showme").toBool();
}

#include "moc_welcomewidget.cpp"
