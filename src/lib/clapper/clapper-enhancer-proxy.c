/* Clapper Playback Library
 * Copyright (C) 2025 Rafał Dzięgiel <rafostar.github@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

/**
 * ClapperEnhancerProxy:
 *
 * An intermediary between player and enhancer plugin.
 *
 * Applications can use this to inspect enhancer information,
 * its properties, configure and enable it.
 *
 * Clapper player manages all enhancers internally, including
 * creating when needed and destroying them later. Instead,
 * it provides access to so called enhancer proxy objects which
 * allow to browse available enhancers and store their config
 * separately for each player instance.
 *
 * Use [property@Clapper.Player:enhancer-proxies] property to
 * access a [iface@Gio.ListModel] of available enhancer proxies.
 *
 * Since: 0.10
 */

#include "clapper-enhancer-proxy-private.h"
#include "clapper-player-private.h"
#include "clapper-extractable.h"

#include "clapper-functionalities-availability.h"

#if CLAPPER_WITH_ENHANCERS_LOADER
#include <libpeas.h>
#endif

#define GST_CAT_DEFAULT clapper_enhancer_proxy_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

struct _ClapperEnhancerProxy
{
  GstObject parent;

  GObject *peas_info;

  guint n_ifaces;
  GType *ifaces;

  guint n_pspecs;
  GParamSpec **pspecs;

  GstStructure *config;
};

#define parent_class clapper_enhancer_proxy_parent_class
G_DEFINE_TYPE (ClapperEnhancerProxy, clapper_enhancer_proxy, GST_TYPE_OBJECT);

static gboolean
_update_config_cb (GQuark field_id, const GValue *value, GstStructure *config)
{
  gst_structure_set_value (config, g_quark_to_string (field_id), value);

  return TRUE;
}

static void
_update_config_from_structure (ClapperEnhancerProxy *self, const GstStructure *src)
{
  GST_OBJECT_LOCK (self);

  if (!self->config)
    self->config = gst_structure_copy (src);
  else
    gst_structure_foreach (src, (GstStructureForeachFunc) _update_config_cb, self->config);

  GST_OBJECT_UNLOCK (self);
}

static void
_post_config (ClapperEnhancerProxy *self, GstStructure *structure)
{
  ClapperPlayer *player;

  if ((player = CLAPPER_PLAYER_CAST (gst_object_get_parent (GST_OBJECT_CAST (self))))) {
    //clapper_enhancers_manager_trigger_config_changed (player->enhancers_manager, self, structure);
    gst_object_unref (player);
  }
}

/*
 * clapper_enhancer_proxy_new_take:
 * @peas_info: (transfer full): a #PeasPluginInfo cast to GObject
 *
 * Returns: (transfer full): a new #ClapperEnhancerProxy instance.
 */
ClapperEnhancerProxy *
clapper_enhancer_proxy_new_take (GObject *peas_info)
{
  ClapperEnhancerProxy *proxy;

  proxy = g_object_new (CLAPPER_TYPE_ENHANCER_PROXY, NULL);
  proxy->peas_info = peas_info;

  gst_object_ref_sink (proxy);

  return proxy;
}

/*
 * clapper_enhancer_proxy_new_from_proxy:
 * @proxy: a #ClapperEnhancerProxy
 *
 * Create new (unconfigured) enhancer proxy. Uses another proxy
 * as source to avoid reading cache again.
 *
 * Returns: (transfer full): a new #ClapperEnhancerProxy instance.
 */
ClapperEnhancerProxy *
clapper_enhancer_proxy_new_from_proxy (ClapperEnhancerProxy *proxy)
{
  ClapperEnhancerProxy *copy;
  guint i;

  copy = g_object_new (CLAPPER_TYPE_ENHANCER_PROXY, NULL);
  copy->peas_info = g_object_ref (proxy->peas_info);

  /* Copy extra data from source proxy */

  copy->n_ifaces = proxy->n_ifaces;
  copy->ifaces = g_new (GType, copy->n_ifaces);

  copy->n_pspecs = proxy->n_pspecs;
  copy->pspecs = g_new (GParamSpec *, copy->n_pspecs);

  for (i = 0; i < proxy->n_pspecs; ++i)
    copy->pspecs[i] = proxy->pspecs[i];

  gst_object_ref_sink (copy);

  return copy;
}

gboolean
clapper_enhancer_proxy_fill_from_cache (ClapperEnhancerProxy *self)
{
  GST_FIXME_OBJECT (self, "Implement enhancer proxy caching");

  return FALSE;
}

void
clapper_enhancer_proxy_fill_from_instance (ClapperEnhancerProxy *self, GType iface_type, GObject *enhancer)
{
  self->ifaces = g_type_interfaces (G_OBJECT_TYPE (enhancer), &self->n_ifaces);
  self->pspecs = g_object_class_list_properties (G_OBJECT_GET_CLASS (enhancer), &self->n_pspecs);

  GST_DEBUG_OBJECT (self, "Filled proxy \"%s\", n_ifaces: %u, n_pspecs: %u",
      clapper_enhancer_proxy_get_friendly_name (self), self->n_ifaces, self->n_pspecs);
}

gboolean
clapper_enhancer_proxy_target_has_interface (ClapperEnhancerProxy *self, GType iface_type)
{
  guint i;

  for (i = 0; i < self->n_ifaces; ++i) {
    if (self->ifaces[i] == iface_type)
      return TRUE;
  }

  return FALSE;
}

GObject *
clapper_enhancer_proxy_get_peas_info (ClapperEnhancerProxy *self)
{
  return self->peas_info;
}

/**
 * clapper_enhancer_proxy_get_friendly_name:
 * @proxy: a #ClapperEnhancerProxy
 *
 * Get a name from enhancer plugin info file.
 * Can be used for showing in UI and such.
 *
 * Returns: (nullable): name of the proxied enhancer.
 *
 * Since: 0.10
 */
const gchar *
clapper_enhancer_proxy_get_friendly_name (ClapperEnhancerProxy *self)
{
  g_return_val_if_fail (CLAPPER_IS_ENHANCER_PROXY (self), NULL);

#if CLAPPER_WITH_ENHANCERS_LOADER
  return peas_plugin_info_get_name ((const PeasPluginInfo *) self->peas_info);
#else
  return NULL;
#endif
}

/**
 * clapper_enhancer_proxy_get_module_name:
 * @proxy: a #ClapperEnhancerProxy
 *
 * Get name of the module from enhancer plugin info file.
 * This value is used to uniquely identify a particular plugin.
 *
 * Returns: (nullable): name of the proxied enhancer.
 *
 * Since: 0.10
 */
const gchar *
clapper_enhancer_proxy_get_module_name (ClapperEnhancerProxy *self)
{
  g_return_val_if_fail (CLAPPER_IS_ENHANCER_PROXY (self), NULL);

#if CLAPPER_WITH_ENHANCERS_LOADER
  return peas_plugin_info_get_module_name ((const PeasPluginInfo *) self->peas_info);
#else
  return NULL;
#endif
}

/**
 * clapper_enhancer_proxy_get_description:
 * @proxy: a #ClapperEnhancerProxy
 *
 * Get description from enhancer plugin info file.
 *
 * Returns: (nullable): description of the proxied enhancer.
 *
 * Since: 0.10
 */
const gchar *
clapper_enhancer_proxy_get_description (ClapperEnhancerProxy *self)
{
  g_return_val_if_fail (CLAPPER_IS_ENHANCER_PROXY (self), NULL);

#if CLAPPER_WITH_ENHANCERS_LOADER
  return peas_plugin_info_get_description ((const PeasPluginInfo *) self->peas_info);
#else
  return NULL;
#endif
}

/**
 * clapper_enhancer_proxy_get_version:
 * @proxy: a #ClapperEnhancerProxy
 *
 * Get version string from enhancer plugin info file.
 *
 * Returns: (nullable): version string of the proxied enhancer.
 *
 * Since: 0.10
 */
const gchar *
clapper_enhancer_proxy_get_version (ClapperEnhancerProxy *self)
{
  g_return_val_if_fail (CLAPPER_IS_ENHANCER_PROXY (self), NULL);

#if CLAPPER_WITH_ENHANCERS_LOADER
  return peas_plugin_info_get_version ((const PeasPluginInfo *) self->peas_info);
#else
  return NULL;
#endif
}

/**
 * clapper_enhancer_proxy_get_extra_data:
 * @proxy: a #ClapperEnhancerProxy
 * @key: name of the data to lookup
 *
 * Get extra data from enhancer plugin info file specified by @key.
 *
 * External data in the plugin info file is prefixed with `X-`.
 * For example `X-Schemes=https`.
 *
 * Returns: (nullable): extra data value of the proxied enhancer.
 *
 * Since: 0.10
 */
const gchar *
clapper_enhancer_proxy_get_extra_data (ClapperEnhancerProxy *self, const gchar *key)
{
  g_return_val_if_fail (CLAPPER_IS_ENHANCER_PROXY (self), NULL);
  g_return_val_if_fail (key != NULL, NULL);

#if CLAPPER_WITH_ENHANCERS_LOADER
  return peas_plugin_info_get_external_data ((const PeasPluginInfo *) self->peas_info, key);
#else
  return NULL;
#endif
}

/*
 * clapper_enhancer_proxy_get_current_config:
 * @proxy: a #ClapperEnhancerProxy
 *
 * Returns: (transfer full) (nullable): a copy of current config structure
 *   or %NULL when no modifications were done.
 *
 * Since: 0.10
 */
GstStructure *
clapper_enhancer_proxy_get_current_config (ClapperEnhancerProxy *self)
{
  GstStructure *config = NULL;

  GST_OBJECT_LOCK (self);

  if (self->config)
    config = gst_structure_copy (self->config);

  GST_OBJECT_UNLOCK (self);

  return config;
}

/**
 * clapper_enhancer_proxy_get_target_interfaces:
 * @proxy: a #ClapperEnhancerProxy
 * @n_interfaces: (out): return location for the length of the returned array
 *
 * Get an array of interfaces that target enhancer implements.
 *
 * Returns: (transfer none) (nullable) (array length=n_interfaces): an array of #GType interfaces.
 *
 * Since: 0.10
 */
const GType *
clapper_enhancer_proxy_get_target_interfaces (ClapperEnhancerProxy *self, guint *n_interfaces)
{
  g_return_val_if_fail (CLAPPER_IS_ENHANCER_PROXY (self), NULL);

  if (n_interfaces)
    *n_interfaces = self->n_ifaces;

  return (const GType *) self->ifaces;
}

/**
 * clapper_enhancer_proxy_get_target_properties:
 * @proxy: a #ClapperEnhancerProxy
 * @n_properties: (out): return location for the length of the returned array
 *
 * Get an array of properties that can be configured within target enhancer.
 *
 * Implementations can use this in order to find out what properties, type of
 * their values (including valid ranges) are allowed to set for a given enhancer.
 *
 * Returns: (transfer none) (nullable) (array length=n_properties): an array of #GParamSpec objects.
 *
 * Since: 0.10
 */
const GParamSpec **
clapper_enhancer_proxy_get_target_properties (ClapperEnhancerProxy *self, guint *n_properties)
{
  g_return_val_if_fail (CLAPPER_IS_ENHANCER_PROXY (self), NULL);

  if (n_properties)
    *n_properties = self->n_pspecs;

  return (const GParamSpec **) self->pspecs;
}

/**
 * clapper_enhancer_proxy_configure:
 * @proxy: a #ClapperEnhancerProxy
 * @first_property_name: name of the first property to configure
 * @...: %NULL-terminated list of arguments
 *
 * Configure one or more properties on the target enhancer.
 *
 * Arguments should be %NULL terminated list of property name,
 * followed by its `GType` and finally a value to set.
 *
 * Since: 0.10
 */
void
clapper_enhancer_proxy_configure (ClapperEnhancerProxy *self, const gchar *first_property_name, ...)
{
  GstStructure *structure;
  va_list args;

  g_return_if_fail (CLAPPER_IS_ENHANCER_PROXY (self));
  g_return_if_fail (first_property_name != NULL);

  va_start (args, first_property_name);
  structure = gst_structure_new_valist ("config", first_property_name, args);
  va_end (args);

  _update_config_from_structure (self, structure);

  if (FALSE) // FIXME: If self->ifaces has one of the managed
    _post_config (self, structure);
  else
    gst_structure_free (structure);
}

/**
 * clapper_enhancer_proxy_configure_value: (rename-to clapper_enhancer_proxy_configure)
 * @proxy: a #ClapperEnhancerProxy
 * @property_name: name of property to configure
 * @value: a #GValue to set
 *
 * Configure property on the target enhancer using #GValue.
 *
 * Since: 0.10
 */
void
clapper_enhancer_proxy_configure_value (ClapperEnhancerProxy *self, const gchar *property_name, const GValue *value)
{
  GstStructure *structure;

  g_return_if_fail (CLAPPER_IS_ENHANCER_PROXY (self));
  g_return_if_fail (property_name != NULL);
  g_return_if_fail (G_IS_VALUE (value));

  structure = gst_structure_new_empty ("config");
  gst_structure_set_value (structure, property_name, value);

  _update_config_from_structure (self, structure);

  if (FALSE) // FIXME: If self->ifaces has one of the managed
    _post_config (self, structure);
  else
    gst_structure_free (structure);
}

static void
clapper_enhancer_proxy_init (ClapperEnhancerProxy *self)
{
}

static void
clapper_enhancer_proxy_finalize (GObject *object)
{
  ClapperEnhancerProxy *self = CLAPPER_ENHANCER_PROXY_CAST (object);

  GST_TRACE_OBJECT (self, "Finalize");

  g_object_unref (self->peas_info);
  g_free (self->pspecs);
  gst_clear_structure (&self->config);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clapper_enhancer_proxy_class_init (ClapperEnhancerProxyClass *klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;

  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "clapperenhancerproxy", 0,
      "Clapper Enhancer Proxy");

  gobject_class->finalize = clapper_enhancer_proxy_finalize;
}
