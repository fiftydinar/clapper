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

#pragma once

#if !defined(__CLAPPER_INSIDE__) && !defined(CLAPPER_COMPILATION)
#error "Only <clapper/clapper.h> can be included directly."
#endif

#include <glib.h>
#include <glib-object.h>
#include <gst/gst.h>

#include <clapper/clapper-visibility.h>

G_BEGIN_DECLS

#define CLAPPER_TYPE_ENHANCER_PROXY (clapper_enhancer_proxy_get_type())
#define CLAPPER_ENHANCER_PROXY_CAST(obj) ((ClapperEnhancerProxy *)(obj))

CLAPPER_API
G_DECLARE_FINAL_TYPE (ClapperEnhancerProxy, clapper_enhancer_proxy, CLAPPER, ENHANCER_PROXY, GstObject)

CLAPPER_API
const gchar * clapper_enhancer_proxy_get_friendly_name (ClapperEnhancerProxy *proxy);

CLAPPER_API
const gchar * clapper_enhancer_proxy_get_module_name (ClapperEnhancerProxy *proxy);

CLAPPER_API
const gchar * clapper_enhancer_proxy_get_description (ClapperEnhancerProxy *proxy);

CLAPPER_API
const gchar * clapper_enhancer_proxy_get_version (ClapperEnhancerProxy *proxy);

CLAPPER_API
const gchar * clapper_enhancer_proxy_get_extra_data (ClapperEnhancerProxy *proxy, const gchar *key);

CLAPPER_API
const GType * clapper_enhancer_proxy_get_target_interfaces (ClapperEnhancerProxy *proxy, guint *n_interfaces);

CLAPPER_API
const GParamSpec ** clapper_enhancer_proxy_get_target_properties (ClapperEnhancerProxy *proxy, guint *n_properties);

CLAPPER_API
void clapper_enhancer_proxy_configure (ClapperEnhancerProxy *proxy, const gchar *first_property_name, ...) G_GNUC_NULL_TERMINATED;

CLAPPER_API
void clapper_enhancer_proxy_configure_value (ClapperEnhancerProxy *proxy, const gchar *property_name, const GValue *value);

G_END_DECLS
