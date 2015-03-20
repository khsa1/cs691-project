#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <glib.h>

#include <gspan.h>
#include <graph.h>

GList *find_frequent_node_labels(GList *database, int nsupport)
{
	int i, j, *key, *value;
	GHashTableIter iter;
	GHashTable *map;
	GList *ret = NULL;
	GList *l = NULL;

	map = g_hash_table_new(g_direct_hash, g_direct_equal);

	/* 
	 * Iterate over all graphs in the database, and determine how
 	 * in how many graphs a given not label appears.  This uses a 
 	 * GHashTable as a set
 	 */
	for (i = 0; i < g_list_length(database); i++) {
		struct graph *g;
		uint32_t *iter1;
		int *labels;
		uint32_t length;
		//GHashTable *gset;

		GList *gset = NULL;
		//gset = g_hash_table_new(g_int_hash, g_int_equal);

		g = (struct graph *)g_list_nth_data(database, i);

		for (j = 0; j < g_list_length(g->nodes); j++) {
			int32_t label = graph_get_node(g, j)->label;

			//printf("node label: %d\n", label);
			if (g_list_find(gset, GINT_TO_POINTER(label)))
				continue;
			gset = g_list_append(gset, GINT_TO_POINTER(label));
			//g_hash_table_add(gset, &graph_get_node(g, j)->label);
		}
		//labels = (int *)g_hash_table_get_keys_as_array(gset, &length);
		//printf("labels: %d %d %d\n", labels[0], labels[1], labels[2]);
		for (j = 0; j < g_list_length(gset); j++) {
			int val = 0;
			int32_t label = GPOINTER_TO_INT(g_list_nth_data(gset, j));
			
			if (!g_hash_table_lookup(map, GINT_TO_POINTER(label)))
				g_hash_table_insert(map, GINT_TO_POINTER(label),
							 GINT_TO_POINTER(val));
			else
				val = GPOINTER_TO_INT(g_hash_table_lookup(map, 
						GINT_TO_POINTER(label)));
			val++;
			//printf("inserting %d %d\n", label, val);
			g_hash_table_insert(map, GINT_TO_POINTER(label), 
							GINT_TO_POINTER(val));
		}
		g_list_free(gset);
		//g_free(labels);
		//g_hash_table_destroy(gset);	
	}

	g_hash_table_iter_init(&iter, map);
	while (g_hash_table_iter_next(&iter, (void **)&key, (void **)&value)) {
		//printf("%d %d %d ",nsupport,GPOINTER_TO_INT(key),
		//			GPOINTER_TO_INT(value));
		if (GPOINTER_TO_INT(value) < nsupport)
			continue;
		ret = g_list_append(ret, key);
	}
	g_hash_table_destroy(map);

	return ret;
}

int prune_infrequent_nodes(GList *database, GList *frequent_labels)
{
	GList *lg, *ln, *le;

	for (lg = g_list_first(database); lg; lg = g_list_next(lg)) {
		struct graph *g = (struct graph *)lg->data;
		int nodeid = 0;
		int *id_map_old_new;
		
		id_map_old_new = malloc(g_list_length(g->nodes)*sizeof(int));
		if (!id_map_old_new) {
			perror("allocating idmap\n");
			continue;
		}
		memset(id_map_old_new, 0xff, g_list_length(g->nodes)*sizeof(int));

		/* 
		 * Note, we have to process all the nodes before we can
		 * process the edges, even though its tempting to do it
		 * in one go.  You need to know the new node identities. 
		 */
		ln = g->nodes;		
		while(ln != NULL) {
			struct node *n = (struct node *)ln->data;
			GList *next = ln->next;

			if (!g_list_find(frequent_labels, 
						GINT_TO_POINTER(n->label))) {
				printf("removing node\n");
				node_free(n);
				g->nodes = g_list_delete_link(g->nodes, ln);
				ln = next;
				continue;
			}
			id_map_old_new[n->id] = nodeid;
			n->id = nodeid;
			nodeid ++;
			ln = next;
		}

		for (ln = g_list_first(g->nodes); ln; ln = g_list_next(ln)) {
			struct node *n = (struct node *)ln->data;
			int edgeid = 0;

			le = n->edges;
			while (le != NULL) {
				struct edge *e = (struct edge *)le->data;
				GList *enext = le->next;

				/* 
				 * If one of the nodes was deleted, delete
				 * the edge.
				 */
				if (id_map_old_new[e->from] == -1 ||
						id_map_old_new[e->to] == -1) {
					printf("removing edge\n");
					edge_free(e);
					n->edges = g_list_delete_link(
								n->edges, le);
					le = enext;
					continue;
				}
				/* 
				 * otherwise, update the from/to fields with
				 * the new known index
				 */
				e->to = id_map_old_new[e->to];
				e->from = id_map_old_new[e->from];
				e->id = edgeid;

				edgeid ++;

				le = enext;
			}
		}
	}
}
