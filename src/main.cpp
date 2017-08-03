/*
 * Copyright 2004,2006,2011 by the Massachusetts Institute of
 * Technology.  All Rights Reserved.
 * Copyright 2007, Martin Ginkel <mginkel@mpi-magdeburg.mpg.de>
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

#include <KAboutData>
#include <KDBusService>

#include <QApplication>
#include <QCommandLineParser>
#include <QObject>

static const char version[] = "2.0-alpha";

int main(int argc, char **argv)
{
	QApplication app(argc, argv);

    app.setOrganizationDomain("org.kde");
    app.setDesktopFileName(QString("org.kde.kredentials.desktop"));

    KDBusService service(KDBusService::Unique);

    KAboutData about(QObject::tr("kredentials"),
        QObject::tr("kredentials"),
		version,
        QObject::tr("Monitor and update authentication tokens"),
        KAboutLicense::Custom,
        QObject::tr("(C) 2004 Noah Meyerhans"));

    about.addAuthor( QObject::tr("Noah Meyerhans"), QObject::tr("developer"),
		"noahm@csail.mit.edu", 0 );

    KAboutData::setApplicationData(about);

	QCommandLineParser parser;
    parser.setApplicationDescription(QObject::tr("Monitor and update authentication tokens"));
	parser.addHelpOption();
	parser.addVersionOption();
	
    QCommandLineOption informOption(QStringList() << "i" << "inform", "Inform the user when credentials are renewed");
	parser.addOption(informOption);
    QCommandLineOption disableAklogOption(QStringList() << "d" << "disable-aklog", "Don't run aklog after renewing Kerberos tickets");
	parser.addOption(disableAklogOption);

    about.setupCommandLine(&parser);
	parser.process(app);
    about.processCommandLine(&parser);

	// try to bind to the dbus name and if it fails return 0
	// TODO
	//if (!KUniqueApplication::start()) {
	//	kError() << "Kredentials is already running!";
	//	exit(0);
    //}

	kredentials* k_obj = new kredentials();
    if(parser.isSet(informOption)){
		k_obj->setDoNotify(true);
	}

    if(parser.isSet(disableAklogOption)) {
		k_obj->setDoAklog(false);
	}

    app.setQuitOnLastWindowClosed(false);

    k_obj->show();

    return app.exec();
}

