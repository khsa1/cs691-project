#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>

#include <glib.h>

#include <graph.h>

struct edge *edge_new(uint32_t id, int32_t label, uint32_t to, uint32_t from)
{
	struct edge *ret;

	ret = malloc(sizeof(struct edge));
	if (!ret) {
		perror("Allocating new edge");
		return NULL;
	}

	ret->id = id;
	ret->label = label;
	ret->to = to;
	ret->from = from;

	return ret;
}

void edge_free(struct edge *e)
{
	if (e)
		free(e);
}

struct node *node_new(uint32_t id, int32_t label, GList *edges)
{
	struct node *ret;

	ret = malloc(sizeof(struct node));
	if (!ret) {
		perror("Allocating new node");
		return NULL;
	}

	ret->id = id;
	ret->label = label;
	if (edges)
		ret->edges = edges;
	else
		ret->edges = NULL;

	return ret;
}

void node_free(struct node *n)
{
	if (n->edges)
		g_list_free(n->edges);
	free(n);
}

void node_free_cb(void *n)
{
	node_free((struct node *)n);
}
		
struct graph *graph_new(uint32_t id, uint32_t nedges, GList *nodes)
{
	struct graph *ret;

	ret = malloc(sizeof(struct node));
	if (!ret) {
		perror("Allocating new node");
		return NULL;
	}

	ret->nedges = nedges;
	ret->id = id;
	if (nodes)
		ret->nodes = nodes;
	else
		ret->nodes = NULL;

	return ret;
}

void graph_free(struct graph *g)
{
	if (g->nodes)
		g_list_free_full(g->nodes, (GDestroyNotify)node_free_cb);
	free(g);
}

void graph_free_cb(void *g)
{
	graph_free((struct graph *)g);
}

uint32_t graph_get_size(struct graph *g)
{
	return g_list_length(g->nodes);
}

struct node *graph_get_node(struct graph *g, uint32_t index)
{
	return (struct node *)g_list_nth_data(g->nodes, index);
}

void graph_set_nodes(struct graph *g, GList *nodes)
{
	g->nodes = nodes;
	return;
}

void graph_clear(struct graph *g)
{
	int i;

	g->id = 0;
	g->nedges = 0;

	if (g->nodes) {
		g_list_free_full(g->nodes, (GDestroyNotify)node_free_cb);
		g->nodes = NULL;
	}
		
	return;
}


