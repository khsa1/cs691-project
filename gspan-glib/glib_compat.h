#include <glib.h>

#ifndef __GLIB_COMPAT_H__
#define __GLIB_COMPAT_H__

#if (GLIB_MINOR_VERSION < 32)

static inline gboolean g_hash_table_add(GHashTable *hash_table, gpointer key)
{
	g_hash_table_replace(hash_table, key, key);
	return TRUE;
}

static inline gboolean g_hash_table_contains(GHashTable *hash_table, gpointer key)
{
	return g_hash_table_lookup_extended(hash_table, key, NULL, NULL);
}

#endif
#endif /* __GLIB_COMPAT_H__ */
