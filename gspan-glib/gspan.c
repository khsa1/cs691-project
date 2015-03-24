#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <glib.h>

#include <gspan.h>
#include <graph.h>
#include <history.h>

void mine_subgraph(struct gspan *gs, GList *projection);

static void print_dfs(struct dfs_code *dfsc)
{
	printf("(from=%d, to=%d, from_label=%d, edge_label=%d, to_label=%d)", 
			dfsc->from, dfsc->to, dfsc->from_label, 
			dfsc->edge_label, dfsc->to_label);
}

static void print_dfs_list(GList *list)
{
	GList *l;
	int i;

	for (i=0,l = g_list_first(list); l; i++,l = g_list_next(l)) {
		struct dfs_code *dfsc = (struct dfs_code *)l->data;

		printf(" --- %d ", i);
		if (dfsc)
			print_dfs(dfsc);
		else 
			printf("NULL");
		printf("\n");
	}
}

static void print_pre_dfs(struct pre_dfs *pdfs)
{
	printf("(id=%d, edge=(%d,%d,%d), prev=%p)", pdfs->id, pdfs->edge->from, 
		pdfs->edge->to, pdfs->edge->label, pdfs->prev);
}

static void print_edge(struct edge *e)
{
	printf("(id=%d, from=%d, to=%d, labal=%d)",e->id, e->from, e->to, e->label);
}

GList *find_frequent_node_labels(GList *database, int nsupport, GHashTable *map)
{
	int i, j, *key, *value;
	GHashTableIter iter;
	GList *ret = NULL;
	GList *l = NULL;

	//map = g_hash_table_new(g_direct_hash, g_direct_equal);

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
				values = NULL;
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
		values = NULL;
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
				//printf("%d ---\n", g_list_length(values));
				values = g_list_append(values, npdfs);
				g_hash_table_insert(pm_forward, dfsc, values);
				values = NULL;
			}
		}
	}
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

unsigned int dfs_code_hash(const void *key)
{
	struct dfs_code *dfsc = (struct dfs_code *)key;
	unsigned int ret;

	ret = 0 | (dfsc->from << 24) | (dfsc->to << 16) | 
		(dfsc->from_label << 8) | (dfsc->edge_label << 0);

	/* 
	 * don't have enough bits on 32-bit to include to_label directly
	 * This just means a little worse hash table performance.
	 */ 
	return ret;
}

gboolean glib_dfs_code_equal(const void *a, const void *b)
{
	if (dfs_code_equal((const struct dfs_code *)a, 
				(const struct dfs_code *)b))
		return TRUE;
	return FALSE;
}

void cleanup_map(GHashTable *map)
{
	GList *lists, *l;

	lists = g_hash_table_get_values(map);

	for (l = g_list_first(lists); l; l = g_list_next(l)) {
		GList *to_delete = (GList *)l->data;

		g_list_free_full(to_delete, (GFreeFunc)free);
	}
	
	g_list_free(lists);

	lists = NULL;

	lists = g_hash_table_get_keys(map);
	for (l = g_list_first(lists); l; l = g_list_next(l)) {
		struct dfs_code *to_delete = (struct dfs_code *)l->data;

		free(to_delete);
	}
	
	g_list_free(lists);
	
	g_hash_table_destroy(map);

	return;
}

int is_min(struct gspan *gs)
{
	GHashTable *projection_map;
	GList *l1, *l2, *edges, *values = NULL;
	struct pre_dfs *pm;
	struct dfs_code *first_key, *start_code;
	int ret;

	printf("g_list_length(dfs_codes) = %d\n", g_list_length(gs->dfs_codes));
	if (g_list_length(gs->dfs_codes) == 1) {
		printf("heremin1\n");
		return 1;
	}

	g_list_free(gs->min_dfs_codes);
	gs->min_dfs_codes = NULL;

	if (gs->min_graph)
		graph_free(gs->min_graph);
	print_dfs_list(gs->dfs_codes);
	gs->min_graph = build_graph_dfs(gs->dfs_codes);

	projection_map = g_hash_table_new(dfs_code_hash, glib_dfs_code_equal);

	for (l1=g_list_first(gs->min_graph->nodes); l1; l1=g_list_next(l1)) {
		struct node *n = (struct node *)l1->data;

		edges = get_forward_init(n, gs->min_graph);

		if (g_list_length(edges) == 0) {
			g_list_free(edges);
			continue;
		}

		for (l2 = g_list_first(edges); l2; l2 = g_list_next(l2)) {
			struct edge *e = (struct edge *)l2->data;
			struct node *from_node, *to_node;
			struct dfs_code *dfsc;
			struct pre_dfs *npdfs;

			from_node = graph_get_node(gs->min_graph, e->from);
			to_node = graph_get_node(gs->min_graph, e->to);

			dfsc = malloc(sizeof(struct dfs_code));
			if (!dfsc) {
				perror("malloc dfsc in is_min");
				exit(1);
			}
			dfsc->from = 0;
			dfsc->to = 1;
			dfsc->from_label = from_node->label;
			dfsc->edge_label = e->label;
			dfsc->to_label = to_node->label;

			npdfs = malloc(sizeof(struct pre_dfs));
			if (!npdfs) {
				perror("malloc npdfs in is_min");
				exit(1);
			}
			npdfs->id = 0;
			npdfs->edge = e;
			npdfs->prev = NULL;

			if (g_hash_table_contains(projection_map, dfsc))
				values = g_hash_table_lookup(projection_map, 
									dfsc);
			values = g_list_append(values, npdfs);
			g_hash_table_insert(projection_map, dfsc, values);
			values = NULL;
		}

		//g_list_free(edges);
		//edges = NULL;
	}

	l2 = g_hash_table_get_keys(projection_map);
	l2 = g_list_sort(l2, (GCompareFunc)dfs_code_project_compare);
	first_key = (struct dfs_code *)g_list_first(l2)->data;
	g_list_free(l2);

	start_code = malloc(sizeof(struct dfs_code));
	if (!start_code) {
		perror("malloc start_code in is_min");
		exit(1);
	}
	start_code->from = 0;
	start_code->to = 1;
	start_code->from_label = first_key->from_label;
	start_code->edge_label = first_key->edge_label;
	start_code->to_label = first_key->to_label;

	gs->min_dfs_codes = g_list_append(gs->min_dfs_codes, start_code);
	print_dfs(g_list_nth_data(gs->dfs_codes, 
					g_list_length(gs->min_dfs_codes)-1));
	printf("\n");
	print_dfs(g_list_last(gs->min_dfs_codes)->data);
	printf("\n");
	if (!dfs_code_equal(
				g_list_nth_data(gs->dfs_codes, 
					g_list_length(gs->min_dfs_codes)-1), 
				g_list_last(gs->min_dfs_codes)->data)) {
		printf("heremin2\n");
		return 0;
	}
	
	values = g_hash_table_lookup(projection_map, start_code);
	ret = projection_min(gs, values);
	
	cleanup_map(projection_map);
	//g_list_free(values);
	printf("ret = %d\n", ret);
	return ret;
}

int judge_backwards(struct gspan *gs, GList *right_most_path, GList *projection,
						GHashTable *pm_backwards)
{
	GList *l1, *l2, *l3, *values = NULL;
	int i;
	int rmp0;

	rmp0 = 	GPOINTER_TO_INT(g_list_nth_data(right_most_path, 0));

	for (i=g_list_length(right_most_path)-1,l1=g_list_last(right_most_path);
					i > 1; i--, l1 = g_list_previous(l1)) {
		int rmp = GPOINTER_TO_INT(l1->data);

		for(l2 = g_list_first(projection); l2; l2 = g_list_next(l2)) {
			struct pre_dfs *p = (struct pre_dfs *)l2->data;
			struct history *h;
			struct edge *last_edge;
			struct node *last_node, *to_node, *from_node;
			struct edge *edge;
			
			h = alloc_history();
			build_history(h, p);

			last_edge = (struct edge *)
					g_list_nth_data(h->edges, rmp0);
			last_node = graph_get_node(gs->min_graph, 
								last_edge->to);
			edge = (struct edge *)
					g_list_nth_data(h->edges, rmp);
			to_node = graph_get_node(gs->min_graph, edge->to);
			from_node = graph_get_node(gs->min_graph, edge->from);

			for (l3 = g_list_first(last_node->edges); l3; 
							l3 = g_list_next(l3)) {
				struct edge *e = (struct edge *)l3->data;

				if (g_hash_table_contains(h->has_edges, 
							GINT_TO_POINTER(e->id)))
					continue;

				if (!g_hash_table_contains(h->has_nodes, 
							GINT_TO_POINTER(e->to)))
					continue;

				if (e->to == edge->from && 
						(e->label > edge->label || 
						(e->label == edge->label &&
							last_node->label > 
							to_node->label))) {
					struct dfs_code *dfsc;
					struct pre_dfs *npdfs;
					int from_id;
					int to_id;

					from_id = ((struct dfs_code *)
							g_list_nth_data(
							gs->min_dfs_codes, 
							rmp0))->to;

					to_id = ((struct dfs_code *)
							g_list_nth_data(
							gs->min_dfs_codes, 
							rmp0))->from;

					dfsc = malloc(sizeof(struct dfs_code));
					if (!dfsc) {
						perror("malloc dfsc in jb()");
						exit(1);
					}
					dfsc->from = from_id;
					dfsc->to = to_id;
					dfsc->from_label = last_node->label;
					dfsc->edge_label = e->label;
					dfsc->to_label = from_node->label;

					npdfs = malloc(sizeof(struct pre_dfs));
					if (!npdfs) {
						perror("malloc npdfs in jb()");
						exit(1);
					}
					npdfs->id = 0;
					npdfs->edge = e;
					npdfs->prev = p;

					if (g_hash_table_contains(pm_backwards, 
									dfsc))
						values = g_hash_table_lookup(
							pm_backwards,dfsc);
					values = g_list_append(values, npdfs);
					g_hash_table_insert(pm_backwards, dfsc,
									values);
					values = NULL;

				}
			}
			free_history(h);
		}

		if (g_hash_table_size(pm_backwards) > 0)
			return 1;
	}

	return 0;
}

int judge_forwards(struct gspan *gs, GList *right_most_path, GList *projection,
					GHashTable *pm_forwards, int min_label)
{
	int i, rmp0;
	struct history *h;
	GList *l1, *l2, *l3, *l4, *values = NULL;

	rmp0 = 	GPOINTER_TO_INT(g_list_nth_data(right_most_path, 0));

	for(i=0,l1 = g_list_first(projection); l1; l1 = g_list_next(l1)) {
		struct pre_dfs *p = (struct pre_dfs *)l1->data;
		struct node *last_node;
		struct edge *last_edge;
	
		h = alloc_history();
		build_history(h,p);

		last_edge = g_list_nth_data(h->edges, rmp0);
		last_node = graph_get_node(gs->min_graph, last_edge->to);

		for (l2 = g_list_first(last_node->edges); l2; 
						l2 = g_list_next(l2)) {
			struct edge *e = (struct edge *)l2->data;
			struct node *to_node;
			struct dfs_code *dfsc;
			struct pre_dfs *npdfs;
			int to_id;
			
			to_node = graph_get_node(gs->min_graph, e->to);

			if (g_hash_table_contains(h->has_nodes, 
						GINT_TO_POINTER(e->to)) || 
						to_node->label < min_label)
				continue;
			
			to_id = ((struct dfs_code *)g_list_nth_data(
						gs->min_dfs_codes, rmp0))->to;

			dfsc = malloc(sizeof(struct dfs_code));
			if (!dfsc) {
				perror("malloc dfsc in jf()");
				exit(1);
			}
			dfsc->from = to_id;
			dfsc->to = to_id + 1;
			dfsc->from_label = last_node->label;
			dfsc->edge_label = e->label;
			dfsc->to_label = to_node->label;

			npdfs = malloc(sizeof(struct pre_dfs));
			if (!npdfs) {
				perror("malloc npdfs in jf()");
				exit(1);
			}
			npdfs->id = 0;
			npdfs->edge = e;
			npdfs->prev = p;

			if (g_hash_table_contains(pm_forwards, dfsc))
				values = g_hash_table_lookup(
							pm_forwards,dfsc);
			values = g_list_append(values, npdfs);
			g_hash_table_insert(pm_forwards, dfsc, values);
			values = NULL;

		}

		free_history(h);
	}

	
	if (g_hash_table_size(pm_forwards) == 0) {
		for(l1 = g_list_first(right_most_path); l1; 
							l1 = g_list_next(l1)) {
			int rmp = GPOINTER_TO_INT(l1->data);
			
			for (l2=g_list_first(l2); l2; l2 = g_list_next(l2)) {
				struct pre_dfs *p = (struct pre_dfs *)l2->data;
				struct edge *cur_edge;
				struct node *cur_node;
				struct node *cur_to;

				h = alloc_history();
				build_history(h, p);

				cur_edge = g_list_nth_data(h->edges, rmp);
				cur_node = graph_get_node(gs->min_graph, 
								cur_edge->from);
				cur_to = graph_get_node(gs->min_graph, 
								cur_edge->to);

				for (l3 = g_list_first(cur_node->edges); l3;
							l3 = g_list_next(l3)) {
					struct edge *e = (struct edge *)l3->data;
					struct node *to_node;

					to_node = graph_get_node(gs->min_graph, e->to);

					if (cur_edge->to == to_node->id || 
							g_hash_table_contains(h->has_nodes, 
									GINT_TO_POINTER(e->id)) || 
									to_node->label < min_label)
						continue;

					if (cur_edge->label < e->label || 
							(cur_edge->label == e->label &&
							cur_to->label <= to_node->label)) {
						int from_id, to_id;
						struct dfs_code *dfsc;
						struct pre_dfs *npdfs;

						from_id = ((struct dfs_code *)g_list_nth_data(
								gs->min_dfs_codes, rmp))->from;

						to_id = ((struct dfs_code *)g_list_nth_data(
								gs->min_dfs_codes, rmp0))->to;

						dfsc = malloc(sizeof(struct dfs_code));
						if (!dfsc) {
							perror("malloc dfsc in jf()");
							exit(1);
						}
						dfsc->from = from_id;
						dfsc->to = to_id + 1;
						dfsc->from_label = cur_node->label;
						dfsc->edge_label = e->label;
						dfsc->to_label = to_node->label;

						npdfs = malloc(sizeof(struct pre_dfs));
						if (!npdfs) {
							perror("malloc npdfs in jf()");
							exit(1);
						}
						npdfs->id = 0;
						npdfs->edge = e;
						npdfs->prev = p;

						if (g_hash_table_contains(pm_forwards, dfsc))
							values = g_hash_table_lookup(
									pm_forwards,dfsc);
						values = g_list_append(values, npdfs);
						g_hash_table_insert(pm_forwards, dfsc, values);
						values = NULL;
					}
				}

				free_history(h);
			}

			if (g_hash_table_size(pm_forwards) > 0) 
				break;
		}
	}

	if (g_hash_table_size(pm_forwards) > 0) 
		return 1;
	return 0;
}



int projection_min(struct gspan *gs, GList *projection)
{
	GList *right_most_path, *l1, *keys, *values = NULL;
	int min_label;
	int ret;
	GHashTable *pm_backwards, *pm_forwards;

	right_most_path = build_right_most_path(gs->min_dfs_codes);
	min_label = ((struct dfs_code *)
				g_list_first(gs->min_dfs_codes))->from_label;

	pm_backwards = g_hash_table_new(dfs_code_hash, glib_dfs_code_equal);
	pm_forwards = g_hash_table_new(dfs_code_hash, glib_dfs_code_equal);

	ret = judge_backwards(gs, right_most_path, projection, pm_backwards);
	if (ret) {
		keys = g_hash_table_get_keys(pm_backwards);
		keys = g_list_sort(keys, (GCompareFunc)dfs_code_backward_compare);
		for (l1 = g_list_first(keys); l1; l1 = g_list_next(l1)) {
			struct dfs_code *dfsc = (struct dfs_code *)l1->data;

			gs->min_dfs_codes = g_list_append(gs->min_dfs_codes, 
									dfsc);
			if (!dfs_code_equal(
				g_list_nth_data(gs->dfs_codes, 
					g_list_length(gs->min_dfs_codes)-1), 
					g_list_last(gs->min_dfs_codes)->data)) {

				g_list_free(keys);
				cleanup_map(pm_backwards);
				g_list_free(right_most_path);
				return 0;
			}
		
			values = g_hash_table_lookup(pm_backwards, dfsc);
			ret = projection_min(gs, values);

			g_list_free(keys);
			cleanup_map(pm_backwards);
			cleanup_map(pm_forwards);
			g_list_free(right_most_path);
			return ret;
		}
		g_list_free(keys);
	}
	
	ret = judge_forwards(gs, right_most_path, projection, 
						pm_forwards, min_label);
	if (ret) {
		keys = g_hash_table_get_keys(pm_forwards);
		keys = g_list_sort(keys, (GCompareFunc)dfs_code_forward_compare);
		for (l1 = g_list_first(keys); l1; l1 = g_list_next(l1)) {
			struct dfs_code *dfsc = (struct dfs_code *)l1->data;

			gs->min_dfs_codes = g_list_append(gs->min_dfs_codes, 
									dfsc);
			if (!dfs_code_equal(
				g_list_nth_data(gs->dfs_codes, 
					g_list_length(gs->min_dfs_codes)-1), 
					g_list_last(gs->min_dfs_codes)->data)) {

				g_list_free(keys);
				cleanup_map(pm_forwards);
				g_list_free(right_most_path);
				return 0;
			}
		
			values = g_hash_table_lookup(pm_forwards, dfsc);
			ret = projection_min(gs, values);

			g_list_free(keys);
			cleanup_map(pm_backwards);
			cleanup_map(pm_forwards);
			g_list_free(right_most_path);
			return ret;
		}
		g_list_free(keys);
	}

	g_list_free(right_most_path);
	cleanup_map(pm_backwards);
	cleanup_map(pm_forwards);
	return 1;
}

void show_subgraph(GList *dfs_codes, int nsupport)
{
	struct graph *g;
	
	g = build_graph_dfs(dfs_codes);
	print_graph(g, nsupport);
	graph_free(g);

	return;
}

int project(struct gspan *gs, GList *frequent_nodes, GHashTable *freq_labels)
{
	GHashTable *projection_map;
	GList *l1, *l2, *l3, *edges, *values=NULL, *keys;
	struct pre_dfs *pm;
	struct dfs_code *first_key, *start_code;
	int ret;

	if (gs->dfs_codes != NULL)
		g_list_free_full(gs->dfs_codes, (GDestroyNotify)free);
	gs->dfs_codes = NULL;

	for (l1 = g_list_first(frequent_nodes); l1; l1 = g_list_next(l1)) {
		int nodelabel = GPOINTER_TO_INT(l1->data);
		int nodesup = GPOINTER_TO_INT(
				g_hash_table_lookup(freq_labels, 
					GINT_TO_POINTER(nodelabel)));

		print_graph_node(nodelabel, nodesup);
	}

	projection_map = g_hash_table_new(dfs_code_hash, glib_dfs_code_equal);

	for(l1 = g_list_first(gs->database); l1; l1 = g_list_next(l1)) {
		struct graph *g = (struct graph *)l1->data;

		for (l2 = g_list_first(g->nodes); l2; l2 = g_list_next(l2)) {
			struct node *n = (struct node *)l2->data;
			GList *edges;

			edges = get_forward_init(n, g);

			if (g_list_length(edges) == 0) {
				g_list_free(edges);
				continue;
			}
			for(l3 = g_list_first(edges); l3; l3=g_list_next(l3)) {
				struct edge *e = (struct edge *)l3->data;
				struct node *from_node;
				struct node *to_node;
				struct dfs_code *dfsc;
				struct pre_dfs *npdfs;

				from_node = graph_get_node(g, e->from);
				to_node = graph_get_node(g, e->to);

				dfsc = malloc(sizeof(struct dfs_code));
				if (!dfsc) {
					perror("malloc dfsc in jf()");
					exit(1);
				}
				dfsc->from = 0;
				dfsc->to = 1;
				dfsc->from_label = from_node->label;
				dfsc->edge_label = e->label;
				dfsc->to_label = to_node->label;
	
				npdfs = malloc(sizeof(struct pre_dfs));
				if (!npdfs) {
					perror("malloc npdfs in jf()");
					exit(1);
				}
				npdfs->id = g->id;
				npdfs->edge = e;
				npdfs->prev = NULL;

				//print_dfs(dfsc);
				//printf("\n");
				//fflush(stdout);
	
				if (g_hash_table_contains(projection_map, dfsc)) {
					//printf("Here1\n");
					values = g_hash_table_lookup(
							projection_map, dfsc);
				}
				values = g_list_append(values, npdfs);
				g_hash_table_insert(projection_map, dfsc, 
									values);
				values = NULL;
			}

			g_list_free(edges);
			edges = NULL;
		}
	}

	keys = g_hash_table_get_keys(projection_map);
	keys = g_list_sort(keys, (GCompareFunc)dfs_code_project_compare);

	for (l1 = g_list_last(keys); l1; l1 = g_list_previous(l1)) {
		struct dfs_code *dfsc = (struct dfs_code *)l1->data;
		struct dfs_code *start_code; 

		values = g_hash_table_lookup(projection_map, dfsc);

		if (g_list_length(values) < gs->nsupport) {
			//g_list_free(values);
			continue;
		}

		print_dfs(dfsc);
		printf(" %d\n", g_list_length(values));


		start_code = malloc(sizeof(struct dfs_code));
		if (!start_code) {
			perror("malloc start_code in project");
			exit(1);
		}
		start_code->from = 0;
		start_code->to = 1;
		start_code->from_label = dfsc->from_label;
		start_code->edge_label = dfsc->edge_label;
		start_code->to_label = dfsc->to_label;

		gs->dfs_codes = g_list_append(gs->dfs_codes, start_code);

		mine_subgraph(gs, values);

		l2 = g_list_last(gs->dfs_codes);
		gs->dfs_codes = g_list_remove_link(gs->dfs_codes, l2);
		free((struct dfs_code *)l2->data);
		g_list_free(l2);
	}

	cleanup_map(projection_map);

	return 0;
}

void mine_subgraph(struct gspan *gs, GList *projection)
{
	int support;
	GList *right_most_path, *keys, *values = NULL, *l1, *l2;
	int min_label;
	GHashTable *pm_forwards, *pm_backwards;

	support = count_support(projection);
	if (support < gs->nsupport) {
		printf("here1\n");
		return;
	}

	if (!is_min(gs)) {
		printf("here2\n");
		return;
	}

	show_subgraph(gs->dfs_codes, support);

	right_most_path = build_right_most_path(gs->dfs_codes);
	min_label = ((struct dfs_code *)g_list_first(gs->dfs_codes)->data)->from_label;

	pm_forwards = g_hash_table_new(dfs_code_hash, glib_dfs_code_equal);
	pm_backwards = g_hash_table_new(dfs_code_hash, glib_dfs_code_equal);

	enumerate(gs, projection, right_most_path, pm_backwards, pm_forwards, 
								min_label);

	keys = g_hash_table_get_keys(pm_backwards);
	keys = g_list_sort(keys, (GCompareFunc)dfs_code_backward_compare);

	for (l1 = g_list_first(keys); l1; l1 = g_list_next(l1)) {
		struct dfs_code *dfsc = (struct dfs_code *)l1->data;
		printf("backward  ");
		print_dfs(dfsc);
		printf("\n");
		values = g_hash_table_lookup(pm_backwards, dfsc);

		gs->dfs_codes = g_list_append(gs->dfs_codes, dfsc);

		mine_subgraph(gs, values);

		l2 = g_list_last(gs->dfs_codes);
		gs->dfs_codes = g_list_remove_link(gs->dfs_codes, l2);
		free((struct dfs_code *)l2->data);
		g_list_free(l2);
		//g_list_free(values);
		values = NULL;
	}

	g_list_free(keys);

	keys = g_hash_table_get_keys(pm_forwards);
	keys = g_list_sort(keys, (GCompareFunc)dfs_code_forward_compare);

	for (l1 = g_list_last(keys); l1; l1 = g_list_previous(l1)) {
		struct dfs_code *dfsc = (struct dfs_code *)l1->data;
		printf("forward  ");
		print_dfs(dfsc);
		printf("\n");

		values = g_hash_table_lookup(pm_forwards, dfsc);

		gs->dfs_codes = g_list_append(gs->dfs_codes, dfsc);

		mine_subgraph(gs, values);

		l2 = g_list_last(gs->dfs_codes);
		gs->dfs_codes = g_list_remove_link(gs->dfs_codes, l2);
		//free((struct dfs_code *)l2->data);
		g_list_free(l2);
		//g_list_free(values);
		values = NULL;
	}

	g_list_free(keys);
	cleanup_map(pm_backwards);
	cleanup_map(pm_forwards);
	g_list_free(right_most_path);

	return;
}
