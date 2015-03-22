#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>

#include <glib.h>

#include <graph.h>

static int _graph_count = 0;

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

void edge_free_cb(void *e)
{
	edge_free((struct edge *)e);
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
		g_list_free_full(n->edges, (GDestroyNotify)edge_free_cb);
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

void print_graph(struct graph *g, int support)
{
	GList *l1, *l2, *l3;
	GList *edges = NULL;

	if (support > 0)
		printf("t # %d * %d\n", _graph_count, support);
	else
		printf("t # %d\n", _graph_count);

	for (l1 = g_list_first(g->nodes); l1; l1 = g_list_next(l1)) {
		struct node *n = (struct node *)l1->data;
		printf("v %d %d\n", n->id, n->label);

		for (l2 = g_list_first(n->edges); l2; l2 = g_list_next(l2)) {
			struct edge *e = (struct edge *)l2->data;
			int found = 0;
			
			for (l3 = g_list_first(edges); l3; 
						l3 = g_list_next(l3)) {
				struct edge *e2 = (struct edge *)l3->data;

				if (e->id == e2->id) {
					found = 1;
					break;
				}	
			}

			if (!found)
				edges = g_list_prepend(edges, e);
		}
	}
	edges = g_list_reverse(edges);

	for (l1 = g_list_first(edges); l1; l1 = g_list_next(l1)) {
		struct edge *e = (struct edge *)l1->data;

		printf("e %d %d %d\n", e->from, e->to, e->label);
	}

	g_list_free(edges);
	printf("\n");

	return;
}
