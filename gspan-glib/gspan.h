/* 
 * gspan.h: Core structs and inline functions for the gspan algorithm
 *
 * Author: John Clemens <clemej1 at umbc.edu>
 * Copyrigt (c) 2015
 */


#include <stdint.h>
#include <float.h>

#include <glib.h>
#include <glib_compat.h>

#include <graph.h>

#ifndef __GSPAN_H__
#define __GSPAN_H__

struct dfs_code {
	uint8_t from;
	uint8_t to;
	int8_t from_label;
	int8_t edge_label;
	int8_t to_label;
};

struct pre_dfs {
	uint32_t id;
	struct edge *edge;
	struct pre_dfs *prev;
};

struct gspan {
	GList *database;
	double support;
	uint32_t nsupport;
	GList *subgraphs;
	GList *dfs_codes;
	GList *min_dfs_codes;
	struct graph *min_graph;
};

static inline int dfs_code_equal(const struct dfs_code *a, 
						const struct dfs_code *b)
{
	return (a->from == b->from) && (a->to == b->to) && 
				(a->from_label == b->from_label) && 
				(a->edge_label == b->edge_label) && 
				(a->to_label == b->to_label);
}

/* 
 * Comparison functions that order struct dfs_codes when sorting the
 * keys.  This ordering is VERY IMPORTANT for the algorithms. 
 * Return -1, 0, or 1 if a is less than, equal to, or greater than b
 */
static inline int dfs_code_project_compare(const struct dfs_code *a, 
						const struct dfs_code *b) 
{
	if (a->from_label != b->from_label) {
		if (a->from_label < b->from_label)
			return -1;
		return 1;
	}

	if (a->edge_label != b->edge_label) {
		if (a->edge_label < b->edge_label)
			return -1;
		return 1;	
	}

	if (a->to_label != b->to_label) {
		if (a->to_label < b->to_label)
			return -1;
		return 1;
	}

	return 0;
}

static inline int dfs_code_backward_compare(const struct dfs_code *a, 
						const struct dfs_code *b) 
{
	if (a->to != b->to) {
		if (a->to < b->to)
			return -1;
		return 1;
	}
	
	if (a->edge_label != b->edge_label) {
		if (a->edge_label < b->edge_label)
			return -1;
		return 1;
	}

	return 0;
}

static inline int dfs_code_forward_compare(const struct dfs_code *a, 
						const struct dfs_code *b) 
{
	if (a->from != b->from) {
		if (a->from > b->from)
			return -1;
		return 1;
	}

	if (a->edge_label != b->edge_label) {
		if (a->edge_label < b->edge_label)
			return -1;
		return 1;
	}

	if (a->to_label != b->to_label) {
		if (a->to_label < b->to_label)
			return -1;
		return 1;
	}

	return 0;
}

/* Print out a struct dfs_code */
static inline void print_dfs(struct dfs_code *dfsc)
{
	printf("(from=%d, to=%d, from_label=%d, edge_label=%d, to_label=%d)", 
			dfsc->from, dfsc->to, dfsc->from_label, 
			dfsc->edge_label, dfsc->to_label);
}

/* Print out a list of dfs_codes, one per line */
static inline void print_dfs_list(GList *list)
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

/* Utility function to print an edge */
static inline void print_edge(struct edge *e)
{
	printf("(id=%d, from=%d, to=%d, labal=%d)",e->id, e->from, e->to, e->label);
}

static inline void print_id_list(GList *list)
{
	GList *l;
	printf(" --- [");
	for (l = g_list_first(list); l; l = g_list_next(l)) {
		int e = GPOINTER_TO_INT(l->data);

		printf("%d,",e);
	}
	printf("] --\n");
}


/* Utility function to print a pre_dfs struct */
static inline void print_pre_dfs(struct pre_dfs *pdfs)
{
	printf("(id=%d, edge=(%d,%d,%d), prev=%p)", pdfs->id, pdfs->edge->from, 
		pdfs->edge->to, pdfs->edge->label, pdfs->prev);
}


/* A simple hash table function to hash dfs codes */
static inline unsigned int dfs_code_hash(const void *key)
{
	struct dfs_code *dfsc = (struct dfs_code *)key;
	unsigned int ret;

	ret = 0 | (dfsc->from << 26) | (dfsc->to << 20) | 
		(dfsc->from_label << 14) | (dfsc->edge_label << 8) | dfsc->to_label;

	/* 
	 * don't have enough bits on 32-bit to include to_label directly
	 * This just means a little worse hash table performance.
	 */ 
	return ret;
}

/* GLib-formatted callback for comparing DFS codes */
static inline gboolean glib_dfs_code_equal(const void *a, const void *b)
{
	if (dfs_code_equal((const struct dfs_code *)a, 
				(const struct dfs_code *)b))
		return TRUE;
	return FALSE;
}

/* De-allocate a map */
static inline void cleanup_map(GHashTable *map)
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


GList *find_frequent_node_labels(GList *database, int nsupport, GHashTable *map);
int prune_infrequent_nodes(GList *database, GList *frequent_labels);
int project(struct gspan *gs, GList *frequent_nodes, GHashTable *freq_labels);
int is_min(struct gspan *gs);
GList *build_right_most_path(GList *dfs_codes);
GList *get_forward_init(struct node *n, struct graph *g);

#endif /* __GSPAN_H__ */
