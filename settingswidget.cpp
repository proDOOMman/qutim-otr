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

#include "settingswidget.h"
#include <QDir>
#include <QDebug>

SettingsWidget::SettingsWidget(psiotr::OtrMessaging *otr, PluginSystemInterface *plugin, QWidget *parent) :
    QWidget(parent),
    m_otr(otr),
    m_plugin(plugin)
{
    setupUi(this);
    QSettings s(QSettings::defaultFormat(), QSettings::UserScope, "qutim/"+m_plugin->getProfileDir().dirName(), "otr");
    switch(s.value("policy",psiotr::OTR_POLICY_AUTO).toInt())
    {
    case psiotr::OTR_POLICY_OFF:
        polAuto->setChecked(false);
        polEnable->setChecked(false);
        polReq->setChecked(false);
        polAuto->setEnabled(false);
        polReq->setEnabled(false);
        break;
    case psiotr::OTR_POLICY_REQUIRE:
        polAuto->setChecked(true);
        polEnable->setChecked(true);
        polReq->setChecked(true);
        polAuto->setEnabled(true);
        polReq->setEnabled(true);
        break;
    case psiotr::OTR_POLICY_AUTO:
        polEnable->setChecked(true);
        polReq->setChecked(false);
        polAuto->setEnabled(true);
        polReq->setEnabled(true);
        polAuto->setChecked(true);
        break;
    case psiotr::OTR_POLICY_ENABLED:
        polAuto->setChecked(false);
        polEnable->setChecked(true);
        polReq->setChecked(false);
        polAuto->setEnabled(true);
        polReq->setEnabled(false);
        break;
    }
    emit updateFingerprints();
    connect(fingerprintsTable,SIGNAL(currentCellChanged(int,int,int,int)),this,SLOT(fingerprintChanged(int)));
    emit updateKeys();
    connect(keysTable,SIGNAL(currentCellChanged(int,int,int,int)),this,SLOT(keyChanged(int)));
    fingerprintsTable->setColumnWidth(0, 150);
    fingerprintsTable->setColumnWidth(1, 150);
    fingerprintsTable->setColumnWidth(2, 360);
    fingerprintsTable->setColumnWidth(3, 80);
    fingerprintsTable->setColumnWidth(4, 100);
    forgetButton->setEnabled(false);
    pushButton->setEnabled(false);
}

void SettingsWidget::fingerprintChanged(int row)
{
    if(row>=0)
    {
        forgetButton->setEnabled(true);
    }
    else
    {
        forgetButton->setEnabled(false);
    }
}

void SettingsWidget::keyChanged(int row)
{
    if(row>=0)
    {
        pushButton->setEnabled(true);
    }
    else
    {
        pushButton->setEnabled(false);
    }
}

void SettingsWidget::updateKeys()
{
    keysTable->setRowCount(0);
    QHash<QString, QHash<QString ,QString> > privateKeys = m_otr->getPrivateKeys();
    foreach(QString acc,privateKeys.keys())
    {
        foreach(QString prot, privateKeys.value(acc).keys())
        {
            QString fp = privateKeys.value(acc).value(prot);
            keysTable->insertRow(keysTable->rowCount());
            QList<QTableWidgetItem*> row;
            row.append(new QTableWidgetItem(acc));
            row.append(new QTableWidgetItem(prot));
            row.append(new QTableWidgetItem(fp));
            keysTable->setItem(keysTable->rowCount()-1,0,row.at(0));
            keysTable->setItem(keysTable->rowCount()-1,1,row.at(1));
            keysTable->setItem(keysTable->rowCount()-1,2,row.at(2));
        }
    }
}

void SettingsWidget::updateFingerprints()
{
    fingerprintsTable->setRowCount(0);
    m_fingerprints = m_otr->getFingerprints();
    QListIterator<psiotr::Fingerprint> fingerprintIt(m_fingerprints);
    while(fingerprintIt.hasNext())
    {
        psiotr::Fingerprint fp = fingerprintIt.next();
        fingerprintsTable->insertRow(fingerprintsTable->rowCount());
        fingerprintsTable->setItem(fingerprintsTable->rowCount()-1,0,new QTableWidgetItem(fp.account));
        fingerprintsTable->setItem(fingerprintsTable->rowCount()-1,1,new QTableWidgetItem(fp.username));
        fingerprintsTable->setItem(fingerprintsTable->rowCount()-1,2,new QTableWidgetItem(fp.fingerprintHuman));
        fingerprintsTable->setItem(fingerprintsTable->rowCount()-1,3,new QTableWidgetItem(fp.trust));
        fingerprintsTable->setItem(fingerprintsTable->rowCount()-1,4,new QTableWidgetItem(fp.messageState));
        fingerprintsTable->item(fingerprintsTable->rowCount()-1,0)->setData(Qt::UserRole,fp.fingerprintHuman);
    }
}

void SettingsWidget::save()
{
    QSettings s(QSettings::defaultFormat(), QSettings::UserScope, "qutim/"+m_plugin->getProfileDir().dirName(), "otr");
    int pol = psiotr::OTR_POLICY_OFF;
    if(polReq->isChecked()&&polReq->isEnabled())
        pol = psiotr::OTR_POLICY_REQUIRE;
    else if(polAuto->isChecked()&&polAuto->isEnabled())
        pol = psiotr::OTR_POLICY_AUTO;
    else if(polEnable->isChecked()&&polEnable->isEnabled())
        pol = psiotr::OTR_POLICY_ENABLED;
    s.setValue("policy",pol);
    m_otr->setPolicy((OtrPolicy)pol);
}

void SettingsWidget::on_forgetButton_released()
{
    if(fingerprintsTable->currentRow()<0)
        return;
    QString msg(tr("Are you sure you want to delete the fingerprint:\naccount: %1\nbuddy: %2\nfingerprint: %3").arg(m_fingerprints[fingerprintsTable->currentRow()].account).arg(m_fingerprints[fingerprintsTable->currentRow()].username).arg(m_fingerprints[fingerprintsTable->currentRow()].fingerprintHuman));

    QMessageBox mb(QMessageBox::Question, tr("qutim-otr"), msg,
                   QMessageBox::Yes | QMessageBox::No, this,
                   Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);

    if (mb.exec() == QMessageBox::Yes)
    {
        m_otr->deleteFingerprint(m_fingerprints[fingerprintsTable->currentRow()]);
        updateFingerprints();
    }
}

void SettingsWidget::on_pushButton_released()
{
    if(keysTable->currentRow()<0)
        return;
    QString msg(tr("Are you sure you want to delete the private key:\naccount: %1\nprotocol: %2\n fingerptint: %3").arg(keysTable->item(keysTable->currentRow(),0)->text()).arg(keysTable->item(keysTable->currentRow(),1)->text()).arg(keysTable->item(keysTable->currentRow(),2)->text()));

    QMessageBox mb(QMessageBox::Question, tr("qutim-otr"), msg,
                   QMessageBox::Yes | QMessageBox::No, this,
                   Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);

    if (mb.exec() == QMessageBox::Yes)
    {
        m_otr->deleteKey(keysTable->item(keysTable->currentRow(),0)->text(),keysTable->item(keysTable->currentRow(),1)->text());
        emit updateKeys();
    }
}
