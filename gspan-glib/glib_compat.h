#include <glib.h>

#ifndef __GLIB_COMPAT_H__
#define __GLIB_COMPAT_H__

#if (GLIB_MINOR_VERSION < 32)

static void g_free_tramp(void *d, void *u)
{
        void (*func)(void *) = u;

        func(d);
        return;
}

static inline void g_queue_free_full(GQueue *q, void (*f)(void *))
{
        g_queue_foreach(q, g_free_tramp, f);
        g_queue_free(q);
}

#if (GLIB_MINOR_VERSION < 28)
static inline void g_list_free_full(GList *l, void (*f)(void *))
{
	g_list_foreach(l, g_free_tramp, f);
	g_list_free(l);
}
#endif

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
