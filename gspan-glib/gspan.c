/* 
 * gspan.c: Core gspan algorithm implementation.
 *
 * Author: John Clemens <clemej1 at umbc.edu>
 * Copyrigt (c) 2015
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <glib.h>

#include <glib_compat.h>
#include <gspan.h>
#include <graph.h>
#include <history.h>

/*
 * Iterate through the database and count the occurances of each node label.
 * Create a hash table (map) with the key as each label, and th4 value as the
 * number of times the label occurs.  Return a list of labels that are mode
 * than the nsupport parameter.
 */
GList *find_frequent_node_labels(GArray *database, int nsupport, GHashTable *map)
{
	int i, j, *key, *value;
	GHashTableIter iter;
	GList *ret = NULL;
	GList *l = NULL;

	/* 
	 * Iterate over all graphs in the database, and determine how
 	 * in how many graphs a given not label appears. 
 	 */
	for (i = 0; i < database->len; i++) {
		struct graph *g;
		uint32_t *iter1;
		int *labels;
		uint32_t length;

		GList *gset = NULL;

		g = (struct graph *)g_array_index(database, struct graph *, i);

		/* Make a list of every unique label in the graph */
		for (j = 0; j < g_list_length(g->nodes); j++) {
			int32_t label = graph_get_node(g, j)->label;

			if (g_list_find(gset, GINT_TO_POINTER(label)))
				continue;
			gset = g_list_append(gset, GINT_TO_POINTER(label));
		}

		/* Add one to the label map if it occurred in the graph */
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
			g_hash_table_insert(map, GINT_TO_POINTER(label), 
							GINT_TO_POINTER(val));
		}
		g_list_free(gset);
	}

	/* Create a list of the labels that are above the nsupport value */
	g_hash_table_iter_init(&iter, map);
	while (g_hash_table_iter_next(&iter, (void **)&key, (void **)&value)) {
		if (GPOINTER_TO_INT(value) < nsupport)
			continue;
		ret = g_list_append(ret, key);
	}

	return ret;
}

/* 
 * Builds a right-most path of the subgraph represented by the DFS codes.
 * Returns an ordered list of edge(?) ids.
 */
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

/*
 * Return how many times the subgraph shows up in the DB? 
 */
static int count_support(GQueue *pdfs)
{
	GList *l;
	int size = 0;
	int prev_id = -1;
	
	for (l = g_list_first(pdfs->head); l; l = g_list_next(l)) {
		struct pre_dfs *p = (struct pre_dfs *)l->data;

		if (prev_id != p->id) {
			prev_id = p->id;
			size += 1;
		}
	}
	return size;
}

/*
 * Print the subgraph
 */
static void show_subgraph(GList *dfs_codes, int nsupport)
{
	struct graph *g;
	
	g = build_graph_dfs(dfs_codes);
	print_graph(g, nsupport);
	graph_free(g);

	return;
}

/* 
 * Main recursive mining function, Subproceedure 1 in the paper
 */
void mine_subgraph(struct gspan *gs, GQueue *projection)
{
	int support;
	GList *right_most_path, *keys, *l1, *l2;
	GQueue *values = NULL;
	int min_label;
	GHashTable *pm_forwards, *pm_backwards;

	//printf("Entering mine_subgraph\n");

	/* 
         * Exit condition, stop mining if subgraph not in enough graphs in the
         * DB. Appers to map to lines 11 and 12 in algorihm 1
         */
	support = count_support(projection);
	if (support < gs->nsupport) {
		//printf("here1\n");
		//printf("Exitint 1\n");
		return;
	}

	/* 
         * Determing if this projection is a minimum subgraph, return if not.
         * Line 6 of subproceedure 1.
         */
	if (!is_min(gs)) {
		//printf("Exiting 2\n");
		//printf("here2\n");
		return;
	}

	/* This is a minimum sunbgraph, so print it. Line 3 in subproc1 */
	show_subgraph(gs->dfs_codes, support);

	/* 
	 * Try to expand the subgraph and count its children, 
	 * Line 4 in subproc1. 
	 */
	right_most_path = build_right_most_path(gs->dfs_codes);
	min_label = ((struct dfs_code *)g_list_first(gs->dfs_codes)->data)->from_label;

	pm_forwards = g_hash_table_new(dfs_code_hash, glib_dfs_code_equal);
	pm_backwards = g_hash_table_new(dfs_code_hash, glib_dfs_code_equal);

	/* find all extensions of the subgraph */
	enumerate(gs, projection, right_most_path, pm_backwards, pm_forwards, 
								min_label);

	keys = g_hash_table_get_keys(pm_backwards);
	keys = g_list_sort(keys, (GCompareFunc)dfs_code_backward_compare);
	//printf("BACKWARD ==== ");
	//for (l1 = g_list_first(keys); l1; l1 = g_list_next(l1)) {
	//	struct dfs_code *dfsc = (struct dfs_code *)l1->data;
	//	print_dfs(dfsc);
	//}
	//printf("\n");

	/* This and the next for loop are line 5 in subproc 1 */
	for (l1 = g_list_first(keys); l1; l1 = g_list_next(l1)) {
		struct dfs_code *dfsc = (struct dfs_code *)l1->data;
#ifdef DEBUG
		printf("backward  ");
		print_dfs(dfsc);
		printf("\n");
#endif
		values = g_hash_table_lookup(pm_backwards, dfsc);

		gs->dfs_codes = g_list_append(gs->dfs_codes, dfsc);
		/* Extend and then recurse, lines 7 and 8 in subproc 1 */
		mine_subgraph(gs, values);

		l2 = g_list_last(gs->dfs_codes);
		gs->dfs_codes = g_list_remove_link(gs->dfs_codes, l2);
		//free((struct dfs_code *)l2->data);
		g_list_free(l2);
		//g_list_free(values);
		values = NULL;
	}

	g_list_free(keys);

	keys = g_hash_table_get_keys(pm_forwards);
	keys = g_list_sort(keys, (GCompareFunc)dfs_code_forward_compare);

	//printf("FORWARD ==== ");
	//for (l1 = g_list_last(keys); l1; l1 = g_list_previous(l1)) {
	//	struct dfs_code *dfsc = (struct dfs_code *)l1->data;
	//	print_dfs(dfsc);
	//}
	//printf("\n");

	/* Again, line 5. Note we iterate in reverse order */
	for (l1 = g_list_last(keys); l1; l1 = g_list_previous(l1)) {
		struct dfs_code *dfsc = (struct dfs_code *)l1->data;
#ifdef DEBUG
		printf("forward  ");
		print_dfs(dfsc);
		printf("\n");
#endif
		values = g_hash_table_lookup(pm_forwards, dfsc);

		/* Line 7: subproc 1 */
		gs->dfs_codes = g_list_append(gs->dfs_codes, dfsc);
		/* Line 8, subproc 1 */
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
	
	//printf("Exiting 3\n");


	return;
}

/* 
 * Main entry point for subgeaph mining. Algorithm 1 in the paper 
 */
int project(struct gspan *gs, GList *frequent_nodes, GHashTable *freq_labels)
{
	GHashTable *projection_map;
	GList *l1, *l2, *l3, *edges, *keys;
	GQueue *values;
	struct pre_dfs *pm;
	struct dfs_code *first_key, *start_code;
	int ret;
	int i;

	if (gs->dfs_codes != NULL)
		g_list_free_full(gs->dfs_codes, (GDestroyNotify)free);
	gs->dfs_codes = NULL;

	/* 
	 * Print all 1-node graphs that are frequent graphs. Frequent node 
	 * labels were previously determined and passed into this function.
	 * Line 1 was done in find_frequent_node_labels(), 
         * Lines 2-3 were done by read_graphs with the frequent label list.
	 */
	for (l1 = g_list_first(frequent_nodes); l1; l1 = g_list_next(l1)) {
		int nodelabel = GPOINTER_TO_INT(l1->data);
		int nodesup = GPOINTER_TO_INT(
				g_hash_table_lookup(freq_labels, 
					GINT_TO_POINTER(nodelabel)));

		print_graph_node(nodelabel, nodesup);
	}

	/* Find all frequent one-edges in the database. Line 4 */
	projection_map = g_hash_table_new(dfs_code_hash, glib_dfs_code_equal);

	//for(l1 = g_list_first(gs->database); l1; l1 = g_list_next(l1)) {
	//	struct graph *g = (struct graph *)l1->data;
	for (i=0; i < gs->database->len; i++) {
		struct graph *g = g_array_index(gs->database, struct graph *, i);
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
				} else
					values = g_queue_new();
				g_queue_push_tail(values, npdfs);
				g_hash_table_insert(projection_map, dfsc, 
									values);
			}

			g_list_free(edges);
			edges = NULL;
		}
	}

	/* Sort the keys in dfs code order, line 5 in algorithm 1 */
	keys = g_hash_table_get_keys(projection_map);
	keys = g_list_sort(keys, (GCompareFunc)dfs_code_project_compare);

	/* for each node in lexical order, line 7 in algorithm */
	for (l1 = g_list_last(keys); l1; l1 = g_list_previous(l1)) {
		struct dfs_code *dfsc = (struct dfs_code *)l1->data;
		struct dfs_code *start_code; 

		values = g_hash_table_lookup(projection_map, dfsc);

		/* Lines 11-12 in the algorithm. Exit condition */
		if (g_queue_get_length(values) < gs->nsupport) {
			//g_list_free(values);
			continue;
		}

#ifdef DEBUG
		print_dfs(dfsc);
		printf(" %d\n", g_list_length(values));
#endif

		/* Line 8 in algorithm, starts with the edge */
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

		/* 
		 * Line 9 in algorithm, begin the recursive search extensions to
		 * the edge
		 */	
		mine_subgraph(gs, values);

		/* Line 10, pop the edge from the stack */
		l2 = g_list_last(gs->dfs_codes);
		gs->dfs_codes = g_list_remove_link(gs->dfs_codes, l2);
		//free((struct dfs_code *)l2->data);
		g_list_free(l2);
	}

	cleanup_map(projection_map);

	return 0;
}

