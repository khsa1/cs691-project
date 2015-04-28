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


static inline gboolean g_hash_table_add(GHashTable *hash_table, gpointer key)
{
	g_hash_table_replace(hash_table, key, key);
	return TRUE;
}

static inline gboolean g_hash_table_contains(GHashTable *hash_table, gpointer key)
{
	return g_hash_table_lookup_extended(hash_table, key, NULL, NULL);
}

/**
 * g_list_copy_deep:
 * @list: a #GList, this must point to the top of the list
 * @func: a copy function used to copy every element in the list
 * @user_data: user data passed to the copy function @func, or %NULL
 *
 * Makes a full (deep) copy of a #GList.
 *
 * In contrast with g_list_copy(), this function uses @func to make
 * a copy of each list element, in addition to copying the list
 * container itself.
 *
 * @func, as a #GCopyFunc, takes two arguments, the data to be copied
 * and a @user_data pointer. It's safe to pass %NULL as user_data,
 * if the copy function takes only one argument.
 *
 * For instance, if @list holds a list of GObjects, you can do:
 * |[<!-- language="C" -->   
 * another_list = g_list_copy_deep (list, (GCopyFunc) g_object_ref, NULL);
 * ]|
 *
 * And, to entirely free the new list, you could do:
 * |[<!-- language="C" --> 
 * g_list_free_full (another_list, g_object_unref);
 * ]|
 *
 * Returns: the start of the new list that holds a full copy of @list, 
 *     use g_list_free_full() to free it
 *
 * Since: 2.34
 */
static inline GList *
g_list_copy_deep (GList     *list,
                  GCopyFunc  func,
                  gpointer   user_data)
{
  GList *new_list = NULL;

  if (list)
    {
      GList *last;

      new_list = g_slice_new (GList);
      if (func)
        new_list->data = func (list->data, user_data);
      else
        new_list->data = list->data;
      new_list->prev = NULL;
      last = new_list;
      list = list->next;
      while (list)
        {
          last->next = g_slice_new (GList);
          last->next->prev = last;
          last = last->next;
          if (func)
            last->data = func (list->data, user_data);
          else
            last->data = list->data;
          list = list->next;
        }
      last->next = NULL;
    }

  return new_list;
}

static inline void
g_list_free_full (GList          *list,
                  GDestroyNotify  free_func)
{
  g_list_foreach (list, (GFunc) free_func, NULL);
  g_list_free (list);
}

#endif
#endif /* __GLIB_COMPAT_H__ */
