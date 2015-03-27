#include <stdint.h>
#include <float.h>

#include <glib.h>
#include <glib_compat.h>

#include <graph.h>

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

static inline void print_dfs(struct dfs_code *dfsc)
{
	printf("(from=%d, to=%d, from_label=%d, edge_label=%d, to_label=%d)", 
			dfsc->from, dfsc->to, dfsc->from_label, 
			dfsc->edge_label, dfsc->to_label);
}

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

static inline void print_pre_dfs(struct pre_dfs *pdfs)
{
	printf("(id=%d, edge=(%d,%d,%d), prev=%p)", pdfs->id, pdfs->edge->from, 
		pdfs->edge->to, pdfs->edge->label, pdfs->prev);
}

static inline void print_edge(struct edge *e)
{
	printf("(id=%d, from=%d, to=%d, labal=%d)",e->id, e->from, e->to, e->label);
}


static inline unsigned int dfs_code_hash(const void *key)
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

static inline gboolean glib_dfs_code_equal(const void *a, const void *b)
{
	if (dfs_code_equal((const struct dfs_code *)a, 
				(const struct dfs_code *)b))
		return TRUE;
	return FALSE;
}

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

