#include <stdio.h>
#include <errno.h>

#include <graph.h>

/* Return a GList of graphs */

GList *read_graphs(const char *filename, GList *frequent)
{
	char line[1024];
	FILE *fp;
	GList *ret = NULL;
	struct graph *g = NULL;
	struct node *n = NULL;
	struct edge *e = NULL;
	int count;
	GHashTable *id_map;
	int node_id;
	GList *labels = NULL;

	id_map = g_hash_table_new(g_direct_hash, NULL);

	if ((fp = fopen(filename, "r+")) == NULL) {
		perror("Opening DB file\n");
		return NULL;
	}

	count = 0;
	node_id = 0;
	

	while (fgets(line, 1023, fp) != NULL) {

		if (line[0] == 't') {
			if (count > 0) {
				ret = g_list_append(ret, g);
				g_hash_table_remove_all(id_map);
				g_list_free(labels);
				labels = NULL;
				node_id = 0;
			}
			g = graph_new(0,0,NULL);
			g->id = count;
			count ++;
			continue;
		}

		if (line[0] == 'v') {
			int nid, label;
			sscanf(line, "%*c %u %d", &nid, &label);
			labels = g_list_append(labels, &label);
			if (frequent) {
				if (!g_list_find(frequent, GINT_TO_POINTER(label)))
					continue;
			}
			n = node_new(0,0,NULL);
			n->id = node_id;
			n->label = label;
			g_hash_table_insert(id_map, &nid, &node_id);
			g->nodes = g_list_append(g->nodes, n);
			node_id ++;
			continue;
		}

		if (line[0] == 'e') {
			struct node *pn;
			uint32_t tmp;
			int from, to, label, label_from, label_to;

			sscanf(line, "%*c %u %u %d", &from, &to, &label);
			label_from = GPOINTER_TO_INT(
						g_list_nth_data(labels, from));
			label_to = GPOINTER_TO_INT(g_list_nth_data(labels, to));
			if (frequent) {
				if (!g_list_find(frequent, GINT_TO_POINTER(label_from)) ||
					!g_list_find(frequent, GINT_TO_POINTER(label_to))) 
					continue;
			}

			e = edge_new(0,0,0,0);
			e->id = g->nedges;
			g->nedges ++;
			e->from = GPOINTER_TO_INT(g_hash_table_lookup(id_map, &from));
			e->to = GPOINTER_TO_INT(g_hash_table_lookup(id_map, &to));
			e->label = label;

			pn = graph_get_node(g, e->from);
			pn->edges = g_list_append(pn->edges, e);

			tmp = e->to;
			e->to = e->from;
			e->from = e->to;
			pn = graph_get_node(g, e->from);
			pn->edges = g_list_append(pn->edges, e);

			g->nedges++;
		}
	}

	ret = g_list_append(ret, g);
	g_hash_table_remove_all(id_map);
	g_list_free(labels);

	
	fclose(fp);

	return ret;
}

