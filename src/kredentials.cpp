/*
 * Copyright 2004-2007,2011 by the Massachusetts Institute of
 * Technology.
 * Copyright 2007 Hai Zaar <haizaar@gmail.com>
 * All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that copyright notice and this permission
 * notice appear in supporting documentation, and that the name of
 * M.I.T. not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.  Furthermore if you modify this software you must label
 * your software as modified software and not distribute it in such a
 * fashion that it might be confused with the original
 * M.I.T. software.  M.I.T. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * $Id$
 */

#include "kredentials.h"

// XXX TEMPORARILY disable dcop stuff while porting to KDE4
#if 0
// for DCOP access to screen saver
#include <dcopref.h>
#endif

#include <iostream>
#include <memory>
#include <string>
#include <time.h>

#include <kpassworddialog.h>

#include <QMenu>

#ifndef NDEBUG
#define DEFAULT_RENEWAL_INTERVAL 20
#define DEFAULT_WARNING_INTERVAL 3600
#define LOG qDebug()
#else
/* These intervals really need to be configurable.  In some locations
 * new tickets need to be obtained every day, while others grant
 * tickets with a long renew lifetime and only expect to get new ones
 * once every week or so.  Different intervals are appropriate at
 * different sites...
 */
#define DEFAULT_RENEWAL_INTERVAL 3600
#define DEFAULT_WARNING_INTERVAL 86400
#define LOG qDebug().setVerbosity(0)
#endif /*DEBUG*/

kredentials::kredentials(int notify) {

    QSystemTrayIcon();
    tixmgr();
    // set the shell's ui resource file
    //setXMLFile("kredentialsui.rc");

    LOG << "kredentials constructor called" << kerror;

    ctxMenu = new QMenu("MenuBar");
    this->setContextMenu(ctxMenu);

    doNotify = notify;
    renewWarningTime = DEFAULT_WARNING_INTERVAL;
    secondsToNextRenewal = DEFAULT_RENEWAL_INTERVAL;
    renewWarningFlag = 0;
    this->setIcon(QIcon(":/resources/kerberos.png"));

    config = KSharedConfig::openConfig();
    generalConfigGroup = KConfigGroup(config, "General");
    setDoAklog(generalConfigGroup.readEntry("RunAklog", true));

    renewAct = new QAction(QIcon("1rightarrow"), tr("Renew credentials"), this);
    connect(renewAct, SIGNAL(triggered()), this, SLOT(tryRenewTickets()));
    ctxMenu->addAction(renewAct);

    freshTixAct = new QAction(QIcon(""), tr("&Get new credentials"), this);
    connect(freshTixAct, SIGNAL(triggered()), this, SLOT(tryPassGetTickets()));
    ctxMenu->addAction(freshTixAct);
	
    statusAct = new QAction(QIcon(""), tr("&Credential Status"), this);
    connect(statusAct, SIGNAL(triggered()), this, SLOT(showTicketCache()));
    ctxMenu->addAction(statusAct);

    destroyAct = new QAction(QIcon(""), tr("&Destroy credentials"), this);
    connect(destroyAct, SIGNAL(triggered()), this, SLOT(destroyTickets()));
    ctxMenu->addAction(destroyAct);

    toggleAklogAct = new QAction(tr("&Renew AFS tokens"), this);
    toggleAklogAct->setCheckable(true);

    //toggleAklogAct = new KToggleAction(tr("&Renew AFS tokens"), this);
    connect(toggleAklogAct, SIGNAL(triggered()), this, SLOT(prefsAklog()));
    toggleAklogAct->setChecked(doAklog);
    ctxMenu->addAction(toggleAklogAct);

    hasCurrentTickets();
	
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(ticketTimerEvent()));
    timer->start(1000);

    LOG << "Using Kerberos KRB5CCNAME of" << cc.name().c_str();
    LOG << "kredentials constructor returning";
}

kredentials::~kredentials()
{
}

void kredentials::prefsAklog() {
    doAklog = !doAklog;
    KConfigGroup(config, "General").writeEntry("RunAklog", doAklog);
    config->sync();
    LOG << "Toggled aklog, value now"<<doAklog;
}

bool kredentials::destroyTickets(){
    bool res=FALSE;
    if(!(res=tixmgr::destroyTickets())){
        QMessageBox::warning(dynamic_cast<QWidget*>(this), title, tr("Unable to destroy your tickets."));
    }else{
        QMessageBox::information(dynamic_cast<QWidget*>(this), title, tr("Your tickets have been destroyed."));
    }
    return res;
}

void kredentials::tryPassGetTickets(){
    QString password;
    QString prompt = QString("Please give the password for ");
    std::shared_ptr<krb5::principal> osMe(NULL);
    krb5::principal* pme=cc.getPrincipal();
    if(!pme){
        osMe.reset(osPrincipal());
        pme=osMe.get();
    }
    if(pme){
        prompt.append(QString(pme->getName().c_str()));
    }else{
        prompt.append(QString("unknown user"));
    }
    timer->stop();
    LOG << "Getting Pass";

    KPasswordDialog *dlg = new KPasswordDialog(NULL, 0);
    dlg->setPrompt(prompt);
    if(!dlg->exec()) {
        // User hit cancel, or something
        return;
    }

    while (1) { // infinite loop until receives a valid password
        std::string pass(dlg->password().toLatin1());
        LOG << "Getting Creds";
        bool res=passGetCreds(pass);
        LOG << "Finished Creds";
        if(!res){
            QMessageBox::warning(dynamic_cast<QWidget*>(this), title, tr("Your password was probably wrong"));
            break; // I'll try again
        }else{
            hasCurrentTickets();
            if( !runAklog() ){
                QMessageBox::warning(dynamic_cast<QWidget*>(this), title, tr("Unable to run aklog"));
            }
            timer->start(1000);
            //that's it
            break;
        }
    }

    delete dlg;
    return;
}

void kredentials::tryPassGetTicketsScreenSaverSafe(){
	// check if screen saver is active (blanked)
	// if so, don't prompt the password dialog this time
	// and don't try to get new tickets, either the screen
	// saver unlock will do it, either it will pop-up once
	// screen saver will be deactivated

// XXX TEMPORARILY disable dcop stuff while porting to KDE4
#if 0
	DCOPRef screensaver("kdesktop", "KScreensaverIface");
	DCOPReply reply = screensaver.call("isBlanked");

	if (!reply.isValid())
	{
	    LOG << "There is some error using DCOP to access screensaver status";
	} else if (reply) {
		// screen saver is running
		return;
	}
#endif

	tryPassGetTickets();
}

void kredentials::tryRenewTickets(){
    time_t now = time(0);
    timer->stop();

    if(!hasCurrentTickets()){
        tryPassGetTicketsScreenSaverSafe();
    }else if(tktRenewableExpirationTime == 0){
        // can not be renewed
        // check if it will expire soon
        if ( tktExpirationTime - now < renewWarningTime)
            tryPassGetTicketsScreenSaverSafe();
    }else if(tktRenewableExpirationTime < now){
        QMessageBox::information(dynamic_cast<QWidget*>(this), title,
                                 tr("Your tickets have outlived their renewable lifetime and can't be renewed."));
        LOG << "tktRenewableExpirationTime has passed:"
            << "tktRenewableExpirationTime = " <<
            tktRenewableExpirationTime << ", now = " << now;
        tryPassGetTicketsScreenSaverSafe();
    }else if(!renewTickets()){
        LOG << "renewTickets did not get new tickets";

        if(!hasCurrentTickets()){
            tryPassGetTicketsScreenSaverSafe();
        }
    }else{
        if(doNotify){
            showMessage("kredentials",
                "Successfully renewed Kerberos tickets",
                QSystemTrayIcon::Information,
                3000);
        }
    }
    // restart the timer here, regardless of whether we currently
    // have tickets now or not.  The user may get tickets before
    // the next timeout, and we need to be able to renew them
    secondsToNextRenewal = DEFAULT_RENEWAL_INTERVAL;
    timer->start(1000);

    if(authenticated > 0){
        if( !runAklog() ){
            QMessageBox::warning(dynamic_cast<QWidget*>(this), title, tr("Unable to run aklog"));
        }

        LOG << "WarnTime: " << renewWarningTime << doNotify;
        if(doNotify &&
           tktRenewableExpirationTime - now < renewWarningTime &&
           tktRenewableExpirationTime!=0)
        {
            LOG << "Renew=" << renewWarningFlag;
            if(renewWarningFlag == 0) {
                renewWarningFlag = 1;
                LOG << "RESET: Renew=" << renewWarningFlag;

                QString msgString =
                    QString("Kerberos tickets will permanently expire on ")
                    +  QString(ctime(&tktRenewableExpirationTime)) +
                    QString(" You may want to renew them now.");
                QMessageBox::information(dynamic_cast<QWidget*>(this), title, msgString);
            }

        }else{
            renewWarningFlag = 0;
            LOG << "RESET: Renew=" << renewWarningFlag;
        }
    }
    return;
}

void kredentials::ticketTimerEvent(){
    LOG << "ticketTimerEvent triggered, secondsToNextRenewal ==" 
	<< secondsToNextRenewal;

    --secondsToNextRenewal;
    if(secondsToNextRenewal < 0) {
        tryRenewTickets();
    }
    return;
}

void kredentials::showTicketCache(){
    hasCurrentTickets();
    QString msgString;
	
    if(!authenticated){
        QMessageBox::information(dynamic_cast<QWidget*>(this), title, tr("You do not have any valid tickets."));
    }else{
        const krb5::principal* pme=cc.getPrincipal();
        if(pme){
            const krb5::principal& me=*pme;
            if(me.getDataLength()){
                msgString = QString("Your tickets as ")
                    +QString(me.getName().c_str())+QString(" ");
            }else{
                msgString = QString("Your tickets ");
            }
        }

        msgString+=QString("are\n Valid until ") +
            QString(ctime(&tktExpirationTime));
        if(tktRenewableExpirationTime > time(0)) {
            msgString += QString("\nRenewable until ") +
            QString(ctime(&tktRenewableExpirationTime));
        }else{
            msgString += QString("\nTickets are not renewable");
        }

        //QMessageBox(this, QMessageBox::Information, "Kerberos", msgString);
        QMessageBox::information(dynamic_cast<QWidget*>(this), title, msgString);
    }
    return;
}

void kredentials::setDoNotify(int state){
    doNotify = state;
}
