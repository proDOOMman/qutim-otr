/*

    Copyright (c) 2010 by Stanislav (proDOOMman) Kosolapov <prodoomman@shell.tor.hu>

 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************
*/

#ifndef SETTINGSWIDGET_H
#define SETTINGSWIDGET_H

#include "ui_settingswidget.h"
#include "psi/OtrMessaging.hpp"
#include <qutim/plugininterface.h>
#include <QMessageBox>
#include "smpdialog.h"
using namespace qutim_sdk_0_2;

class SettingsWidget : public QWidget, private Ui::SettingsWidget {
    Q_OBJECT
public:
    SettingsWidget(psiotr::OtrMessaging *otr, PluginSystemInterface *plugin, QWidget *parent = 0);
private:
    psiotr::OtrMessaging *m_otr;
    PluginSystemInterface *m_plugin;
    QList<psiotr::Fingerprint>  m_fingerprints;
public slots:
    void save();
private slots:
    void on_pushButton_released();
    void on_forgetButton_released();
    void updateFingerprints();
    void updateKeys();
    void keyChanged(int row);
    void fingerprintChanged(int row);
};

#endif // SETTINGSWIDGET_H
