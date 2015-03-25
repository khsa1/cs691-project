#include <stdint.h>
#include <float.h>

#include <glib.h>

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


GList *find_frequent_node_labels(GList *database, int nsupport, GHashTable *map);
int prune_infrequent_nodes(GList *database, GList *frequent_labels);
int project(struct gspan *gs, GList *frequent_nodes, GHashTable *freq_labels);



