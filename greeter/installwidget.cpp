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

#include "installwidget.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QTextBrowser>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QCoreApplication>
#include <QProgressBar>

#include <KLocalizedString>

InstallWidget::InstallWidget(org::isoftlinux::Install *installer, 
                             QWidget *parent, 
                             Qt::WindowFlags f)
    : QWidget(parent, f)
{
    QVBoxLayout *vbox = new QVBoxLayout;
    QLabel *title = new QLabel;
    vbox->addWidget(title);
    QProgressBar *progress = new QProgressBar;
    progress->setVisible(false);
    vbox->addWidget(progress);
    QTextBrowser *info = new QTextBrowser;
    vbox->addWidget(info);
    QHBoxLayout *hbox = new QHBoxLayout;
    hbox->setAlignment(Qt::AlignRight);
    QPushButton *finishButton = new QPushButton(i18n("Finish"));
    finishButton->setVisible(false);
    connect(finishButton, &QPushButton::clicked, [this](){
        QCoreApplication::quit();
    });
    QLabel *label = new QLabel(" ");
    label->setFixedHeight(33);
    hbox->addWidget(label);
    hbox->addWidget(finishButton);
    vbox->addLayout(hbox);
    setLayout(vbox);

    connect(installer, &org::isoftlinux::Install::Error, [=](const QString &details){
        info->append(details);
    });

    connect(installer, &org::isoftlinux::Install::PercentChanged, 
            [=](qlonglong status, const QString &file, double percent){
        QFileInfo fileInfo(file);
        if (status == 0)
            title->setText(i18n("Converting %1", fileInfo.fileName()));
        else
            title->setText(i18n("Installing %1", fileInfo.fileName()));
        if (percent == 1.0) {
            if (status == 0) {
                title->setText(i18n("%1 Converted", fileInfo.fileName()));
                info->append(i18n("%1 Converted", fileInfo.fileName()));
            } else {
                title->setText(i18n("%1 Installed", fileInfo.fileName()));
                info->append(i18n("%1 Installed", fileInfo.fileName()));
            }
        }
        progress->setVisible(true);
        progress->setValue(percent * 100.0);
    });

    connect(installer, &org::isoftlinux::Install::Finished, [=](qlonglong status){
        finishButton->setVisible(true);
    });
}

InstallWidget::~InstallWidget()
{
}

#include "moc_installwidget.cpp"
