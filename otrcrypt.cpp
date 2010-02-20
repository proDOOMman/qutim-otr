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

#include "otrcrypt.h"

OTRCrypt::OTRCrypt():
        m_otrConnection(NULL)
{
    QTextCodec::setCodecForCStrings( QTextCodec::codecForName("utf8") );// кто-то еще использует другую кодировку? Ах да, есть же еще M$ с его ср1251...
    QTranslator translator;
    translator.load(QString("qutim_otr_")+QLocale::system().name());
    qApp->installTranslator(&translator);
    m_chatDlgAction = 0;
}

bool OTRCrypt::init(PluginSystemInterface *plugin_system)
{
    qRegisterMetaType<TreeModelItem> ( "TreeModelItem" );
    m_plugin_system = plugin_system;
    m_event_outgoing_message = m_plugin_system->registerEventHandler("Core/ChatWindow/SendLevel3", this);
    m_event_incoming_message = m_plugin_system->registerEventHandler("Core/ChatWindow/ReceiveLevel1", this);
    m_event_fully_incoming_message = m_plugin_system->registerEventHandler("Core/ChatWindow/ReceiveLevel4", this);
    m_event_temporary_receive = m_plugin_system->registerEventHandler("Core/ChatWindow/ReceiveLevel2", this);
    m_event_context = m_plugin_system->registerEventHandler("Core/ContactList/ContactContext", this);
    m_event_fully_outcoming_message = m_plugin_system->registerEventHandler("Core/ChatWindow/SendLevel1.5", this);
    qsrand(QTime::currentTime().msec()+QCoreApplication::applicationPid());
    OTRL_INIT;
    return true;
}

void OTRCrypt::setProfileName(const QString &name)
{
    m_profile_name = name;
    if(m_otrConnection)
    {
        delete m_otrConnection;
        delete m_otrConnectionAuto;
        delete m_otrConnectionManual;
        delete m_otrConnectionNone;
        delete m_otrConnectionReq;
        m_otrConnection = 0;
        m_otrConnectionAuto = 0;
        m_otrConnectionManual = 0;
        m_otrConnectionNone = 0;
        m_otrConnectionReq = 0;
    }
    QSettings s(QSettings::defaultFormat(),
                QSettings::UserScope, "qutim/"+m_plugin_system->getProfileDir().dirName(), "otr");
    OtrlUserState state = otrl_userstate_create();
    m_otrConnection = new OtrMessaging(m_plugin_system,(OtrPolicy)s.value("policy",qutimotr::OTR_POLICY_AUTO).toInt(), state);
    m_otrConnectionAuto = new OtrMessaging(m_plugin_system,qutimotr::OTR_POLICY_AUTO, state);
    m_otrConnectionNone = new OtrMessaging(m_plugin_system,qutimotr::OTR_POLICY_OFF, state);
    m_otrConnectionManual = new OtrMessaging(m_plugin_system,qutimotr::OTR_POLICY_ENABLED, state);
    m_otrConnectionReq = new OtrMessaging(m_plugin_system,qutimotr::OTR_POLICY_REQUIRE, state);
    if(!m_chatDlgAction)
    {
        m_chatDlgAction = new QAction(QIcon(":/otr"), "OTR", this);
        QMenu *m_chatDlgMenu = new QMenu();
        QAction *m_startSessionAction = m_chatDlgMenu->addAction(QIcon(":/encrypted"), tr("Start private Conversation"));
        m_startSessionAction->setData(0);
        QAction *m_endSessionAction = m_chatDlgMenu->addAction(QIcon(":/uncrypted"), tr("End private Conversation"));
        m_endSessionAction->setData(1);
        m_chatDlgMenu->insertSeparator(NULL);
        QAction *m_verifyAction = m_chatDlgMenu->addAction(QIcon(":/auth"), tr("Verify Fingerprint"));
        m_verifyAction->setData(2);
        QAction *m_sessionIdAction = m_chatDlgMenu->addAction(tr("Show secure Session ID"));
        m_sessionIdAction->setData(3);
        QAction *m_fingerprintAction = m_chatDlgMenu->addAction(tr("Show own Fingerprint"));
        m_fingerprintAction->setData(4);
        m_chatDlgAction->setMenu(m_chatDlgMenu);
        QMenu *m_settingsMenu = m_chatDlgMenu->addMenu(tr("Personal settings"));
        group = new QActionGroup(this);
        group->setExclusive(true);
        group->addAction(tr("System settings"))->setCheckable(true);
        group->addAction(tr("OTR disabled"))->setCheckable(true);
        group->addAction(tr("Manual"))->setCheckable(true);
        group->addAction(tr("Auto"))->setCheckable(true);
        group->addAction(tr("Force OTR"))->setCheckable(true);
        group->actions().at(0)->setChecked(true);
        group->actions().at(0)->setData(QVariant(-1));
        group->actions().at(1)->setData(QVariant(OTRL_POLICY_NEVER));
        group->actions().at(2)->setData(QVariant(OTRL_POLICY_MANUAL));
        group->actions().at(3)->setData(QVariant(OTRL_POLICY_OPPORTUNISTIC));
        group->actions().at(4)->setData(QVariant(OTRL_POLICY_REQUIRE_ENCRYPTION));
        m_settingsMenu->addActions(group->actions());
        m_chatDlgMenu->addMenu(m_settingsMenu);
        connect(m_chatDlgMenu,SIGNAL(triggered(QAction*)),this,SLOT(contextMenuEvent(QAction*)));
        connect(m_settingsMenu,SIGNAL(triggered(QAction*)),this,SLOT(contextSettingsMenuEvent(QAction*)));
        m_plugin_system->registerContactMenuAction(m_chatDlgAction);
    }
}

void OTRCrypt::contextMenuEvent(QAction *a)
{
    foreach(QAction *b,group->actions())
        if(a==b)
            return;
    QString myacc = contextItem.m_account_name;
    QString himacc = contextItem.m_item_name;
    mayBeCreateClosure(myacc,himacc,contextItem);
    QutimOtrClosure *closure = m_items[myacc][himacc];
    switch(a->data().toInt())
    {
    case 0:
        emit closure->initiateSession(true);
        break;
    case 1:
        emit closure->endSession(true);
        break;
    case 2:
        emit closure->verifyFingerprint(true);
        break;
    case 3:
        emit closure->sessionID(true);
        break;
    case 4:
        emit closure->fingerprint(true);
        break;
    default:
        qDebug() << "[OTR] Wow, it is interesting!";
    }
}

void OTRCrypt::contextSettingsMenuEvent(QAction *a)
{
    qDebug() << "[OTR] setting policy: " << a->data().toInt();
    QString myacc = contextItem.m_account_name;
    QString himacc = contextItem.m_item_name;
    mayBeCreateClosure(myacc,himacc,contextItem);
    QutimOtrClosure *closure = m_items[myacc][himacc];
    closure->setPolicy(a->data().toInt());
}

void OTRCrypt::processEvent(Event &event)
{
    TreeModelItem eventitem = *(TreeModelItem*)(event.args.at(0));
    if(event.id==m_event_context)
    {
        contextItem = eventitem;
        mayBeCreateClosure(eventitem.m_account_name,eventitem.m_item_name,eventitem);
        m_items[eventitem.m_account_name][eventitem.m_item_name]->updateMessageState();
        QSettings s(QSettings::defaultFormat(), QSettings::UserScope, "qutim/"+m_plugin_system->getProfileDir().dirName(), "otr");
        int pol = s.value(contextItem.m_protocol_name+"/"+contextItem.m_account_name+"/"+contextItem.m_item_name,-1).toInt();
        if(pol==-1)
            group->actions().at(0)->setChecked(true);
        else
            foreach(QAction *act,group->actions())
                if(act->data()==pol)
                    act->setChecked(true);
    }
    if(eventitem.m_item_type!=0)
        return;
    if ( event.id == m_event_outgoing_message ){
            QString *msg = (QString*) (event.args.at(1));
            encryptMessage(msg,eventitem);
    } else
    if ( event.id == m_event_incoming_message ){
            QString *msg = (QString*) (event.args.at(1));
            decryptMessage(msg,eventitem);
            
//            if(msg->isEmpty())
//            {
//                if(event.args.count()>2)
//                    *(bool*)(event.args.at(2)) = false;
//                else
//                {
//                    msg->append(tr("OTR system message"));
//                    event.args.append(new bool(false));
//                }
//            }
    } else
        if ( event.id == m_event_fully_incoming_message || event.id == m_event_fully_outcoming_message ){
        QString *msg = (QString*) (event.args.at(1));
//        if(msg->contains("&lt;b&gt;",Qt::CaseSensitive))
//        {
//            msg->replace("&lt;", "<");
//            msg->replace("&gt;", ">");
//        }
        mayBeCreateClosure(eventitem.m_account_name,eventitem.m_item_name,eventitem);
        if(!msg->startsWith("<OTR service>\n"))
        {
//            m_items[eventitem.m_account_name][eventitem.m_item_name]->updateState();
        }
        else
            msg->remove("<OTR service>\n");
        int id = m_items[eventitem.m_account_name][eventitem.m_item_name]->getCurrentId();
        QString fileName;
        QString stateString;
        int state = m_items[eventitem.m_account_name][eventitem.m_item_name]->getState();
        switch(state)
        {
        case 1:
            fileName = ":auth";
            stateString = tr("<b>OTR is active, but user unverifed</b>");
            break;
        case 2:
            fileName = ":encrypted";
            stateString = tr("<b>OTR encryption is active</b>");
            break;
        default:
            fileName = ":uncrypted";
            stateString = tr("<b>OTR disabled</b>");
            break;
        }

        QFile f(fileName);
        f.open(QIODevice::ReadOnly);
        QByteArray a = f.readAll();
        bool enabled = false;
        QSettings s(QSettings::defaultFormat(), QSettings::UserScope, "qutim/"+m_plugin_system->getProfileDir().dirName(), "otr");
        if(s.value("policy",qutimotr::OTR_POLICY_AUTO).toInt()!=qutimotr::OTR_POLICY_OFF)
            enabled = true;
        if(!s.value(eventitem.m_protocol_name+"/"+eventitem.m_account_name+"/"+eventitem.m_item_name,-1).toInt())
            enabled = false;
        else if(s.value(eventitem.m_protocol_name+"/"+eventitem.m_account_name+"/"+eventitem.m_item_name,-1).toInt()>0)
            enabled = true;
        if(!s.value("notify",true).toBool())
            enabled = false;
        if(enabled)
            msg->append(QString(" <div id=\"statusImage_%1\"><br><img src=\"data:image/png;base64,%3\""
                    "alt=\"Status\" width=\"32\" height=\"32\" onload=\"javascript:"
                    "if(document.getElementById('statusImage_%2'))"
                    "document.getElementById('statusImage_%2').style.display = 'none';"
                    //"else alert('alert! Cant found id %2');"
                    "\">  %4</div>").arg(QString::number(id)).arg(QString::number(id-1)).arg(QString(a.toBase64())).arg(stateString));
    }
    else if(event.id == m_event_temporary_receive)
    {
        QString *msg = (QString*) (event.args.at(1));
        bool &b = event.at<bool>(2);
        if(msg->startsWith("<Internal OTR message>\n"))
        {
            m_items[eventitem.m_account_name][eventitem.m_item_name]->updateMessageState();
            b = true;
            msg->remove("<Internal OTR message>\n");
            QString message(*msg);
            message = message.remove("<Internal OTR message>\n").remove("<b>");
            //m_plugin_system->addServiceMessage(eventitem,message);
        }
        if(msg->startsWith("<OTR service>\n"))
            msg->remove("<OTR service>\n");
    }
}

void OTRCrypt::decryptMessage(QString *message, TreeModelItem &item)
{
    if(message->startsWith("<OTR service>\n"))
        return;
    QString myacc = item.m_account_name;
    QString himacc = item.m_item_name;
    QSystemSemaphore sem("keygen",1,QSystemSemaphore::Open);
    if(sem.acquire())
    {
//        qDebug() << "[OTR] decrypt...";
        mayBeCreateClosure(myacc,himacc,item);
        QString decrypted = m_items[myacc][himacc]->getMessaging()->decryptMessage(
                himacc,
                myacc,
                *message,
                item);
        message->clear();
        Q_ASSERT(m_items[myacc][himacc]);
        message->append(decrypted);
        sem.release(1);
    }
}

void OTRCrypt::encryptMessage(QString *message, TreeModelItem &item)
{
    QString myacc = item.m_account_name;
    QString himacc = item.m_item_name;
    mayBeCreateClosure(myacc,himacc,item);
    QString encrypted = m_items[myacc][himacc]->getMessaging()->encryptMessage(
            item.m_account_name,
            item.m_item_name,
            *message,
            item);
    message->clear();
    message->append(encrypted);
}

void OTRCrypt::mayBeCreateClosure(QString &myacc, QString &himacc, TreeModelItem &item)
{
    QList<OtrMessaging*> meslist;
    meslist << m_otrConnection << m_otrConnectionNone << m_otrConnectionManual << m_otrConnectionAuto << m_otrConnectionReq;
    if(!(m_items.contains(myacc)&&
       m_items.value(myacc).contains(himacc)))
    {
        QutimOtrClosure *closure = new QutimOtrClosure(myacc,himacc,meslist, item, m_plugin_system);
        closure->setDialog(m_chatDlgAction);
        closure->setIsLoggedIn(true);
        m_items[myacc][himacc] = closure;
    }
    m_items[myacc][himacc]->updateMessageState();
}

Q_EXPORT_PLUGIN2(OTR, OTRCrypt);
