/*
 * Primative data structures that make up a graph.
 */
#include <stdint.h>

#include <glib.h>

#ifndef __GRAPH_H__
#define __GRAPH_H__

struct edge {
	uint32_t from;
	int32_t label;
	uint32_t to;
	uint32_t id;
};

struct node {
	uint32_t id;
	int32_t label;
	GList *edges;
};

struct graph {
	uint32_t nedges;
	uint32_t id;
	GList *nodes;
};

/* graph.c */
struct edge *edge_new(uint32_t id, int32_t label, uint32_t to, uint32_t from);
struct node *node_new(uint32_t id, int32_t label, GList *edges);
struct graph *graph_new(uint32_t id, uint32_t nedges, GList *nodes);
uint32_t graph_get_size(struct graph *g);
struct node *graph_get_node(struct graph *g, uint32_t index);
void graph_set_nodes(struct graph *g, GList *nodes);
void graph_clear(struct graph *g);
void graph_free(struct graph *g);
void graph_free_cb(void *g);
struct graph *build_graph_dfs(GList *dfs_codes);

/* fileio.c */
GList *read_graphs(const char *filename, GList *frequent);

#endif /* __GRAPH_H__ */
