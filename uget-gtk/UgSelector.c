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

#include <UgDataset.h>
#include <UgetData.h>
#include <UgSelector.h>
#include <UgUri.h>
#include <UgSetting.h>		// UG_APP_GTK_NAME

#include <glib/gi18n.h>


// ----------------------------------------------------------------------------
// UgSelectorItem
//
typedef struct	UgSelectorItem		UgSelectorItem;

struct UgSelectorItem
{
	gboolean	mark;
	gchar*		uri;
	UgDataset*	dataset;
};

// GtkListStore for UgSelectorItem
static GList*	ug_selector_store_get_marked   (GtkListStore* store, GList* list);
static void		ug_selector_store_set_mark_all (GtkListStore* store, gboolean mark);
static void		ug_selector_store_clear        (GtkListStore* store);
// GtkTreeView for UgSelectorItem
static GtkTreeView*		ug_selector_view_new (const gchar* title, gboolean active_toggled);
static GtkCellRenderer*	ug_selector_view_get_renderer_toggle (GtkTreeView* view);


// ----------------------------------------------------------------------------
// UgSelector
//
static void	ug_selector_filter_init (struct UgSelectorFilter* filter, UgSelector* selector);
static void	ug_selector_filter_show (struct UgSelectorFilter* filter, UgSelectorPage* page);
// signal handlers
static void	on_selector_item_toggled (GtkCellRendererToggle* cell, gchar* path_str, UgSelector* selector);
static void	on_selector_mark_all    (GtkWidget* button, UgSelector* selector);
static void	on_selector_mark_none   (GtkWidget* button, UgSelector* selector);
static void	on_selector_mark_filter (GtkWidget* button, UgSelector* selector);
static void	on_filter_dialog_response (GtkDialog* dialog, gint response_id, UgSelector* selector);
static void	on_filter_button_all  (GtkWidget* widget, GtkTreeView* treeview);
static void	on_filter_button_none (GtkWidget* widget, GtkTreeView* treeview);

void	ug_selector_init (UgSelector* selector, GtkWindow* parent)
{
	GtkBox*		vbox;
	GtkBox*		hbox;
	GtkWidget*	widget;
	gchar*		string;

	selector->parent = parent;
	selector->self = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
	vbox = (GtkBox*) selector->self;

	hbox = (GtkBox*) gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
	gtk_box_pack_start (vbox, (GtkWidget*) hbox, FALSE, FALSE, 1);
	string = g_strconcat (_("Base hypertext reference"), " <base href> :", NULL);
	selector->href_label = gtk_label_new (string);
	g_free (string);
	gtk_box_pack_start (hbox, selector->href_label, FALSE, TRUE, 1);

	selector->href_entry = (GtkEntry*) gtk_entry_new ();
	gtk_box_pack_start (vbox, (GtkWidget*) selector->href_entry, FALSE, FALSE, 1);
	selector->href_separator = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
	gtk_box_pack_start (vbox, selector->href_separator, FALSE, FALSE, 1);

	selector->notebook = (GtkNotebook*) gtk_notebook_new ();
	gtk_box_pack_start (vbox, (GtkWidget*) selector->notebook, TRUE, TRUE, 1);

	hbox = (GtkBox*) gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
	gtk_box_pack_start (vbox, (GtkWidget*) hbox, FALSE, FALSE, 1);
	// select all
	widget = gtk_button_new_with_mnemonic (_("Mark _All"));
	g_signal_connect (widget, "clicked",
			G_CALLBACK (on_selector_mark_all), selector);
	gtk_box_pack_start (hbox, widget, TRUE, TRUE, 1);
	selector->select_all = widget;
	// select none
	widget = gtk_button_new_with_mnemonic (_("Mark _None"));
	g_signal_connect (widget, "clicked",
			G_CALLBACK (on_selector_mark_none), selector);
	gtk_box_pack_start (hbox, widget, TRUE, TRUE, 1);
	selector->select_none = widget;
	// select by filter
	widget = gtk_button_new_with_mnemonic (_("_Mark by filter..."));
	g_signal_connect (widget, "clicked",
			G_CALLBACK (on_selector_mark_filter), selector);
	gtk_box_pack_start (hbox, widget, TRUE, TRUE, 1);
	selector->select_filter = widget;

	gtk_widget_show_all ((GtkWidget*) vbox);

	// UgSelectorPage initialize
	selector->pages = g_array_new (FALSE, FALSE, sizeof (UgSelectorPage));

	// UgSelectorFilter initialize
	ug_selector_filter_init (&selector->filter, selector);
}

void	ug_selector_finalize (UgSelector* selector)
{
	UgSelectorPage*	page;
	GArray*			array;
	guint			index;

	// UgSelectorPage finalize
	array = selector->pages;
	for (index=0; index < array->len; index++) {
		page = &g_array_index (array, UgSelectorPage, index);
		ug_selector_page_finalize (page);
	}
	g_array_free (selector->pages, TRUE);

	// UgSelectorFilter finalize
	gtk_widget_destroy (GTK_WIDGET (selector->filter.dialog));
}

void	ug_selector_hide_href (UgSelector* selector)
{
	gtk_widget_hide ((GtkWidget*) selector->href_label);
	gtk_widget_hide ((GtkWidget*) selector->href_entry);
	gtk_widget_hide ((GtkWidget*) selector->href_separator);
}

static GList*	ug_selector_get_marked (UgSelector* selector)
{
	UgSelectorPage*	page;
	GList*			list;
	guint			index;

	list = NULL;
	for (index = 0;  index < selector->pages->len;  index++) {
		page = &g_array_index (selector->pages, UgSelectorPage, index);
		list = ug_selector_store_get_marked (page->store, list);
	}
	return g_list_reverse (list);
}

GList*	ug_selector_get_marked_downloads (UgSelector* selector)
{
	UgSelectorItem*	item;
	UgDataset*		dataset;
	UgetCommon*	common;
	GString*		gstr;
	GList*			list;
	GList*			link;
	const gchar*	base_href;

	list = ug_selector_get_marked (selector);
	base_href = gtk_entry_get_text (selector->href_entry);
	if (base_href[0] == 0)
		base_href = NULL;
	for (link = list;  link;  link = link->next) {
		item = link->data;
		// UgDataset list
		if (item->dataset) {
			dataset = item->dataset;
			ug_dataset_ref (dataset);
			link->data  = dataset;
//			item->uri = NULL;
			continue;
		}
		// URI list
		if (ug_uri_scheme_len (item->uri) == 0 && base_href) {
			gstr = g_string_new (base_href);
			if (gstr->str[gstr->len -1] == '/') {
				if (item->uri[0] == '/')
					g_string_truncate (gstr, gstr->len -1);
			}
			else if (item->uri[0] != '/')
				g_string_append_c (gstr, '/');
			g_string_append (gstr, item->uri);
		}
		else
			gstr = g_string_new (item->uri);
		dataset = ug_dataset_new ();
		common = ug_dataset_realloc (dataset, UgetCommonInfo, 0);
		common->url = g_string_free (gstr, FALSE);
		common->keeping.url = TRUE;
		link->data = dataset;
	}
	return list;
}

gint	ug_selector_count_marked (UgSelector* selector)
{
	gint			count;
	guint			index;

	count = 0;
	for (index = 0;  index < selector->pages->len;  index++) {
		count += g_array_index (selector->pages, UgSelectorPage, index).n_marked;
//		count += page->n_marked;
	}
	if (selector->notify.func)
		selector->notify.func (selector->notify.data, (count) ? TRUE: FALSE);
	return count;
}

UgSelectorPage*	ug_selector_add_page (UgSelector* selector, const gchar* title)
{
	GtkCellRenderer*	renderer;
	UgSelectorPage*		page;
	GArray*				array;

	array = selector->pages;
	g_array_set_size (array, array->len + 1);
	page = &g_array_index (array, UgSelectorPage, array->len - 1);
	ug_selector_page_init (page);
	renderer = ug_selector_view_get_renderer_toggle (page->view);
	g_signal_connect (renderer, "toggled",
			G_CALLBACK (on_selector_item_toggled), selector);

	gtk_notebook_append_page (selector->notebook, page->self, gtk_label_new (title));
	return page;
}

UgSelectorPage*	ug_selector_get_page (UgSelector* selector, gint nth_page)
{
	UgSelectorPage*		page;

	if (nth_page < 0)
		nth_page = gtk_notebook_get_current_page (selector->notebook);
	if (nth_page == -1 || nth_page >= (gint)selector->pages->len)
		return NULL;
	page = &g_array_index (selector->pages, UgSelectorPage, nth_page);
	return page;
}


// ----------------------------------------------------------------------------
// UgSelectorFilter use UgSelectorFilterData in UgSelectorPage
//
static GtkWidget*	ug_selector_filter_view_init (GtkTreeView* item_view)
{
	GtkSizeGroup*	sizegroup;
	GtkWidget*		widget;
	GtkBox*			vbox;
	GtkBox*			hbox;

	vbox = (GtkBox*) gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
	// filter view and it's scrolled window
	gtk_widget_set_size_request ((GtkWidget*) item_view, 120, 120);
	widget = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (widget),
			GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (widget),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (widget), GTK_WIDGET (item_view));
	gtk_box_pack_start (vbox, widget, TRUE, TRUE, 2);
	// button
	sizegroup = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
	hbox = (GtkBox*) gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
	gtk_box_pack_start (vbox, (GtkWidget*) hbox, FALSE, FALSE, 2);
	widget = gtk_button_new_with_label (_("All"));
	gtk_size_group_add_widget (sizegroup, widget);
	gtk_box_pack_start (hbox, widget, FALSE, FALSE, 1);
	g_signal_connect (widget, "clicked",
			G_CALLBACK (on_filter_button_all), item_view);
	widget = gtk_button_new_with_label (_("None"));
	gtk_size_group_add_widget (sizegroup, widget);
	gtk_box_pack_start (hbox, widget, FALSE, FALSE, 1);
	g_signal_connect (widget, "clicked",
			G_CALLBACK (on_filter_button_none), item_view);

	return (GtkWidget*) vbox;
}

static void	ug_selector_filter_init (struct UgSelectorFilter* filter, UgSelector* selector)
{
	GtkDialog*		dialog;
	GtkWidget*		widget;
	GtkBox*			vbox;
	GtkBox*			hbox;
	gchar*			title;

	title  = g_strconcat (UG_APP_GTK_NAME " - ", _("Mark by filter"), NULL);
	dialog = (GtkDialog*) gtk_dialog_new_with_buttons (title, selector->parent,
			0,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OK,     GTK_RESPONSE_OK,
			NULL);
	g_free (title);
	gtk_window_set_modal ((GtkWindow*) dialog, FALSE);
//	gtk_window_set_resizable ((GtkWindow*) dialog, FALSE);
	gtk_window_set_destroy_with_parent ((GtkWindow*) dialog, TRUE);
	gtk_window_set_transient_for ((GtkWindow*) dialog, selector->parent);
	gtk_window_resize ((GtkWindow*) dialog, 480, 330);
	g_signal_connect (dialog, "response",
			G_CALLBACK (on_filter_dialog_response), selector);
	filter->dialog = dialog;

	vbox = (GtkBox*) gtk_dialog_get_content_area (dialog);
	gtk_box_pack_start (vbox,
			gtk_label_new (_("Mark URLs by host AND filename extension.")),
			FALSE, FALSE, 3);
	gtk_box_pack_start (vbox,
			gtk_label_new (_("This will reset all marks of URLs.")),
			FALSE, FALSE, 3);

	hbox = (GtkBox*) gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
	gtk_box_pack_start (vbox, (GtkWidget*) hbox, TRUE, TRUE, 1);

	// filter view -----------------------
	// left side
	filter->host_view = ug_selector_view_new (_("Host"), TRUE);
	widget = ug_selector_filter_view_init (filter->host_view);
	gtk_box_pack_start (hbox, widget, TRUE, TRUE, 2);
	// right side (filename extension)
	filter->ext_view = ug_selector_view_new (_("File Ext."), TRUE);
	widget = ug_selector_filter_view_init (filter->ext_view);
	gtk_box_pack_start (hbox, widget, FALSE, TRUE, 2);

	gtk_widget_show_all (GTK_WIDGET (vbox));
}

static void	ug_selector_filter_show (struct UgSelectorFilter* filter, UgSelectorPage* page)
{
	GtkWindow* parent;

	gtk_tree_view_set_model (filter->host_view,
			GTK_TREE_MODEL (page->filter.host));
	gtk_tree_view_set_model (filter->ext_view,
			GTK_TREE_MODEL (page->filter.ext));

	// disable sensitive of parent window
	// enable sensitive in function on_filter_dialog_response()
	parent = gtk_window_get_transient_for ((GtkWindow*) filter->dialog);
	if (parent)
		gtk_widget_set_sensitive ((GtkWidget*) parent, FALSE);
	// create filter dialog
	if (gtk_window_get_modal (parent))
		gtk_dialog_run (filter->dialog);
	else
		gtk_widget_show ((GtkWidget*) filter->dialog);
}

//	signal handler ------------------------------
static void	on_selector_item_toggled (GtkCellRendererToggle* cell, gchar* path_str, UgSelector* selector)
{
	UgSelectorPage*	page;
	UgSelectorItem*	item;
	GtkTreeIter		iter;
	GtkTreePath*	path;
	GtkTreeModel*	model;

	page  = ug_selector_get_page (selector, -1);
	model = GTK_TREE_MODEL (page->store);
	path  = gtk_tree_path_new_from_string (path_str);
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_path_free (path);
	gtk_tree_model_get (model, &iter, 0, &item, -1);
	// UgSelectorItem.mark
	item->mark ^= 1;
	if (item->mark)
		page->n_marked++;
	else
		page->n_marked--;
	// count and notify
	ug_selector_count_marked (selector);
}

static void on_selector_mark_all (GtkWidget* button, UgSelector* selector)
{
	UgSelectorPage*	page;

	page = ug_selector_get_page (selector, -1);
	if (page == NULL)
		return;
	ug_selector_store_set_mark_all (page->store, TRUE);
	gtk_widget_queue_draw (GTK_WIDGET (page->view));
	// count and notify
	page->n_marked = gtk_tree_model_iter_n_children (GTK_TREE_MODEL (page->store), NULL);
	ug_selector_count_marked (selector);
}

static void on_selector_mark_none (GtkWidget* button, UgSelector* selector)
{
	UgSelectorPage*	page;

	page = ug_selector_get_page (selector, -1);
	if (page == NULL)
		return;
	ug_selector_store_set_mark_all (page->store, FALSE);
	gtk_widget_queue_draw (GTK_WIDGET (page->view));
	// count and notify
	page->n_marked = 0;
	ug_selector_count_marked (selector);
}

static void on_selector_mark_filter (GtkWidget* button, UgSelector* selector)
{
	UgSelectorPage*	page;

	page = ug_selector_get_page (selector, -1);
	if (page == NULL)
		return;
	ug_selector_page_make_filter (page);
	ug_selector_filter_show (&selector->filter, page);
}

static void on_filter_dialog_response (GtkDialog* dialog, gint response_id, UgSelector* selector)
{
	UgSelectorPage*	page;
	GtkWindow*		parent;

	if (response_id == GTK_RESPONSE_OK) {
		// update selection of page
		page = ug_selector_get_page (selector, -1);
		if (page)
			ug_selector_page_mark_by_filter_all (page);
	}
	// enable parent window
	parent = gtk_window_get_transient_for ((GtkWindow*) dialog);
	if (parent)
		gtk_widget_set_sensitive ((GtkWidget*) parent, TRUE);
	// hide filter dialog
	gtk_widget_hide ((GtkWidget*) dialog);
	// count and notify
	ug_selector_count_marked (selector);
}

static void on_filter_button_all (GtkWidget* widget, GtkTreeView* treeview)
{
	GtkListStore*	store;

	store = (GtkListStore*) gtk_tree_view_get_model (treeview);
	ug_selector_store_set_mark_all (store, TRUE);
	gtk_widget_queue_draw ((GtkWidget*) treeview);
}

static void on_filter_button_none (GtkWidget* widget, GtkTreeView* treeview)
{
	GtkListStore*	store;

	store = (GtkListStore*) gtk_tree_view_get_model (treeview);
	ug_selector_store_set_mark_all (store, FALSE);
	gtk_widget_queue_draw ((GtkWidget*) treeview);
}


// ----------------------------------------------------------------------------
// UgSelectorPage
//
void	ug_selector_page_init (UgSelectorPage* page)
{
	GtkScrolledWindow*	scrolled;

	page->store = gtk_list_store_new (1, G_TYPE_POINTER);
	page->view  = ug_selector_view_new (_("URL"), FALSE);
	gtk_tree_view_set_model (page->view, GTK_TREE_MODEL (page->store));
	// scrolled
	page->self = gtk_scrolled_window_new (NULL, NULL);
	scrolled = GTK_SCROLLED_WINDOW (page->self);
	gtk_scrolled_window_set_shadow_type (scrolled, GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy (scrolled,
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (scrolled), GTK_WIDGET (page->view));
	gtk_widget_show (page->self);

	// total marked count
	page->n_marked = 0;

	// UgSelectorFilterData initialize
	page->filter.hash = g_hash_table_new_full (g_str_hash, g_str_equal,
			NULL, (GDestroyNotify) g_list_free);
	page->filter.host = gtk_list_store_new (1, G_TYPE_POINTER);
	page->filter.ext  = gtk_list_store_new (1, G_TYPE_POINTER);
}

void	ug_selector_page_finalize (UgSelectorPage* page)
{
	ug_selector_store_clear (page->store);
	g_object_unref (page->store);

	// UgSelectorFilterData finalize
	g_hash_table_destroy (page->filter.hash);
	ug_selector_store_clear (page->filter.host);
	g_object_unref (page->filter.host);
	ug_selector_store_clear (page->filter.ext);
	g_object_unref (page->filter.ext);
}

void	ug_selector_page_add_uris (UgSelectorPage* page, GList* uris)
{
	GtkTreeIter			iter;
	UgSelectorItem*		item;

	for (;  uris;  uris = uris->next) {
		item = g_slice_alloc (sizeof (UgSelectorItem));
		item->mark = TRUE;
		item->uri  = uris->data;
		item->dataset = NULL;
		gtk_list_store_append (page->store, &iter);
		gtk_list_store_set (page->store, &iter, 0, item, -1);
		page->n_marked++;
	}
}

void	ug_selector_page_add_downloads (UgSelectorPage* page, GList* list)
{
	GtkTreeIter		iter;
	UgDataset*		dataset;
	UgSelectorItem*	item;

	for (;  list;  list = list->next) {
		dataset = list->data;
		ug_dataset_ref (dataset);
		item = g_slice_alloc (sizeof (UgSelectorItem));
		item->mark = TRUE;
		item->uri  = UG_DATASET_COMMON (dataset)->url;
		item->dataset = dataset;
		gtk_list_store_append (page->store, &iter);
		gtk_list_store_set (page->store, &iter, 0, item, -1);
		page->n_marked++;
	}
}

static void	ug_selector_page_add_filter (UgSelectorPage* page, GtkListStore* filter_store, gchar* key, UgSelectorItem* value)
{
	GtkTreeIter		iter;
	UgSelectorItem*	filter_item;
	GList*			filter_list;
	gchar*			orig_key;

	if (g_hash_table_lookup_extended (page->filter.hash, key,
			(gpointer*) &orig_key, (gpointer*) &filter_list) == FALSE)
	{
		filter_item = g_slice_alloc (sizeof (UgSelectorItem));
		filter_item->uri  = key;
		filter_item->mark = TRUE;
		filter_item->dataset = NULL;
		gtk_list_store_append (filter_store, &iter);
		gtk_list_store_set (filter_store, &iter, 0, filter_item, -1);
		filter_list = NULL;
	}
	else {
		g_hash_table_steal (page->filter.hash, key);
		g_free (key);
		key = orig_key;
	}
	filter_list = g_list_prepend (filter_list, value);
	g_hash_table_insert (page->filter.hash, key, filter_list);
}

void	ug_selector_page_make_filter (UgSelectorPage* page)
{
	UgUri*		upart;
	UgSelectorItem*	item;
	GtkTreeModel*	model;
	GtkTreeIter		iter;
	int				value;
	gchar*			key;

	if (g_hash_table_size (page->filter.hash))
		return;

	upart = g_slice_alloc (sizeof (UgUri));
	model = GTK_TREE_MODEL (page->store);
	value = gtk_tree_model_get_iter_first (model, &iter);
	while (value) {
		gtk_tree_model_get (model, &iter, 0, &item, -1);
		// create filter by host ----------------
		ug_uri_init (upart, item->uri);
		if (upart->authority)
			key = g_strndup (item->uri, upart->path);
		else
			key = g_strdup ("(none)");
		ug_selector_page_add_filter (page, page->filter.host, key, item);
		// create filter by filename extension --
		value = ug_uri_part_file_ext (upart, (const char**) &key);
		if (value)
			key = g_strdup_printf (".%.*s", value, key);
		else
			key = g_strdup (".(none)");
		ug_selector_page_add_filter (page, page->filter.ext, key, item);
		// next
		value = gtk_tree_model_iter_next (model, &iter);
	}
	g_slice_free1 (sizeof (UgUri), upart);
}

static void	ug_selector_page_mark_by_filter (UgSelectorPage* page, GtkListStore* filter_store)
{
	UgSelectorItem*	item;
	GList*			related;
	GList*			marked;
	GList*			link;

	marked = ug_selector_store_get_marked (filter_store, NULL);
	for (link = marked;  link;  link = link->next) {
		item = link->data;
		related = g_hash_table_lookup (page->filter.hash, item->uri);
		for (;  related;  related = related->next) {
			item = related->data;
			item->mark++;	// increase mark count
		}
	}
	g_list_free (marked);
}

void	ug_selector_page_mark_by_filter_all (UgSelectorPage* page)
{
	UgSelectorItem*	item;
	GtkTreeModel*	model;
	GtkTreeIter		iter;
	gboolean		valid;

	// clear all mark
	ug_selector_store_set_mark_all (page->store, FALSE);
	page->n_marked = 0;
	// If filter (host and filename extension) was selected, increase mark count.
	ug_selector_page_mark_by_filter (page, page->filter.host);
	ug_selector_page_mark_by_filter (page, page->filter.ext);
	// remark
	model = GTK_TREE_MODEL (page->store);
	valid = gtk_tree_model_get_iter_first (model, &iter);
	while (valid) {
		gtk_tree_model_get (model, &iter, 0, &item, -1);
		valid = gtk_tree_model_iter_next (model, &iter);
		// decrease mark count. If mark count is 2, it still marked.
		if (item->mark > 0) {
			item->mark--;
			if (item->mark)
				page->n_marked++;
		}
	}
}


// ----------------------------------------------------------------------------
// GtkListStore for UgSelectorItem

// (UgItem*) list->data. To free the returned value, call g_list_free()
static GList*	ug_selector_store_get_marked (GtkListStore* store, GList* list)
{
	UgSelectorItem*	item;
	GtkTreeModel*	model;
	GtkTreeIter		iter;
	gboolean		valid;

	model = GTK_TREE_MODEL (store);
	valid = gtk_tree_model_get_iter_first (model, &iter);
	while (valid) {
		gtk_tree_model_get (model, &iter, 0, &item, -1);
		valid = gtk_tree_model_iter_next (model, &iter);
		if (item->mark)
			list = g_list_prepend (list, item);
	}
	return list;
}

static void	ug_selector_store_set_mark_all (GtkListStore* store, gboolean mark)
{
	UgSelectorItem*	item;
	GtkTreeModel*	model;
	GtkTreeIter		iter;
	gboolean		valid;

	model = (GtkTreeModel*) store;
	valid = gtk_tree_model_get_iter_first (model, &iter);
	while (valid) {
		gtk_tree_model_get (model, &iter, 0, &item, -1);
		valid = gtk_tree_model_iter_next (model, &iter);
		item->mark = mark;
	}
}

static void	ug_selector_store_clear (GtkListStore* store)
{
	UgSelectorItem*	item;
	GtkTreeModel*	model;
	GtkTreeIter		iter;

	model = GTK_TREE_MODEL (store);
	while (gtk_tree_model_get_iter_first (model, &iter)) {
		gtk_tree_model_get (model, &iter, 0, &item, -1);
		gtk_list_store_remove (store, &iter);

		if (item->dataset)
			ug_dataset_unref (item->dataset);
		else
			g_free (item->uri);
		g_slice_free1 (sizeof (UgSelectorItem), item);
	}
}

// ----------------------------------------------------------------------------
// GtkTreeView for UgSelectorItem
//
static void col_set_toggle (GtkTreeViewColumn *column,
                            GtkCellRenderer   *cell,
                            GtkTreeModel      *model,
                            GtkTreeIter       *iter,
                            gpointer           data)
{
	UgSelectorItem*	item;

	gtk_tree_model_get (model, iter, 0, &item, -1);
	g_object_set (cell, "active", item->mark, NULL);
}

static void col_set_uri (GtkTreeViewColumn *column,
                         GtkCellRenderer   *cell,
                         GtkTreeModel      *model,
                         GtkTreeIter       *iter,
                         gpointer           data)
{
	UgSelectorItem*	item;

	gtk_tree_model_get (model, iter, 0, &item, -1);
	g_object_set (cell, "text", item->uri, NULL);
}

// toggled callback
static void on_cell_toggled (GtkCellRendererToggle* cell,
                             gchar*                 path_str,
                             GtkTreeView*           view)
{
	GtkTreeIter		iter;
	GtkTreePath*	path;
	GtkTreeModel*	model;
	UgSelectorItem*	item;

	path  = gtk_tree_path_new_from_string (path_str);
	model = gtk_tree_view_get_model (view);
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_path_free (path);
	gtk_tree_model_get (model, &iter, 0, &item, -1);
	item->mark ^= 1;
}

static GtkTreeView*	ug_selector_view_new (const gchar* title, gboolean active_toggled)
{
	GtkTreeView*		view;
	GtkCellRenderer*	renderer;
	GtkTreeViewColumn*	column;

	view = (GtkTreeView*) gtk_tree_view_new ();
//	gtk_tree_view_set_fixed_height_mode (view, TRUE);
	// UgSelectorItem.mark
	renderer = gtk_cell_renderer_toggle_new ();
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, "M");
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func (column, renderer,
			col_set_toggle, NULL, NULL);
	gtk_tree_view_column_set_resizable (column, FALSE);
	gtk_tree_view_column_set_alignment (column, 0.5);
	gtk_tree_view_column_set_min_width (column, 15);
//	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_append_column (view, column);
	if (active_toggled) {
		g_signal_connect (renderer, "toggled",
				G_CALLBACK (on_cell_toggled), view);
	}
	// UgSelectorItem.uri
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, title);
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func (column, renderer,
			col_set_uri, NULL, NULL);
	gtk_tree_view_column_set_resizable (column, TRUE);
//	gtk_tree_view_column_set_expand (column, TRUE);
//	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_append_column (view, column);

	gtk_widget_show (GTK_WIDGET (view));
	return view;
}

static GtkCellRenderer*	ug_selector_view_get_renderer_toggle (GtkTreeView* view)
{
	GtkCellRenderer*	renderer;
	GtkTreeViewColumn*	column;
	GList*				list;

	column = gtk_tree_view_get_column (view, 0);
	list = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (column));
	renderer = list->data;
	g_list_free (list);
	return renderer;
}

