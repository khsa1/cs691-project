#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <glib.h>

#include <gspan.h>
#include <graph.h>
#include <history.h>

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
			int32_t label = GPOINTER_TO_INT(
						g_list_nth_data(gset, j));
			
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


GList *build_right_most_path(GList *dfs_codes)
{
	GList *iter, *ret = NULL;
	int prev_id = -1;
	int i;
	
	for (i=g_list_length(dfs_codes)-1, iter = g_list_last(dfs_codes); 
			iter; i--, iter = g_list_previous(iter)) {

		struct dfs_code *dfsc = (struct dfs_code *)iter->data;
		if (dfsc->from < dfsc->to && 
			(g_list_length(ret) == 0 || prev_id == dfsc->to)) {
			
			prev_id = dfsc->from;
			ret = g_list_append(ret, GINT_TO_POINTER(i));
		}
	}

	return ret;
}

void enumerate(struct gspan *gs, GList *projections, GList *right_most_path,
			GHashTable *pm_backward, GHashTable *pm_forward, 
			int min_label)
{
	GList *l;

	for (l = g_list_first(projections); l; l = g_list_next(l)) {
		struct pre_dfs *p = (struct pre_dfs *)l->data;
		struct history *h;

		h = alloc_history();
		build_history(h, p);

		get_backward(gs, p, right_most_path, h, pm_backward);
		get_first_forward(gs, p, right_most_path, h, pm_forward, 
								min_label);
		get_other_forward(gs, p, right_most_path, h, pm_forward, 
								min_label);
		free_history(h);
	}

	return;
}

GList *get_forward_init(struct node *n, struct graph *g)
{
	GList *ret = NULL;
	GList *l;

	for(l = g_list_first(n->edges); l; l = g_list_next(l)) {
		struct edge *e = (struct edge *)l->data;

		if (n->label <= graph_get_node(g, e->to)->label)
			ret = g_list_append(ret, e);
	}

	return ret;
}

void get_backward(struct gspan *gs, struct pre_dfs *pdfs, 
			GList *right_most_path, struct history *hist, 
			GHashTable *pm_backward)
{
	struct edge *last_edge;
	struct graph *g;
	struct node *last_node;
	int rmp0;
	int i;
	GList *l1, *l2, *values = NULL;

	rmp0 = 	GPOINTER_TO_INT(g_list_nth_data(right_most_path, 0));
	last_edge = g_list_nth_data(hist->edges, rmp0);
	g = g_list_nth_data(gs->database, pdfs->id);
	last_node = graph_get_node(g, last_edge->to);

	for (i=g_list_length(right_most_path)-1,l1=g_list_last(right_most_path);
					i > 1; i--, l1 = g_list_previous(l1)) {
		int rmp = GPOINTER_TO_INT(l1->data);
		struct edge *edge = g_list_nth_data(hist->edges, rmp);
		struct node *from_node;
		struct node *to_node;

		for (l2 = g_list_first(last_node->edges); l2; 
							l2 = g_list_next(l2)) {
			struct edge *e = (struct edge *)l2->data;

			if (g_hash_table_contains(hist->has_edges, 
							GINT_TO_POINTER(e->id)))
				continue;

			if (!g_hash_table_contains(hist->has_nodes, 
							GINT_TO_POINTER(e->to)))
				continue;

			from_node = graph_get_node(g, edge->from);
			to_node = graph_get_node(g, edge->to);

			if (e->to == edge->from && (e->label > edge->label || 
					(e->label == edge->label && 
					last_node->label >= to_node->label))) {

				struct dfs_code *dfsc;
				dfsc = malloc(sizeof(struct dfs_code));
				if (!dfsc) {
					perror("malloc dfsc in get_backwards");
					exit(1);
				}
				dfsc->from = ((struct dfs_code *)
						g_list_nth_data(gs->dfs_codes, 
								rmp0))->to;
				dfsc->to = ((struct dfs_code *)
						g_list_nth_data(gs->dfs_codes, 
								rmp))->from;
				dfsc->from_label = last_node->label;
				dfsc->edge_label = e->label;
				dfsc->to_label = from_node->label;

				struct pre_dfs *npdfs;
				npdfs = malloc(sizeof(struct pre_dfs));
				if (!npdfs) {
					perror("malloc npdfs in get_backwards");
					exit(1);
				}
				npdfs->id = g->id;
				npdfs->edge = e;
				npdfs->prev = pdfs;

				if (g_hash_table_contains(pm_backward, dfsc))
					values = g_hash_table_lookup(
							pm_backward, dfsc);
				values = g_list_append(values, npdfs);
				g_hash_table_insert(pm_backward, dfsc, values);
			}
		}
	}

	return;
}

void get_first_forward(struct gspan *gs, struct pre_dfs *pdfs, 
			GList *right_most_path, struct history *hist, 
			GHashTable *pm_forward, int min_label)
{
	struct edge *last_edge;
	struct graph *g;
	struct node *last_node;
	int rmp0;
	int i;
	GList *l1, *values = NULL;

	rmp0 = 	GPOINTER_TO_INT(g_list_nth_data(right_most_path, 0));
	last_edge = g_list_nth_data(hist->edges, rmp0);
	g = g_list_nth_data(gs->database, pdfs->id);
	last_node = graph_get_node(g, last_edge->to);

	for (l1 = g_list_first(last_node->edges); l1; l1 = g_list_next(l1)) {
		struct edge *e = (struct edge *)l1->data;
		struct node *to_node;
		struct dfs_code *dfsc;
		struct pre_dfs *npdfs;
		int to_id;

		to_node = graph_get_node(g, e->to);

		if (g_hash_table_contains(hist->has_nodes, GINT_TO_POINTER(
				e->to)) || to_node->label < min_label)
			continue;

		to_id = ((struct dfs_code *)g_list_nth_data(gs->dfs_codes, 
								rmp0))->to;
		dfsc = malloc(sizeof(struct dfs_code));
		if (!dfsc) {
			perror("malloc dfsc in get_backwards");
			exit(1);
		}
		dfsc->from = to_id;
		dfsc->to = to_id+1;
		dfsc->from_label = last_node->label;
		dfsc->edge_label = e->label;
		dfsc->to_label = to_node->label;

		npdfs = malloc(sizeof(struct pre_dfs));
		if (!npdfs) {
			perror("malloc npdfs in get_backwards");
			exit(1);
		}
		npdfs->id = g->id;
		npdfs->edge = e;
		npdfs->prev = pdfs;

		if (g_hash_table_contains(pm_forward, dfsc))
			values = g_hash_table_lookup(pm_forward, dfsc);
		values = g_list_append(values, npdfs);
		g_hash_table_insert(pm_forward, dfsc, values);
	}

	return;
}


void get_other_forward(struct gspan *gs, struct pre_dfs *pdfs, 
			GList *right_most_path, struct history *hist, 
			GHashTable *pm_forward, int min_label)
{
	struct graph *g;
	int rmp0;
	GList *l1, *l2, *l3, *values = NULL;

	rmp0 = 	GPOINTER_TO_INT(g_list_nth_data(right_most_path, 0));
	g = g_list_nth_data(gs->database, pdfs->id);
	
	for (l1 = g_list_first(right_most_path); l1; l1 = g_list_next(l1)) {
		int rmp = GPOINTER_TO_INT(l1->data);
		struct edge *cur_edge;
		struct node *cur_node, *cur_to;

		cur_edge = g_list_nth_data(hist->edges, rmp0);
		cur_node = graph_get_node(g, cur_edge->from);
		cur_to = graph_get_node(g, cur_edge->to);

		for (l2 = g_list_first(cur_node->edges); l2; 
							l2 = g_list_next(l2)) {
			struct edge *e = (struct edge *)l2->data;
			struct node *to_node = graph_get_node(g, e->to);

			if (to_node->id == cur_to->id || 
					g_hash_table_contains(hist->has_nodes, 
					GINT_TO_POINTER(to_node->id)) || 
						to_node->label < min_label) 
				continue;

			if (cur_edge->label < e->label || 
					(cur_edge->label == e->label && 
					cur_to->label <= to_node->label)) {

				struct dfs_code *dfsc;
				dfsc = malloc(sizeof(struct dfs_code));
				if (!dfsc) {
					perror("malloc dfsc in get_backwards");
					exit(1);
				}
				dfsc->from = ((struct dfs_code *)
						g_list_nth_data(gs->dfs_codes, 
								rmp))->from;
				dfsc->to = ((struct dfs_code *)
						g_list_nth_data(gs->dfs_codes, 
								rmp0))->to + 1;
				dfsc->from_label = cur_node->label;
				dfsc->edge_label = e->label;
				dfsc->to_label = to_node->label;

				struct pre_dfs *npdfs;
				npdfs = malloc(sizeof(struct pre_dfs));
				if (!npdfs) {
					perror("malloc npdfs in get_backwards");
					exit(1);
				}
				npdfs->id = g->id;
				npdfs->edge = e;
				npdfs->prev = pdfs;

				if (g_hash_table_contains(pm_forward, dfsc))
					values = g_hash_table_lookup(
							pm_forward, dfsc);
				values = g_list_append(values, npdfs);
				g_hash_table_insert(pm_forward, dfsc, values);
			}
		}
	}
}

int count_support(GList *pdfs)
{
	GList *l;
	int size = 0;
	int prev_id = -1;
	
	for (l = g_list_first(pdfs); l; l = g_list_next(l)) {
		struct pre_dfs *p = (struct pre_dfs *)l->data;

		if (prev_id != p->id) {
			prev_id = p->id;
			size += 1;
		}
	}
	return size;
}

