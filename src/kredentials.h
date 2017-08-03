/*
 * Copyright 2004,2006,2007,2011 by the Massachusetts Institute of
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

#ifndef _KREDENTIALS_H_
#define _KREDENTIALS_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "krb5_wrap.h"
#include <time.h>

#include <QAction>
#include <QDebug>
#include <QMessageBox>
#include <QSystemTrayIcon>
#include <QTimer>

#include <kconfiggroup.h>
#include <kuser.h>
#include <KSharedConfig>

/**
 * @short Application Main Window
 * @author Noah Meyerhans <noahm@csail.mit.edu>
 * @version 0.9.2
 */
class kredentials : public QSystemTrayIcon, public krb5::tixmgr
{
    Q_OBJECT
public:
    /**
     * Constructor
     */
	kredentials( int doNotify=0 );
	
    /**
     * Destructor
     */
    virtual ~kredentials();

	void setDoNotify(int);
	
protected slots:
	void tryRenewTickets();
	void showTicketCache();
	void tryPassGetTickets();
	void tryPassGetTicketsScreenSaverSafe();
	bool destroyTickets();
	void ticketTimerEvent();
	void prefsAklog();

protected:
	void renewOrGetNewTicekts();

private:
	int secondsToNextRenewal;
    int renewWarningFlag;
    int renewWarningTime;
    KSharedConfigPtr config;
    KConfigGroup generalConfigGroup;

	KUser *kerberosUser;
    QAction *renewAct, *reInitAct, *statusAct, *destroyAct, *freshTixAct;
    QAction *quitAct;
    QAction *toggleAklogAct;
	QPixmap panelIcon;
	QTimer *timer;
    QMenu * ctxMenu;
    const QString title = "Kerberos";

	int doNotify;
};

#endif // _KREDENTIALS_H_
