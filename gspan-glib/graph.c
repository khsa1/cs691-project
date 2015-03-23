#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>

#include <glib.h>

#include <graph.h>
#include <gspan.h>

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

	_graph_count ++;
	return;
}

void print_graph_node(int nodelabel, int support)
{
	printf("t # %d * %d\n", _graph_count, support);
	printf("v 0 %d\n", nodelabel);
	printf("\n");
	_graph_count ++;

	return;
}

struct graph *build_graph_dfs(GList *dfs_codes)
{
	struct graph *ret;
	GList *l;
	int numnodes = 0;
	int i;

	ret = graph_new(0,0,NULL);

	for (l = g_list_first(dfs_codes); l; l = g_list_next(l)) {
		struct dfs_code *dfsc = (struct dfs_code *)l->data;

		if (dfsc->from > numnodes)
			numnodes = dfsc->from;

		if (dfsc->to > numnodes)
			numnodes = dfsc->to;
	}

	for (i = 0; i < (numnodes + 1); i++)
		ret->nodes = g_list_append(ret->nodes, node_new(0,0,NULL));
	
	for (i = 0, l = g_list_first(dfs_codes); l; i++,l = g_list_next(l)) {
		struct dfs_code *dfsc = (struct dfs_code *)l->data;
		struct edge *e1, *e2;
		struct node *from_node, *to_node;

		from_node = graph_get_node(ret, dfsc->from);
		to_node = graph_get_node(ret, dfsc->to);

		from_node->id = dfsc->from;
		from_node->label = dfsc->from_label;
		to_node->id = dfsc->to;
		to_node->label = dfsc->to_label;

		e1 = malloc(sizeof(struct edge));
		if (!e1) {
			perror("allocating e1 edge in build_fraph_dfs");
			graph_free(ret);
			return NULL; 
		}
		e1->id = ret->nedges;
		e1->from = dfsc->from;
		e1->to = dfsc->to;
		e1->label = dfsc->edge_label;
		from_node->edges = g_list_append(from_node->edges, e1);

		e2 = malloc(sizeof(struct edge));
		if (!e2) {
			perror("allocating e2 edge in build_fraph_dfs");
			graph_free(ret);
			return NULL; 
		}
		e2->id = e1->id;
		e2->label = e1->label;
		e2->from = dfsc->to;
		e2->to = dfsc->from;
		to_node->edges = g_list_append(to_node->edges, e2);

		ret->nedges ++;
	}

	return ret;
}

