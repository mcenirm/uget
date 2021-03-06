/*
 *
 *   Copyright (C) 2005-2014 by C.H. Huang
 *   plushuang.tw@gmail.com
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  ---
 *
 *  In addition, as a special exception, the copyright holders give
 *  permission to link the code of portions of this program with the
 *  OpenSSL library under certain conditions as described in each
 *  individual source file, and distribute linked combinations
 *  including the two.
 *  You must obey the GNU Lesser General Public License in all respects
 *  for all of the code used other than OpenSSL.  If you modify
 *  file(s) with this exception, you may extend this exception to your
 *  version of the file(s), but you are not obligated to do so.  If you
 *  do not wish to do so, delete this exception statement from your
 *  version.  If you delete this exception statement from all source
 *  files in the program, then also delete it here.
 *
 */


#ifndef UG_PROXY_FORM_H
#define UG_PROXY_FORM_H

#include <gtk/gtk.h>
#include <UgDataset.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif


typedef struct	UgProxyForm			UgProxyForm;

struct UgProxyForm
{
	GtkWidget*		self;		// top level widget

	GtkWidget*		type;		// GtkComboBox

	// classic
//	struct UgProxyFormStd {
	GtkWidget*		std;
	// proxy server
	GtkWidget*		host;		// GtkEntry
	GtkWidget*		port;		// GtkEntry
	// authentication
	GtkWidget*		user;		// GtkEntry
	GtkWidget*		password;	// GtkEntry
//	} std;

	// User changed entry
	struct UgProxyFormChanged
	{
		gboolean		enable:1;
		// UgProxyFormStd
		gboolean		type:1;
		gboolean		host:1;
		gboolean		port:1;
		gboolean		user:1;
		gboolean		password:1;
	} changed;


//#ifdef HAVE_LIBPWMD
	struct UgProxyFormPwmd
	{
		GtkWidget*		self;

		GtkWidget*		socket;
	        GtkWidget*		socket_args;
		GtkWidget*		file;
		GtkWidget*		element;

		// User changed entry
		struct UgProxyFormPwmdChanged {
			gboolean		socket:1;
		        gboolean		socket_args:1;
			gboolean		file:1;
			gboolean		element:1;
		} changed;
	} pwmd;
//#endif	// HAVE_LIBPWMD
};

void	ug_proxy_form_init (UgProxyForm* pform);
void	ug_proxy_form_get  (UgProxyForm* pform, UgDataset* dataset);
void	ug_proxy_form_set  (UgProxyForm* pform, UgDataset* dataset, gboolean keep_changed);


#ifdef __cplusplus
}
#endif

#endif  // End of UG_PROXY_FORM_H

