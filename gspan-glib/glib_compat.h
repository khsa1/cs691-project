#include <glib.h>

#if (GLIB_MINOR_VERSION < 32)

static inline gboolean g_hash_table_add(GHashTable *hash_table, gpointer key)
{
	return g_hash_table_replace(hash_table, key, key);
}

static inline gboolean g_hash_table_contains(GHashTable *hash_table, gpointer key)
{
	return g_hash_table_lookup_extended(hash_table, key, key, NULL, NULL);
}


#endif
