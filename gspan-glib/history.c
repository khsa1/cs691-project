#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <glib.h>

#include <graph.h>
#include <gspan.h>
#include <history.h>

struct history *alloc_history(void)
{
	struct history *ret;

	ret = malloc(sizeof(struct history));
	if (!ret) {
		perror("allocating history struct");
		return NULL;
	}
	ret->edges = NULL;
	ret->has_edges = NULL;//g_hash_table_new(g_direct_hash, g_direct_equal);
	ret->has_nodes = NULL;//g_hash_table_new(g_direct_hash, g_direct_equal);
	
	return ret;
}

void free_history(struct history *h)
{
	//g_hash_table_destroy(h->has_edges);
	//g_hash_table_destroy(h->has_nodes);
	g_list_free(h->has_edges);
	g_list_free(h->has_nodes);
	g_list_free(h->edges);
	free(h);
}

int build_history(struct history *h, struct pre_dfs *pdfs)
{
	struct pre_dfs *p = pdfs;

	while (p) {
		h->edges = g_list_append(h->edges, p->edge);

		if (!g_list_find(h->has_edges, GINT_TO_POINTER(p->edge->id)))
			h->has_edges = g_list_append(h->has_edges, 
						GINT_TO_POINTER(p->edge->id));

		if (!g_list_find(h->has_nodes, GINT_TO_POINTER(p->edge->from)))
			h->has_nodes = g_list_append(h->has_nodes, 
					GINT_TO_POINTER(p->edge->from));

		if (!g_list_find(h->has_nodes, GINT_TO_POINTER(p->edge->to)))
			h->has_nodes = g_list_append(h->has_nodes, 
					GINT_TO_POINTER(p->edge->to));



		//g_hash_table_add(h->has_edges, GINT_TO_POINTER(p->edge->id));
		//g_hash_table_add(h->has_nodes, GINT_TO_POINTER(p->edge->from));
		//g_hash_table_add(h->has_nodes, GINT_TO_POINTER(p->edge->to));
		//printf("Adding %d and %d\n", p->edge->from, p->edge->to);
		p = p->prev;
	}
	h->edges = g_list_reverse(h->edges);

	return 0;
}

