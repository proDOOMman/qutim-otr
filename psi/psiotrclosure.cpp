/*
 * psiotrclosure.cpp
 *
 * Copyright (C) Timo Engel (timo-e@freenet.de), Berlin 2007.
 * This program was written as part of a diplom thesis advised by
 * Prof. Dr. Ruediger Weis (PST Labor)
 * at the Technical University of Applied Sciences Berlin.
 * Copyright (C) Stanislav (proDOOMman) Kosolapov <prodoomman@shell.tor.hu>, 2010
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "psiotrclosure.h"
#include "OtrMessaging.hpp"

#include <QAction>
#include <QMenu>
#include <QMessageBox>

namespace psiotr
{

//-----------------------------------------------------------------------------

PsiOtrClosure::PsiOtrClosure(QString &account, const QString& toJid,
                             QList<OtrMessaging*> otrcList, TreeModelItem &item,
                             PluginSystemInterface* pluginSystem)
        : /*m_otr(otrcList.at(0)),*/
        m_otr_system(otrcList.at(0)),
        m_otr_none(otrcList.at(1)),
        m_otr_manual(otrcList.at(2)),
        m_otr_auto(otrcList.at(3)),
        m_otr_req(otrcList.at(4)),
        m_myAccount(account),
        m_otherJid(toJid),
        m_isLoggedIn(false),
        m_parentWidget(0),
        m_item(item),
        m_pluginSystem(pluginSystem)
{
    m_timer = new QTimer(this);
    connect(m_timer,SIGNAL(timeout()),this,SLOT(updateState()));
    m_timer->start(1000);
    m_id = 0;
    QSettings s(QSettings::defaultFormat(), QSettings::UserScope, "qutim/"+m_pluginSystem->getProfileDir().dirName(), "otr");
    int pol = s.value(m_item.m_protocol_name+"/"+m_item.m_account_name+"/"+m_item.m_item_name,(int)-1).toInt();
    this->setPolicy(pol);
    prevState = 666;
}

//-----------------------------------------------------------------------------

PsiOtrClosure::~PsiOtrClosure()
{
}

//-----------------------------------------------------------------------------

void PsiOtrClosure::initiateSession(bool b)
{
        Q_UNUSED(b);
        QSettings s(QSettings::defaultFormat(), QSettings::UserScope, "qutim/"+m_pluginSystem->getProfileDir().dirName(), "otr");
        int pol = s.value(m_item.m_protocol_name+"/"+m_item.m_account_name+"/"+m_item.m_item_name,(int)OTRL_POLICY_DEFAULT).toInt();
        m_otr->startSession(m_myAccount, m_otherJid, m_item, pol);
}

void PsiOtrClosure::setPolicy(int pol)
{
    QSettings s(QSettings::defaultFormat(), QSettings::UserScope, "qutim/"+m_pluginSystem->getProfileDir().dirName(), "otr");
    if (pol!=s.value(m_item.m_protocol_name+"/"+m_item.m_account_name+"/"+m_item.m_item_name,-1).toInt())
        emit endSession(true);
    switch(pol){
    case OTRL_POLICY_NEVER:
        m_otr = m_otr_none;
        break;
    case OTRL_POLICY_MANUAL:
        m_otr = m_otr_manual;
        break;
    case OTRL_POLICY_OPPORTUNISTIC:
        m_otr = m_otr_auto;
        break;
    case OTRL_POLICY_REQUIRE_ENCRYPTION:
        m_otr = m_otr_req;
        break;
    default:
        m_otr = m_otr_system;
        break;
    }
    s.setValue(m_item.m_protocol_name+"/"+m_item.m_account_name+"/"+m_item.m_item_name,pol);
}

//-----------------------------------------------------------------------------

void PsiOtrClosure::verifyFingerprint(bool)
{
    SMPdialog dialog(m_item,m_otr,m_parentWidget);
    dialog.exec();
}

//-----------------------------------------------------------------------------

void PsiOtrClosure::sessionID(bool)
{
    QString sId = m_otr->getSessionId(m_myAccount,
                                      m_otherJid.toStdString().c_str(),m_item);
    QString msg;

    if (sId.isEmpty() ||
        (sId.compare(QString("<b></b>")) == 0) ||
        (sId.compare(QString("<b> </b>")) == 0) ||
        (sId.compare(QString(" <b> </b>")) == 0))
    {
        msg = tr("no active encrypted session");
    }
    else
    {
        msg = tr("Session ID of connection from account %1 to %2 is: ").arg(m_myAccount).arg(m_otherJid)+"<br/>" + sId + ".";
    }

    QMessageBox mb(QMessageBox::Information, "qutim-otr", msg, NULL, m_parentWidget,
                   Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
    mb.setTextFormat(Qt::RichText);
    mb.exec();
}

//-----------------------------------------------------------------------------

void PsiOtrClosure::endSession(bool b)
{
        Q_UNUSED(b);
        m_otr->endSession(m_myAccount, m_otherJid, m_item);
        updateMessageState();
}

//-----------------------------------------------------------------------------

void PsiOtrClosure::fingerprint(bool)
{
    QString fingerprint =  (m_otr->getPrivateKeys().value(m_myAccount).value(m_item.m_protocol_name,
                                                                             tr("no private key for ") +
                                                                             m_myAccount));

        QString msg(tr("Fingerprint for account %1 is:").arg(m_myAccount)+"\n" + fingerprint + ".");

        QMessageBox mb(QMessageBox::Information, "qutim-otr",
                   msg, NULL, m_parentWidget,
                   Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
        mb.exec();
}

void PsiOtrClosure::setDialog(QAction *a)
{
    m_chatDlgMenu = a->menu();
    m_chatDlgAction = a;
    m_verifyAction = m_chatDlgMenu->actions().at(2);
    m_sessionIdAction  = m_chatDlgMenu->actions().at(3);
    m_fingerprintAction = m_chatDlgMenu->actions().at(4);
    m_startSessionAction = m_chatDlgMenu->actions().at(0);
    m_endSessionAction = m_chatDlgMenu->actions().at(1);

}

//-----------------------------------------------------------------------------

void PsiOtrClosure::updateState()
{
    qint16 newState = this->getState();
    if(prevState==666)
        prevState = newState;
    else
    if(prevState!=newState)
    {
        bool enabled = false;
        QSettings s(QSettings::defaultFormat(), QSettings::UserScope, "qutim/"+m_pluginSystem->getProfileDir().dirName(), "otr");
        if(s.value("policy",psiotr::OTR_POLICY_AUTO).toInt()!=psiotr::OTR_POLICY_OFF)
            enabled = true;
        if(!s.value(m_item.m_protocol_name+"/"+m_item.m_account_name+"/"+m_item.m_item_name,-1).toInt())
            enabled = false;
        else if(s.value(m_item.m_protocol_name+"/"+m_item.m_account_name+"/"+m_item.m_item_name,-1).toInt()>0)
            enabled = true;
        if(!s.value("notify",true).toBool())
            enabled = false;
        if(!enabled)
            return;
        prevState = newState;
        QString state = m_otr->getMessageStateString(m_myAccount,m_otherJid,m_item);
        m_pluginSystem->addMessageFromContact(m_item,"<OTR service>\n"+tr("OTR state is changed. New state is [%1]").arg(state),QDateTime::currentDateTime());
    }
}

void PsiOtrClosure::updateMessageState()
{
    if (m_chatDlgAction != 0)
    {
        QString stateString("OTR Plugin [" +
                            m_otr->getMessageStateString(m_myAccount,
                                                          m_otherJid,
                                                          m_item)
                            + "]");
        m_chatDlgAction->setText(stateString);

        OtrMessageState state = m_otr->getMessageState(m_myAccount,
                                                        m_otherJid,
                                                        m_item);

        if (state == OTR_MESSAGESTATE_ENCRYPTED)
        {
            m_verifyAction->setEnabled(true);
            m_sessionIdAction->setEnabled(true);
            m_startSessionAction->setEnabled(false);
            m_endSessionAction->setEnabled(true);
        }
        else if (state == OTR_MESSAGESTATE_PLAINTEXT)
        {
            m_verifyAction->setEnabled(false);
            m_sessionIdAction->setEnabled(false);
            m_startSessionAction->setEnabled(true);
            m_endSessionAction->setEnabled(false);
        }
        else // finished, unknown
        {
            m_startSessionAction->setEnabled(true);
            m_endSessionAction->setEnabled(true);
            m_verifyAction->setEnabled(false);
            m_sessionIdAction->setEnabled(false);
        }

        if (m_otr->getPolicy() < OTR_POLICY_ENABLED)
        {
            m_startSessionAction->setEnabled(false);
            m_endSessionAction->setEnabled(false);
        }
    }
}

//-----------------------------------------------------------------------------

void PsiOtrClosure::setIsLoggedIn(bool isLoggedIn)
{
        m_isLoggedIn = isLoggedIn;
}

//-----------------------------------------------------------------------------

bool PsiOtrClosure::isLoggedIn() const
{
        return m_isLoggedIn;
}

//-----------------------------------------------------------------------------

} // namespace
