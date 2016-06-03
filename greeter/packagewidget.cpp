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

#include "packagewidget.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QListWidget>
#include <QCoreApplication>

#include <KLocalizedString>

#include <iostream>

PackageWidget::PackageWidget(int argc, 
                             char **argv, 
                             org::isoftlinux::Install *installer, 
                             QWidget *parent, 
                             Qt::WindowFlags f)
    : QWidget(parent, f), 
      m_installer(installer)
{
    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->addWidget(new QLabel(i18n("Please confirm the packages you want to install")));
    QListWidget *packages = new QListWidget;
    QPushButton *nextButton = new QPushButton(i18n("Next"));
    nextButton->setMaximumWidth(100);
    for (int i = 1; i < argc; i++) {
        QFileInfo file(argv[i]);
        files << file;
        QCheckBox *choose = new QCheckBox(file.fileName());
        connect(choose, &QCheckBox::clicked, [=](){
            int checked = 0;
            for (int row = 0; row < packages->count(); row++) {
                QCheckBox *obj = (QCheckBox *)packages->itemWidget(packages->item(row));
                if (obj->isChecked())
                    checked++;
            }
            nextButton->setEnabled(checked ? true : false);
        });
        choose->setChecked(true);
        QListWidgetItem *item = new QListWidgetItem;
        packages->addItem(item);
        packages->setItemWidget(item, choose);
    }
    vbox->addWidget(packages);
    QHBoxLayout *hbox = new QHBoxLayout;
    connect(nextButton, &QPushButton::clicked, [=](){
        for (int row = 0; row < packages->count(); row++) {
            QCheckBox *obj = (QCheckBox *)packages->itemWidget(packages->item(row));
            if (obj->isChecked()) {
                Q_FOREACH (QFileInfo file, files) {
                    std::cout << "DEBUG: " << __PRETTY_FUNCTION__ << " " << 
                    file.fileName().toStdString() << " " << 
                    obj->text().replace("&", "").toStdString() << std::endl;
                    if (file.fileName() == obj->text().replace("&", ""))
                        installer->AddInstall(file.absoluteFilePath());
                }
            }
        }
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

PackageWidget::~PackageWidget()
{
}

#include "moc_packagewidget.cpp"
