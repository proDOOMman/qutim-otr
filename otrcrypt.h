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

#ifndef OTRCRYPT_H
#define OTRCRYPT_H

#include <QObject>
#include <QHash>
#include "global.h"
#include "psi/OtrInternal.hpp"
#include "psi/psiotrclosure.h"
#include "psi/OtrMessaging.hpp"
#include "psi/psiotrclosure.h"
#include "settingswidget.h"
#include <QSystemSemaphore>

using namespace psiotr;

class OTRCrypt : public QObject, SimplePluginInterface, EventHandler
{
    Q_OBJECT
    Q_INTERFACES ( qutim_sdk_0_2::PluginInterface)
public:
    OTRCrypt();
    bool init(PluginSystemInterface *plugin_system);
    void release(){}
    QString name(){return "OTR messaging";}
    QString description(){return "Off-The-Record messaging";}
    QString type(){return "other";}
    QIcon* icon(){return new QIcon(":/otr");}
    void setProfileName(const QString& name);
    QWidget* settingsWidget()
    {
        m_settings_widget = new SettingsWidget(m_otrConnection,m_plugin_system);
        return m_settings_widget;
    }
    void removeSettingsWidget(){/*if(m_settings_widget) delete m_settings_widget; m_settings_widget = 0;*/}
    void saveSettings(){if(m_settings_widget) m_settings_widget->save();/*this->setProfileName(m_profile_name);*/}
    void processEvent(Event &event);
    qint16 m_event_incoming_message;
    qint16 m_event_outgoing_message;
    qint16 m_event_context;
    qint16 m_event_fully_incoming_message;
    qint16 m_event_fully_outcoming_message;
    qint16 m_event_temporary_receive;
    void decryptMessage(QString *message, TreeModelItem &item);
    void encryptMessage(QString *message, TreeModelItem &item);
    void mayBeCreateClosure(QString &myacc, QString &himacc, TreeModelItem &item);
signals:

public slots:
    void contextMenuEvent(QAction*);
    void contextSettingsMenuEvent(QAction*);

private:
    PluginSystemInterface *m_plugin_system;
    QString m_profile_name;
    OtrMessaging* m_otrConnection;
    OtrMessaging* m_otrConnectionNone;
    OtrMessaging* m_otrConnectionManual;
    OtrMessaging* m_otrConnectionAuto;
    OtrMessaging* m_otrConnectionReq;
    QHash<QString,QHash<QString,PsiOtrClosure *> > m_items;
    TreeModelItem contextItem;
    QAction *m_chatDlgAction;
    SettingsWidget *m_settings_widget;
    QActionGroup *group;
};

#endif // OTRCRYPT_H
