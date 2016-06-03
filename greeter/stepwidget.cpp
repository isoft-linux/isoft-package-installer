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

#include "stepwidget.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QIcon>
#include <QHBoxLayout>
#include <QStackedWidget>

#include <KLocalizedString>

#include <iostream>

#include "welcomewidget.h"
#include "packagewidget.h"
#include "installwidget.h"

StepWidget::StepWidget(int argc, char **argv, QWidget *parent, Qt::WindowFlags f) 
    : QWidget(parent, f)
{
    m_installer = new org::isoftlinux::Install("org.isoftlinux.Install",
                                               "/org/isoftlinux/Install",
                                               QDBusConnection::systemBus(),
                                               this);
    
    setWindowTitle(i18n("iSOFT Package Installer"));
    setWindowIcon(QIcon::fromTheme("applications-other"));
    setFixedSize(400, 300);
    move((QApplication::desktop()->width() - width()) / 2, 
         (QApplication::desktop()->height() - height()) / 2);

    QHBoxLayout *hbox = new QHBoxLayout;
    setLayout(hbox);

    QStackedWidget *stack = new QStackedWidget;
    WelcomeWidget *welcome = new WelcomeWidget;
#ifdef DEBUG
    std::cout << "DEBUG: " << __FILE__ << " " << __PRETTY_FUNCTION__ 
        << " WelcomeWidget isShowMe " << welcome->isShowMe() << std::endl;
#endif
    connect(welcome, &WelcomeWidget::next, [=](){
        stack->setCurrentIndex(m_currentIndex++);
    });
    if (welcome->isShowMe()) 
        stack->addWidget(welcome);
    PackageWidget *package = new PackageWidget(argc, argv, m_installer);
    connect(package, &PackageWidget::next, [=](){
        stack->setCurrentIndex(m_currentIndex++);
        m_installer->Install();
    });
    if (argc < 3) { 
        m_installer->AddInstall(argv[1]);
        m_installer->Install();
    } else
        stack->addWidget(package);
    InstallWidget *install = new InstallWidget(m_installer);
    stack->addWidget(install);
    hbox->addWidget(stack);
}

StepWidget::~StepWidget()
{
    if (m_installer) {
        delete m_installer;
        m_installer = Q_NULLPTR;
    }
}

#include "moc_stepwidget.cpp"
