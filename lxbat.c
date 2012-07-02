/*
 *   lxbat plugin for lxpanel 
 *   Copyright 2012 Pau Oliva Fora <pof@eslack.org>
 * 
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation; either version 2
 *   of the License, or (at your option) any later version.
 *
 *   Inspired on:
 *     http://danamlund.dk/sensors_lxpanel_plugin
 *     http://wiki.lxde.org/en/How_to_write_plugins_for_LXPanel
 *
 */

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib/gi18n.h>

#include <lxpanel/plugin.h>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

/* update period in seconds */
#define PERIOD 10

/* from lxpanel-0.5.8/src/plugin.h */
gboolean plugin_button_press_event (GtkWidget * widget,
				    GdkEventButton * event, Plugin * plugin);

/* from lxpanel-0.5.8/src/configurator.h */
GtkWidget *create_generic_config_dlg (const char *title, GtkWidget * parent,
				      GSourceFunc apply_func, Plugin * plugin,
				      const char *name, ...);

/* from lxpanel-0.5.8/src/misc.h */
guint32 gcolor2rgb24 (GdkColor * color);


#define MAXLEN 1024

typedef struct
{
	unsigned int timer;
	GtkWidget *label;
	char *col_normal_str, *col_critical_str;
	GdkColor col_normal, col_critical;
} LxbatPlugin;

char battery_info[MAXLEN];
char battery_state[MAXLEN];

gboolean battery_enabled = FALSE;

void init_battery_data()
{

	FILE *fd;
	char input[MAXLEN];
	const char *battery_names[] = { "BAT0", "BAT1", "C23A", "C23B" };
	unsigned int i;

	for (i = 0; i < sizeof (battery_names) / sizeof (battery_names[0]) && !battery_enabled; i++)
	{

		sprintf (battery_info, "/proc/acpi/battery/%s/info", battery_names[i]);
		sprintf (battery_state, "/proc/acpi/battery/%s/state", battery_names[i]);

		fd = fopen (battery_info, "r");
		if (fd == NULL)
			continue;

		while ((fgets (input, sizeof (input), fd)) != NULL)
		{
			if ((strncmp ("last full capacity:", input, 19)) == 0)
			{
				battery_enabled = TRUE;
				printf ("Found battery info at /proc/acpi/battery/%s/\n",
				battery_names[i]);
			}
		}

		fclose (fd);
	}
}


int get_battery()
{
	char input[MAXLEN], temp[MAXLEN];
	FILE *fd;
	size_t len;
	int remaining_capacity = 1, last_full_capacity = 67000;
	int ret;

	fd = fopen (battery_state, "r");
	if (fd == NULL)
	{
		fprintf (stderr, "Could not open battery_state: %s\n", battery_state);
		battery_enabled = FALSE;
		return (-1);
	}

	while ((fgets (input, sizeof (input), fd)) != NULL)
	{

		if ((strncmp ("remaining capacity:", input, 19)) == 0)
		{
			strncpy (temp, input + 19, MAXLEN - 1);
			len = strlen (temp);
			temp[len + 1] = '\0';
			remaining_capacity = atoi (temp);
		}
	}

	fclose (fd);

	fd = fopen (battery_info, "r");
	if (fd == NULL)
	{
		fprintf (stderr, "Could not open battery_info: %s\n", battery_info);
		return (-1);
	}

	while ((fgets (input, sizeof (input), fd)) != NULL)
	{

		if ((strncmp ("last full capacity:", input, 19)) == 0)
		{
			strncpy (temp, input + 19, MAXLEN - 1);
			len = strlen (temp);
			temp[len + 1] = '\0';
			last_full_capacity = atoi (temp);
		}
	}

	fclose (fd);

	ret = 100 * remaining_capacity / last_full_capacity;
	if (ret > 100)
		return 100;
	else
		return ret;
}



static int update_display (LxbatPlugin * pLxbat)
{
	int battery;
	char out[100] = { 0 };
	char label[100] = { 0 };
	GdkColor color;

	if (battery_enabled)
	{

		battery = get_battery ();
		sprintf (out, "%02d%%", battery);

		color = pLxbat->col_normal;
		if (battery <= 10)
			color = pLxbat->col_critical;

		if (pLxbat != NULL)
		{
			strcpy (label, out);
			sprintf (label, "<span color=\"#%06x\"><b>%s</b></span>",
			gcolor2rgb24 (&color), out);
		}

		gtk_label_set_markup (GTK_LABEL (pLxbat->label), label);

	}
	return 1;
}


static int lxbat_constructor (Plugin * p, char **fp)
{

	// allocate our private structure instance
	LxbatPlugin *pLxbat = g_new0 (LxbatPlugin, 1);

	// put it where it belongs
	p->priv = pLxbat;

	line s;
	s.len = 256;
	if (fp != NULL)
	{
		while (lxpanel_get_line (fp, &s) != LINE_BLOCK_END)
		{
			if (s.type == LINE_NONE)
			{
				ERR ("lxbat: illegal token %s\n", s.str);
				return 0;
			}
			if (s.type == LINE_VAR)
			{
				if (g_ascii_strcasecmp (s.t[0], "ColorNormal") == 0)
					pLxbat->col_normal_str = g_strdup (s.t[1]);
				else if (g_ascii_strcasecmp (s.t[0], "ColorCritical") == 0)
					pLxbat->col_critical_str = g_strdup (s.t[1]);
			}
		}
	}

	if (pLxbat->col_normal_str == NULL)
		pLxbat->col_normal_str = g_strdup ("white");
	if (pLxbat->col_critical_str == NULL)
		pLxbat->col_critical_str = g_strdup ("red");

	gdk_color_parse (pLxbat->col_normal_str, &pLxbat->col_normal);
	gdk_color_parse (pLxbat->col_critical_str, &pLxbat->col_critical);

	GtkWidget *label = gtk_label_new ("..");
	pLxbat->label = label;

	// need to create a widget to show
	p->pwid = gtk_event_box_new ();

	// set border width
	gtk_container_set_border_width (GTK_CONTAINER (p->pwid), 1);

	// add the label to the container
	gtk_container_add (GTK_CONTAINER (p->pwid), GTK_WIDGET (label));

	// set the size we want
	gtk_widget_set_size_request (p->pwid, 40, 25);

	// our widget doesn't have a window...
	gtk_widget_set_has_window (p->pwid, FALSE);

	// show our widget
	gtk_widget_show_all (p->pwid);

	init_battery_data ();
	update_display (pLxbat);
	// update period in milliseconds
	pLxbat->timer = g_timeout_add (1000 * PERIOD, (GSourceFunc) update_display, (gpointer) pLxbat);

	// success!!!
	return 1;
}

static void lxbat_destructor (Plugin * p)
{
	// find our private structure instance
	LxbatPlugin *pLxbat = (LxbatPlugin *) p->priv;

	g_source_remove (pLxbat->timer);
	g_free (pLxbat->col_normal_str);
	g_free (pLxbat->col_critical_str);

	// free it  -- no need to worry about freeing 'pwid', the panel does it
	g_free (pLxbat);
}

static void lxbat_apply_configure (Plugin * p)
{
	LxbatPlugin *pLxbat = (LxbatPlugin *) p->priv;

	gdk_color_parse (pLxbat->col_normal_str, &pLxbat->col_normal);
	gdk_color_parse (pLxbat->col_critical_str, &pLxbat->col_critical);
}

static void lxbat_configure (Plugin * p, GtkWindow * parent)
{
	GtkWidget *dialog;
	LxbatPlugin *pLxbat = (LxbatPlugin *) p->priv;

	dialog = create_generic_config_dlg (_(p->class->name), GTK_WIDGET (parent), (GSourceFunc) lxbat_apply_configure, (gpointer) p, _("Normal text color"), &pLxbat->col_normal_str, CONF_TYPE_STR, _("Low battery text color"), &pLxbat->col_critical_str, CONF_TYPE_STR, NULL);

	GtkWidget *hbox = gtk_hbox_new (FALSE, 2);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), hbox);

	gtk_widget_show_all (dialog);
	gtk_window_present (GTK_WINDOW (dialog));

}

static void lxbat_save_configuration (Plugin * p, FILE * fp)
{
	LxbatPlugin *pLxbat = (LxbatPlugin *) p->priv;
	lxpanel_put_str (fp, "ColorNormal", pLxbat->col_normal_str);
	lxpanel_put_str (fp, "ColorCritical", pLxbat->col_critical_str);
}


/* Plugin descriptor. */
PluginClass lxbat_plugin_class = {

	// this is a #define taking care of the size/version variables
	PLUGINCLASS_VERSIONING,

	// type of this plugin
	type:"lxbat",
	name:N_("Lx Battery Plugin"),
	version:"0.1",
	description:N_("Displays the battery charge percentage."),

	// we can have only one instance running at the same time
	one_per_system:TRUE,

	// can't expand this plugin
	expand_available:FALSE,

	// assigning our functions to provided pointers.
	constructor:lxbat_constructor,
	destructor:lxbat_destructor,
	config:lxbat_configure,
	save:lxbat_save_configuration
};
