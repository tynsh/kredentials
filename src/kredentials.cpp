/*
 * Copyright 2004 by the Massachusetts Institute of Technology.
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
 */

#include "kredentials.h"

#include <qlabel.h>
#include <qcursor.h>
#include <qevent.h>

#include <kapplication.h>
#include <kmainwindow.h>
#include <klocale.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <kpassivepopup.h>

#include <krb5.h>

#include <time.h>   /* for time() */
#include <stdlib.h> /* for system() */

#ifdef DEBUG
#define DEFAULT_RENEWAL_INTERVAL 20
#else
#define DEFAULT_RENEWAL_INTERVAL 3600
#endif /*DEBUG*/

kredentials::kredentials()
    : KSystemTray()
{
    // set the shell's ui resource file
    //setXMLFile("kredentialsui.rc");

#ifdef DEBUG
	kdDebug() << "kredentials constructor called" << endl;
#endif /* DEBUG */
	doNotify = 0;
	doAklog  = 1;
	secondsToNextRenewal = DEFAULT_RENEWAL_INTERVAL;
	this->setPixmap(this->loadIcon("panel"));
	menu = new QPopupMenu();
	//menu->insertItem("Renew Tickets", this, SLOT(renewTickets()), CTRL+Key_R);
	//menu->insertItem("Exit", i18n("Quit"), KApplication::kApplication(), SLOT(quit()));
	renewAct = new KAction(i18n("&Renew credentials"), "1rightarrow", 0,
                               this, SLOT(renewTickets()), actionCollection(), "renew");
	
	renewAct->plug(menu);
	statusAct = new KAction(i18n("&Credential Status"), "", 0, this, SLOT(showTicketCache()), actionCollection(), "status");
	statusAct->plug(menu);
	menu->insertItem(SmallIcon("exit"), i18n("Quit"), kapp, SLOT(quit()));
		
	initKerberos();
	hasCurrentTickets();
	
	killTimers();
	startTimer(1000);
#ifdef DEBUG
	kdDebug() << "Using Kerberos KRB5CCNAME of " << krb5_cc_get_name(ctx, cc) << endl;
	kdDebug() << "kredentials constructor returning" << endl;
#endif /* DEBUG */
}

kredentials::~kredentials()
{
}

void kredentials::initKerberos()
{
	kerror = 0;
	kerror = krb5_init_context(&ctx);
	if(kerror)
		kdDebug() << "Kerberos returned " << kerror << endl;
	kerror = krb5_cc_default(ctx, &cc);
	if(kerror)
		kdDebug() << "Kerberos returned " << kerror << endl;
	kdDebug() << "Set cc to " << krb5_cc_get_name(ctx, cc) << endl;
	kerror = krb5_cc_get_principal(ctx, cc, &me);
	if(kerror)
	{
		kdDebug() << "Kerberos returned " << kerror << endl;
	}
	
	return;
}

void kredentials::mousePressEvent(QMouseEvent *e)
{
	if(e->button() == RightButton)
	{
		menu->popup(QCursor::pos());
	}
}

int kredentials::renewTickets()
{
		krb5_creds my_creds;
		krb5_error_code code = 0;
		krb5_get_init_creds_opt options;
		//const char *err_mesg;

		// Can't renew tickets if we don't have any to renew
		if(!authenticated)
			return -1;
			
		krb5_get_init_creds_opt_init(&options);
		memset(&my_creds, 0, sizeof(my_creds));
		
#ifdef DEBUG
		kdDebug() << "renewing tickets for " << me->data->data << "@" << me->realm.data << endl;
#endif /* DEBUG */

		kerror = krb5_get_renewed_creds(ctx, &my_creds, me, cc,
								NULL);

		if(kerror)
		{
				kdDebug() << "Kerberos returned " << kerror << 
					" while renewing creds" << endl;
				return kerror;
		}
		kerror = krb5_cc_initialize(ctx, cc, me);
		if(kerror)
		{
				kdDebug() << "Kerberos returned " << kerror << 
					" while initializing cred cache" << endl;
				return code;
		}
		kerror = krb5_cc_store_cred(ctx, cc, &my_creds);
		if(kerror)
		{
				kdDebug() << "Kerberos returned " << kerror << 
					" while storing credentials" << endl;
				return kerror;
		}
#ifdef DEBUG
		kdDebug() << "Successfully renewed tickets" << endl;
#endif
		if(doNotify)
		{
			KPassivePopup::message("Kerberos tickets have been renewed", 0);
		}

		if(doAklog)
		{
			int aklogResult = system("aklog");
			if(aklogResult)
				KMessageBox::error(0, "Unable to get new AFS tokens.", 0);
		}
		return 0;
}

void kredentials::hasCurrentTickets()
{
	int noTix = 1;
	krb5_cc_cursor cur;
	krb5_creds creds;
	krb5_principal princ;
	krb5_int32 now;
	
		
#ifdef DEBUG
	kdDebug() << "Called hasCurrentTickets()" << endl;
#endif /* DEBUG */

	/* if kerberos is not currently happy, try reinitializing.  The user may 
	   have obtained new tickets since we last initialized.
	*/
	if(kerror)
	{
#ifdef DEBUG
		kdDebug() << "hasCurrentTickets(): kerror = " << kerror << endl;
		kdDebug() << "Trying to reinitialize kerberos..." << endl;
#endif /* DEBUG */
		initKerberos();
		if(kerror)
		{
			/* If kerberos is still unhappy, we are not authenticated and can
			   return now. */
			authenticated = 0;
			return;
		}
	}
	
	memset(&cur, 0, sizeof(cur));
	memset(&princ, 0, sizeof(princ));

	now = time(0);
	
	if(kerror = krb5_cc_get_principal(ctx, cc, &princ))
	{
		kdDebug() << "While retrieving principal name, Kerberos returned " << kerror << endl;
		authenticated = 0;
		return;
	}
	kerror = krb5_cc_start_seq_get(ctx, cc, &cur);
	if(kerror)
	{
		kdDebug() << "While beginning CC iterations, kerberos returned " << kerror << endl;
		authenticated = 0;
		return;
	}
	
	while (!(kerror = krb5_cc_next_cred(ctx, cc, &cur, &creds)))
	{
		if (noTix && creds.server->length == 2 &&
			strcmp(creds.server->realm.data, princ->realm.data) == 0 &&
			strcmp((char *)creds.server->data[0].data, "krbtgt") == 0 &&
			strcmp((char *)creds.server->data[1].data,
			princ->realm.data) == 0 &&
			creds.times.endtime > now)
		{
			noTix = 0;
			tktExpirationTime = creds.times.endtime;
			tktRenewableExpirationTime = creds.times.renew_till;
		}
	}
	noTix == 0 ? authenticated = 1 : authenticated = 0;

#ifdef DEBUG
	kdDebug() << "hasCurrentTickets set authenticated=" << authenticated << endl;
#endif /* DEBUG */

	return;
}

void kredentials::timerEvent(QTimerEvent *e)
{
#ifdef DEBUG
	kdDebug() << "timerEvent triggered, secondsToNextRenewal == " << secondsToNextRenewal << endl;
#endif /* DEBUG */
	time_t now = time(0);
	secondsToNextRenewal--;
	if(secondsToNextRenewal < 0)
	{
		killTimers();
		if((tktRenewableExpirationTime < now) || (renewTickets() != 0))
		{
#ifdef DEBUG
				kdDebug() << "renewTickets did not get new tickets" << endl;
#endif /* DEBUG */
			hasCurrentTickets();
			if(authenticated == 0)
			{
				KMessageBox::information(0, "Your tickets have expired. Please run 'renew' in a shell.", "Kerberos", 0, 0);
			}
		}

		// restart the timer here, regardless of whether we currently have tickets now or not.
		// The user may get tickets before the next timeout, and we need to be able to renew them
		secondsToNextRenewal = DEFAULT_RENEWAL_INTERVAL;
		startTimer(1000);
	}
	else if(authenticated &&
			((now - tktExpirationTime) < 3600) &&
			((authenticated % 900) == 0))
	{
		// tickets expire in less than 1 hour
		KPassivePopup::message("Kerberos tickets expire in less than one hour.  You may wish to renew soon.", 0);
	}
	return;
}


void kredentials::showTicketCache()
{
	hasCurrentTickets();
	QString msgString;
	
	if(!authenticated)
	{
		KMessageBox::information(0, "You do not have any valid tickets.", "Kerberos", 0, 0);
	}
	else
	{
		msgString = QString("Your tickets are valid until ") + QString(ctime(&tktExpirationTime));
		if(tktRenewableExpirationTime > time(0))
		{
			msgString += QString("\nRenewable until ") + QString(ctime(&tktRenewableExpirationTime));
		}
		else
		{
			msgString += QString("\nTickets are not renewable");
		}
		KMessageBox::information(0, msgString, "Kerberos", 0, 0);
	}
	return;
}

void kredentials::setNotify(int state)
{
	doNotify = state;
}

#include "kredentials.moc"
