/* 
 * extend.c: Extend each growing subgraph by another edge
 *
 * Author: John Clemens <clemej1 at umbc.edu>
 * Copyrigt (c) 2015
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <glib.h>

#include <graph.h>
#include <gspan.h>
#include <history.h>
#include <glib_compat.h>

GList *get_forward_init(struct node *n, struct graph *g)
{
	GList *ret = NULL;
	GList *l;

	for(l = g_list_first(n->edges); l; l = g_list_next(l)) {
		struct edge *e = (struct edge *)l->data;

		if (n->label <= graph_get_node(g, e->to)->label) {
			ret = g_list_append(ret, e);
			//print_edge(e);
		}
	}

	return ret;
}

static void get_backward(struct gspan *gs, struct pre_dfs *pdfs, 
			GList *right_most_path, struct history *hist, 
			GHashTable *pm_backward)
{
	struct edge *last_edge;
	struct graph *g;
	struct node *last_node;
	int rmp0;
	int i;
	GList *l1, *l2;
	GQueue *values = NULL;

	rmp0 = 	GPOINTER_TO_INT(g_list_nth_data(right_most_path, 0));
	last_edge = g_list_nth_data(hist->edges, rmp0);
	g = g_list_nth_data(gs->database, pdfs->id);
	last_node = graph_get_node(g, last_edge->to);

	for (i=g_list_length(right_most_path)-1,l1=g_list_last(right_most_path);
					i > 0; i--, l1 = g_list_previous(l1)) {
		int rmp = GPOINTER_TO_INT(l1->data);
		struct edge *edge = g_list_nth_data(hist->edges, rmp);
		struct node *from_node;
		struct node *to_node;

		for (l2 = g_list_first(last_node->edges); l2; 
							l2 = g_list_next(l2)) {
			struct edge *e = (struct edge *)l2->data;

			if (g_list_find(hist->has_edges, 
							GINT_TO_POINTER(e->id))) {
				//printf("here4 -- contains\n");
				continue;
			}

			if (!g_list_find(hist->has_nodes, 
							GINT_TO_POINTER(e->to))) {
				//printf("here4 -- not\n");
				continue;
			}

			from_node = graph_get_node(g, edge->from);
			to_node = graph_get_node(g, edge->to);
			//printf("here3\n");
			if (e->to == edge->from && (e->label > edge->label || 
					(e->label == edge->label && 
					last_node->label >= to_node->label))) {

				struct dfs_code *dfsc;
				//printf("here4\n");
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
				else
					values = g_queue_new();

				g_queue_push_tail(values, npdfs);
				g_hash_table_insert(pm_backward, dfsc, values);
			}
		}
	}

	return;
}

static void get_first_forward(struct gspan *gs, struct pre_dfs *pdfs, 
			GList *right_most_path, struct history *hist, 
			GHashTable *pm_forward, int min_label)
{
	struct edge *last_edge;
	struct graph *g;
	struct node *last_node;
	int rmp0;
	int i;
	GList *l1;
	GQueue *values = NULL;

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

		if (g_list_find(hist->has_nodes, GINT_TO_POINTER(
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
		else
			values = g_queue_new();

		g_queue_push_tail(values, npdfs);
		g_hash_table_insert(pm_forward, dfsc, values);
	}

	return;
}


static void get_other_forward(struct gspan *gs, struct pre_dfs *pdfs, 
			GList *right_most_path, struct history *hist, 
			GHashTable *pm_forward, int min_label)
{
	struct graph *g;
	int rmp0;
	GList *l1, *l2, *l3;
	GQueue *values = NULL;

	rmp0 = 	GPOINTER_TO_INT(g_list_nth_data(right_most_path, 0));
	g = g_list_nth_data(gs->database, pdfs->id);
	
	for (l1 = g_list_first(right_most_path); l1; l1 = g_list_next(l1)) {
		int rmp = GPOINTER_TO_INT(l1->data);
		struct edge *cur_edge;
		struct node *cur_node, *cur_to;

		cur_edge = g_list_nth_data(hist->edges, rmp);
		cur_node = graph_get_node(g, cur_edge->from);
		cur_to = graph_get_node(g, cur_edge->to);

		for (l2 = g_list_first(cur_node->edges); l2; 
							l2 = g_list_next(l2)) {
			struct edge *e = (struct edge *)l2->data;
			struct node *to_node = graph_get_node(g, e->to);

			if (to_node->id == cur_to->id || 
					g_list_find(hist->has_nodes, 
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
				else
					values = g_queue_new();

				//printf("%d ---\n", g_list_length(values));
				g_queue_push_tail(values, npdfs);
				g_hash_table_insert(pm_forward, dfsc, values);
			}
		}
	}
}

void enumerate(struct gspan *gs, GQueue *projections, GList *right_most_path,
			GHashTable *pm_backward, GHashTable *pm_forward, 
			int min_label)
{
	GList *l;

	for (l = g_list_first(projections->head); l; l = g_list_next(l)) {
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


