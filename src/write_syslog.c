/**
 * collectd - src/write_syslog.c
 * Copyright (C) 2014       Anand Karthik Tumuluru
 * Copyright (C) 2012       Pierre-Yves Ritschard
 * Copyright (C) 2011       Scott Sanders
 * Copyright (C) 2009       Paul Sadauskas
 * Copyright (C) 2009       Doug MacEachern
 * Copyright (C) 2007-2013  Florian octo Forster
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; only version 2 of the License is applicable.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * Authors:
 *   Anand Karthik Tumuluru <anand.karthik at flipkart.com>
 *   Florian octo Forster <octo at collectd.org>
 *   Doug MacEachern <dougm at hyperic.com>
 *   Paul Sadauskas <psadauskas at gmail.com>
 *   Scott Sanders <scott at jssjr.com>
 *   Pierre-Yves Ritschard <pyr at spootnik.org>
 *
 * Based on the write_http plugin.
 **/

 /* write_syslog plugin configuation example
  *
  * <Plugin write_syslog>
  *   <Node cosmos>
  *     Prefix "collectd"
  *   </Node>
  * </Plugin>
  */

#include <syslog.h>
#include "collectd.h"
#include "common.h"
#include "plugin.h"
#include "configfile.h"

#include "utils_cache.h"
#include "utils_complain.h"
#include "utils_parse_option.h"
#include "utils_format_syslog.h"

/* Folks without pthread will need to disable this plugin. */
#include <pthread.h>

#include <sys/socket.h>
#include <netdb.h>

#ifndef WS_DEFAULT_NODE
# define WS_DEFAULT_NODE "localhost"
#endif

#ifndef WS_DEFAULT_SERVICE
# define WS_DEFAULT_SERVICE "2003"
#endif

#ifndef WS_DEFAULT_PROTOCOL
# define WS_DEFAULT_PROTOCOL "tcp"
#endif

#ifndef WS_DEFAULT_LOG_SEND_ERRORS
# define WS_DEFAULT_LOG_SEND_ERRORS 1
#endif

#ifndef WS_DEFAULT_ESCAPE
# define WS_DEFAULT_ESCAPE '_'
#endif

/* Ethernet - (IPv6 + TCP) = 1500 - (40 + 32) = 1428 */
#ifndef WS_SEND_BUF_SIZE
# define WS_SEND_BUF_SIZE 1428
#endif

/*
 * Private variables
 */
struct ws_callback
{
    char    *name;
    char    *node;
    char    *prefix;
    char    *tags;
    char     escape_char;
    unsigned int format_flags;
};


static void ws_callback_free (void *data)
{
    struct ws_callback *cb;
    if (data == NULL)
        return;
    cb = data;
    sfree(cb->name);
    sfree(cb->prefix);
    sfree(cb->tags);
    sfree(cb);
}

static int ws_send_message (char const *message, struct wt_callback *cb)
{
    int status;
    size_t message_len;

    message_len = strlen (message);

    syslog (6, "%s", msg);
    return (0);
}

static int ws_write_messages (const data_set_t *ds, const value_list_t *vl,
        struct ws_callback *cb)
{
    int status;

    if (0 != strcmp (ds->type, vl->type))
    {
        ERROR ("write_syslog plugin: DS type does not match "
                "value list type");
        return -1;
    }

    memset (buffer, 0, sizeof (buffer));
    status = format_syslog (buffer, sizeof (buffer), ds, vl,
            cb->prefix, cb->tags, cb->escape_char, cb->format_flags);
    if (status != 0) /* error message has been printed already. */
        return (status);

    /* Send the message to syslog */
    status = ws_send_message (buffer, cb);
    if (status != 0) /* error message has been printed already. */
        return (status);

    return (0);
} /* int ws_write_messages */

static int ws_write (const data_set_t *ds, const value_list_t *vl,
        user_data_t *user_data)
{
    struct ws_callback *cb;
    int status;

    if (user_data == NULL)
        return (EINVAL);

    cb = user_data->data;

    status = ws_write_messages (ds, vl, cb);

    return (status);
}

static int config_set_char (char *dest,
        oconfig_item_t *ci)
{
    char buffer[4];
    int status;

    memset (buffer, 0, sizeof (buffer));

    status = cf_util_get_string_buffer (ci, buffer, sizeof (buffer));
    if (status != 0)
        return (status);

    if (buffer[0] == 0)
    {
        ERROR ("write_syslog plugin: Cannot use an empty string for the "
                "\"EscapeCharacter\" option.");
        return (-1);
    }

    if (buffer[1] != 0)
    {
        WARNING ("write_syslog plugin: Only the first character of the "
                "\"EscapeCharacter\" option ('%c') will be used.",
                (int) buffer[0]);
    }

    *dest = buffer[0];

    return (0);
}

static int ws_config_node (oconfig_item_t *ci)
{
    struct ws_callback *cb;
    user_data_t user_data;
    char callback_name[DATA_MAX_NAME_LEN];
    int i;
    int status = 0;

    cb = malloc (sizeof (*cb));
    if (cb == NULL)
    {
        ERROR ("write_syslog plugin: malloc failed.");
        return (-1);
    }
    memset (cb, 0, sizeof (*cb));
    cb->name = NULL;
    cb->node = NULL;
    cb->prefix = NULL;
    cb->tags = NULL;
    cb->escape_char = WS_DEFAULT_ESCAPE;
    cb->format_flags = SYSLOG_STORE_RATES;

    for (i = 0; i < ci->children_num; i++)
    {
        oconfig_item_t *child = ci->children + i;

        if (strcasecmp ("Prefix", child->key) == 0)
            cf_util_get_string (child, &cb->prefix);
        else if (strcasecmp ("Tags", child->key) == 0)
            cf_util_get_string (child, &cb->tags);
        else if (strcasecmp ("StoreRates", child->key) == 0)
            cf_util_get_flag (child, &cb->format_flags,
                    SYSLOG_STORE_RATES);
        else if (strcasecmp ("EscapeCharacter", child->key) == 0)
            config_set_char (&cb->escape_char, child);
        else
        {
            ERROR ("write_syslog plugin: Invalid configuration "
                        "option: %s.", child->key);
            status = -1;
        }

	DEBUG("write_syslog parsed Tags: %s", cb->tags);

        if (status != 0)
            break;
    }

    if (status != 0)
    {
        ws_callback_free (cb);
        return (status);
    }

    /* FIXME: Legacy configuration syntax. */
    if (cb->name == NULL)
        ssnprintf (callback_name, sizeof (callback_name), "write_syslog");
    else
        ssnprintf (callback_name, sizeof (callback_name), "write_syslog/%s",
                cb->name);

    memset (&user_data, 0, sizeof (user_data));
    user_data.data = cb;
    user_data.free_func = ws_callback_free;
    plugin_register_write (callback_name, ws_write, &user_data);
    return (0);
}

static int ws_config (oconfig_item_t *ci)
{
    int i;

    for (i = 0; i < ci->children_num; i++)
    {
        oconfig_item_t *child = ci->children + i;

        if (strcasecmp ("Node", child->key) == 0)
            ws_config_node (child);
        else
        {
            ERROR ("write_syslog plugin: Invalid configuration "
                    "option: %s.", child->key);
        }
    }

    return (0);
}

void module_register (void)
{
    openlog ("collectd", LOG_CONS | LOG_PID, LOG_DAEMON);
    plugin_register_complex_config ("write_syslog", ws_config);
}

/* vim: set sw=4 ts=4 sts=4 tw=78 et : */
