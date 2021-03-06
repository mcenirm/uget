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

/*
 *	XML-RPC for C Language, it base on GLib.
 *
 *	UgXmlrpc			xmlrpc;
 *	UgXmlrpcValue*		value;
 *	UgXmlrpcResponse	response;
 *
 *	ug_xmlrpc_init (&xmlrpc);
 *
 *	ug_xmlrpc_use_client (&xmlrpc, "http://localhost:8080/RPC", "Agent/1.0");
 *
 *	response = ug_xmlrpc_call (&xmlrpc,  "methodName"
 *			UG_XMLRPC_INT,    5678,
 *			UG_XMLRPC_STRING, "sample",
 *			UG_XMLRPC_NONE);
 *
 *	if (response != UG_XMLRPC_ERROR)
 *		value = ug_xmlrpc_get_value (&xmlrpc);
 *
 *	ug_xmlrpc_finalize (&xmlrpc);
 *
 */

#ifndef UG_XMLRPC_H
#define UG_XMLRPC_H

#include <stdarg.h>
#include <memory.h>
#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct	UgXmlrpc			UgXmlrpc;
typedef struct	UgXmlrpcValue		UgXmlrpcValue;
typedef union	UgXmlrpcValueC		UgXmlrpcValueC;
typedef enum	UgXmlrpcType		UgXmlrpcType;
typedef enum	UgXmlrpcResponse	UgXmlrpcResponse;

// XML-RPC types
// If no type is indicated, the type is string.
enum UgXmlrpcType
{
	UG_XMLRPC_NONE,

	UG_XMLRPC_INT,			// <i4> or <int>		// int
	UG_XMLRPC_INT64,		// <i8>					// long long
	UG_XMLRPC_BOOLEAN,		// <boolean>			// int
	UG_XMLRPC_STRING,		// <string>				// char*
	UG_XMLRPC_DOUBLE,		// <double>				// double
//	UG_XMLRPC_DATETIME,		// <dateTime.iso8601>	// time_t
	UG_XMLRPC_DATETIME,		// <dateTime.iso8601>	// char*
	UG_XMLRPC_BINARY,		// <base64>				// unsigned char*

	// XML-RPC extension, such as
	// Python's None object, Perl's undef value, or a NULL pointer in C.
	UG_XMLRPC_NIL,			// <nil>				// NULL

	UG_XMLRPC_ARRAY,		// <array>				// UgXmlrpcValue*
	UG_XMLRPC_STRUCT,		// <struct>				// UgXmlrpcValue*
	UG_XMLRPC_PARAMS,		// <params>				// UgXmlrpcValue*
};

enum UgXmlrpcResponse
{
	UG_XMLRPC_OK,
	UG_XMLRPC_FAULT,
	UG_XMLRPC_ERROR,	// HTTP header or data has error
};


// ----------------------------------------------------------------------------
// UgXmltag: simple parser for XML-RPC
//
typedef struct	UgXmltag		UgXmltag;
typedef void	(*UgXmltagFunc)	(UgXmltag* xmltag, gpointer user_data);

struct UgXmltag
{
	// beg        end  next
	//  v          v    v
	// <elementName>text<nextElement>
	const char*		beg;
	const char*		end;
	const char*		next;

	GString*		buffer;

	struct
	{
		gpointer*		data;
		guint			len;
		guint			allocated;
	} parser;

	struct
	{
		gpointer	storage;
		gpointer	position;
	} user;
};

void	ug_xmltag_init		(UgXmltag* xmltag);
void	ug_xmltag_finalize	(UgXmltag* xmltag);

gboolean	ug_xmltag_parse (UgXmltag* xmltag, const gchar* string, int len);
gboolean	ug_xmltag_clear	(UgXmltag* xmltag);

void		ug_xmltag_push  (UgXmltag* xmltag, UgXmltagFunc func, gpointer data);
void		ug_xmltag_pop   (UgXmltag* xmltag);

#define		ug_xmltag_text(xmltag)		((xmltag)->end+1)
#define		ug_xmltag_text_len(xmltag)	((xmltag)->next - ((xmltag)->end+1) )
#define		ug_xmltag_len(xmltag)		((xmltag)->end  -  (xmltag)->beg )


// ----------------------------------------------------------------------------
// UgXmlrpcValueC: XML-RPC value for C Language
//
union UgXmlrpcValueC
{
	int				int_;		// UG_XMLRPC_INT
	gint64			int64;		// UG_XMLRPC_INT64
	int				boolean;	// UG_XMLRPC_BOOLEAN
	char*			string;		// UG_XMLRPC_STRING
	double			double_;	// UG_XMLRPC_DOUBLE
//	time_t			datetime;	// UG_XMLRPC_DATETIME
	char*			datetime;	// UG_XMLRPC_DATETIME
	unsigned char*	binary;		// UG_XMLRPC_BINARY

	// Balanced Binary Trees used for UG_XMLRPC_STRUCT only
	// This is not XML-RPC type.
	GTree*			tree;
};


// ----------------------------------------------------------------------------
// UgXmlrpcValue: XML-RPC value
//
struct UgXmlrpcValue
{
	UgXmlrpcType	type;
	UgXmlrpcValueC	c;

	// used for member of UG_XMLRPC_STRUCT
	gchar*			name;

	// used with UG_XMLRPC_ARRAY and UG_XMLRPC_STRUCT
	// UG_XMLRPC_BINARY use len with c.binary
	UgXmlrpcValue*	data;
	guint			len;
	guint			allocated;
};

UgXmlrpcValue*	ug_xmlrpc_value_new   (void);
void			ug_xmlrpc_value_free  (UgXmlrpcValue* value);

//void			ug_xmlrpc_value_init  (UgXmlrpcValue* value);
#define			ug_xmlrpc_value_init(value)		memset(value, 0, sizeof (UgXmlrpcValue))

void			ug_xmlrpc_value_clear (UgXmlrpcValue* value);

// below functions are used with UG_XMLRPC_ARRAY and UG_XMLRPC_STRUCT
UgXmlrpcValue*	ug_xmlrpc_value_alloc (UgXmlrpcValue* value);
UgXmlrpcValue*	ug_xmlrpc_value_find  (UgXmlrpcValue* value, const gchar* name);

UgXmlrpcValue*	ug_xmlrpc_value_new_data (UgXmlrpcType type, guint preallocated_size);
#define			ug_xmlrpc_value_new_array(size)		ug_xmlrpc_value_new_data (UG_XMLRPC_ARRAY,  size)
#define			ug_xmlrpc_value_new_struct(size)	ug_xmlrpc_value_new_data (UG_XMLRPC_STRUCT, size)
#define			ug_xmlrpc_value_new_params(size)	ug_xmlrpc_value_new_data (UG_XMLRPC_PARAMS, size)
#define			ug_xmlrpc_value_at(value, index)	(&(value)->data[index])


// ----------------------------------------------------------------------------
// UgXmlrpc: XML-RPC
//
struct UgXmlrpc
{
//	CURL*			curl;
	gpointer		curl;
	gpointer		curl_slist;

	char*			uri;
	char*			user_agent;

	GString*			buffer;
	GStringChunk*		chunk;
	UgXmltag			xmltag;
	UgXmlrpcValue		value;
	guint				index;
	UgXmlrpcResponse	response;
};

void	ug_xmlrpc_init     (UgXmlrpc* xmlrpc);
void	ug_xmlrpc_finalize (UgXmlrpc* xmlrpc);

void	ug_xmlrpc_use_client (UgXmlrpc* xmlrpc, const gchar* uri, const gchar* user_agent);

// ----------------------------------------------------------------------------
// functions used to write UgXmlrpc.buffer
//
//
//	UgXmlrpcValue*	xrArray  = ug_xmlrpc_value_new_array (0);
//	UgXmlrpcValue*	xrStruct = ug_xmlrpc_value_new_struct (0);
//	UgXmlrpcValue*	xrBinary = ug_xmlrpc_value_new ();
//
//	xrBinary->type     = UG_XMLRPC_BINARY;
//	xrBinary->c.binary = binary;
//	xrBinary->len      = binary_length;
//
//	ug_xmlrpc_call (xmlrpc,   "methodName",
//			UG_XMLRPC_INT,    9876,
//			UG_XMLRPC_DOUBLE, 0.65,
//			UG_XMLRPC_STRING, "sample",
//			UG_XMLRPC_BINARY, xrBinary,
//			UG_XMLRPC_ARRAY,  xrArray,
//			UG_XMLRPC_STRUCT, xrStruct,
//			UG_XMLRPC_NIL,    NULL,
//			UG_XMLRPC_NONE);
//
UgXmlrpcResponse	ug_xmlrpc_call (UgXmlrpc* xmlrpc, const gchar* methodName, ...);

// ----------------------------------------------------------------------------
// functions used to parse UgXmlrpc.buffer
//
UgXmlrpcValue*		ug_xmlrpc_get_value (UgXmlrpc* xmlrpc);


#ifdef __cplusplus
}
#endif

#endif  // UG_XMLRPC_H

