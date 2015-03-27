/*
 * minimum.c: Check whether the candidate dfs codes are a minimum DFS code
 *
 * Author: John Clemens <clemej1 at umbc.edu>
 * Copyright (c) 2015
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <glib.h>

#include <graph.h>
#include <gspan.h>
#include <history.h>

static int judge_backwards(struct gspan *gs, GList *right_most_path, GList *projection,
						GHashTable *pm_backwards)
{
	GList *l1, *l2, *l3, *values = NULL;
	int i;
	int rmp0;

	rmp0 = 	GPOINTER_TO_INT(g_list_nth_data(right_most_path, 0));

	for (i=g_list_length(right_most_path)-1,l1=g_list_last(right_most_path);
					i > 0; i--, l1 = g_list_previous(l1)) {
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

static int judge_forwards(struct gspan *gs, GList *right_most_path, GList *projection,
					GHashTable *pm_forwards, int min_label)
{
	int rmp0;
	struct history *h;
	GList *l1, *l2, *l3, *l4, *values = NULL;

	rmp0 = 	GPOINTER_TO_INT(g_list_nth_data(right_most_path, 0));

	for(l1 = g_list_first(projection); l1; l1 = g_list_next(l1)) {
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
			
			for (l2=g_list_first(projection); l2; l2 = g_list_next(l2)) {
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


static int projection_min(struct gspan *gs, GList *projection)
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
	printf("jb %d ", ret);
	if (ret) {
		keys = g_hash_table_get_keys(pm_backwards);
		keys = g_list_sort(keys, (GCompareFunc)dfs_code_backward_compare);
		print_dfs_list(keys);
		printf("\n");
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
				cleanup_map(pm_forwards);
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
	} else {printf("\n");}
	
	ret = judge_forwards(gs, right_most_path, projection, 
						pm_forwards, min_label);
	printf("jf %d", ret);
	if (ret) {
		keys = g_hash_table_get_keys(pm_forwards);
		keys = g_list_sort(keys, (GCompareFunc)dfs_code_forward_compare);

		print_dfs_list(keys);
		printf("\n");
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
				cleanup_map(pm_backwards);
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
	} else { printf("\n"); }

	g_list_free(right_most_path);
	cleanup_map(pm_backwards);
	cleanup_map(pm_forwards);
	return 1;
}

int is_min(struct gspan *gs)
{
	GHashTable *projection_map;
	GList *l1, *l2, *edges, *values = NULL;
	struct pre_dfs *pm;
	struct dfs_code *first_key, *start_code;
	int ret;

	//printf("g_list_length(dfs_codes) = %d\n", g_list_length(gs->dfs_codes));
	if (g_list_length(gs->dfs_codes) == 1) {
	//	printf("heremin1\n");
		return 1;
	}

	g_list_free(gs->min_dfs_codes);
	gs->min_dfs_codes = NULL;

	if (gs->min_graph)
		graph_free(gs->min_graph);
	//print_dfs_list(gs->dfs_codes);
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
	//print_dfs(g_list_nth_data(gs->dfs_codes, 
	//				g_list_length(gs->min_dfs_codes)-1));
	//printf("\n");
	//print_dfs(g_list_last(gs->min_dfs_codes)->data);
	//printf("\n");
	if (!dfs_code_equal(
				g_list_nth_data(gs->dfs_codes, 
					g_list_length(gs->min_dfs_codes)-1), 
				g_list_last(gs->min_dfs_codes)->data)) {
		//printf("heremin2\n");
		return 0;
	}
	
	values = g_hash_table_lookup(projection_map, start_code);
	ret = projection_min(gs, values);
	
	cleanup_map(projection_map);
	//g_list_free(values);
	//printf("ret = %d\n", ret);
	return ret;
}


