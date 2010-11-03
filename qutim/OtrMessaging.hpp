/*
 * OtrMessaging.hpp - interface to libotr
 * Copyright (C) 2007  Timo Engel (timo-e@freenet.de)
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

#ifndef OTRMESSAGING_HPP_
#define OTRMESSAGING_HPP_

#include <QString>
#include <QList>
#include <QHash>
#include <qutim/plugininterface.h>
using namespace qutim_sdk_0_2;

#include "global.h"

class OtrInternal;

// ---------------------------------------------------------------------------

namespace qutimotr
{

// ---------------------------------------------------------------------------

enum OtrPolicy
{
    OTR_POLICY_OFF,
    OTR_POLICY_ENABLED,
    OTR_POLICY_AUTO,
    OTR_POLICY_REQUIRE
};

// ---------------------------------------------------------------------------

enum OtrMessageState
{
    OTR_MESSAGESTATE_UNKNOWN,
    OTR_MESSAGESTATE_PLAINTEXT,
    OTR_MESSAGESTATE_ENCRYPTED,
    OTR_MESSAGESTATE_FINISHED
};

// ---------------------------------------------------------------------------

/**
* This struct contains all data shown in the table of 'Known Fingerprints'.
*/
struct Fingerprint
{
    /**
    * Pointer to fingerprint in libotr struct. Binary format.
    */
    unsigned char* fingerprint;

    /**
    * own account
    */
    QString account;

    /**
    * owner of the fingerprint
    */
    QString username;

    /**
    * The fingerprint in a human-readable format
    */
    QString fingerprintHuman;

    /**
    * the level of trust
    */
    QString trust;

    /**
    * The messageState of the context (i.e. plaintext, encrypted, finished)
    */
    QString messageState;
};

// ---------------------------------------------------------------------------

/**
* This class it the interface to the off the record messaging library.
* See the libotr documentation for more information.
*
*/
class OtrMessaging
{
public:

    /**
    * Constructor
    *
    * @param plugin Pointer to the plugin, used for sending messages.
    * @param policy The default OTR policy
    */
    OtrMessaging(PluginSystemInterface *plugin, OtrPolicy policy, OtrlUserState userstate);

    /**
    * Deconstructor
    */
    ~OtrMessaging();

    /**
    * Process an outgoing message.
    *
    * @param from Account the message is send from
    * @param to Recipient of message
    * @param message The message itself.
    * @param item The item that emit event.
    *
    * @return The encrypted message.
    */
    QString encryptMessage(const QString& from, const QString& to,
                           const QString& message, TreeModelItem &item);

    /**
    * Decrypt an incoming message
    *
    * @param from Sender of the message
    * @param to Account the message is send to.
    * @param message the mesasge itself.
    * @param item The item that emit event.
    *
    * @return The decrypted message if the original message was encrypted.
    */
    QString decryptMessage(const QString& from, const QString& to,
                           const QString& message, TreeModelItem &item);

    /**
    * Returns a list of known fingerprints.
    */
    QList<Fingerprint> getFingerprints();

    /**
    * Set fingerprint verified/not verified.
    */
    void verifyFingerprint(const Fingerprint& fingerprint, bool verified);

    /**
    * Delete a known fingerprint.
    */
    void deleteFingerprint(const Fingerprint& fingerprint);
    void deleteKey(QString account, QString protocol);

    /**
    * Get hash of fingerprints of own private keys.
    * Account -> KeyFingerprint
    */
    QHash<QString, QHash<QString, QString> > getPrivateKeys();

    /**
    * Send an OTR query message from account to jid.
    */
    void startSession(const QString& account, const QString& jid, TreeModelItem &item, int pol);

    /**
    * Send otr-finished message to user.
    */
    void endSession(const QString& account, const QString& jid, TreeModelItem &item);

    /**
    * Return the messageState of a context.
    * i.e. plaintext, encrypted, finished
    */
    OtrMessageState getMessageState(const QString& thisJid,
                                    const QString& remoteJid,
                                    TreeModelItem &item);

    /**
    * returns the messageState in human-readable string.
    */
    QString getMessageStateString(const QString& thisJid,
                                  const QString& remoteJid,
                                  TreeModelItem &item);
    
    int getMessageStateInt(const QString& thisJid,
                                  const QString& remoteJid,
                                  TreeModelItem &item);

    /**
    * Return the secure session id (ssid) for a context
    */
    QString getSessionId(const QString& thisJid, const QString& remoteJid, TreeModelItem &item);

    /**
    * Set the default OTR policy.
    */
    void setPolicy(OtrPolicy policy);

    /**
    * @return The default OTR policy
    */
    OtrPolicy getPolicy();

    // TODO: check this function
    void requestAuth(TreeModelItem &item, bool agree, QString answer = QString(), QString question = QString());


private:
    OtrPolicy    m_otrPolicy;
    OtrInternal* m_impl;
};

// ---------------------------------------------------------------------------

} // namespace qutimotr

#endif
