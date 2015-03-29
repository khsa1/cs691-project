#include <stdio.h>
#include <float.h>
#include <stdlib.h>
#include <string.h>

#include <graph.h>
#include <gspan.h>

void clean_database(GArray *database)
{
	int i;
	for(i=0; i<database->len; i++) {
		struct graph *g = (struct graph *)
				g_array_index(database,struct graph *, i);
		graph_free(g);
	}
	g_array_free(database, FALSE);
}

int compare_int(const void *a, const void *b)
{
	if (a > b)
		return 1;
	if (a < b)
		return -1;
	return 0;
}

int main(int argc, char **argv)
{
	struct gspan gs;
	GArray *database = NULL;
	GList *frequent = NULL;
	GHashTable *map;

	memset(&gs, 0, sizeof(struct gspan));

	printf("Reading graph database... ");
	database = read_graphs(argv[1], NULL);
	gs.support = atof(argv[2]);
	gs.nsupport = (int)(gs.support*(double)database->len);

	printf("Support level %f : %d/%d\n", gs.support, gs.nsupport, 
							database->len);
	printf("Finding frequent labels... ");
	
	map = g_hash_table_new(g_direct_hash, g_direct_equal);

	frequent = find_frequent_node_labels(database, gs.nsupport, map);
	printf("%d labels\n", g_list_length(frequent));

        //print_graph((struct graph *)g_list_first(database)->data, -1);

	clean_database(database);

	printf("Pruning infrequent nodes... ");
	gs.database = read_graphs(argv[1], frequent);
	printf("done.\n");
	printf("Database contains %d graphs\n", gs.database->len);

	//print_graph((struct graph *)g_list_first(gs.database)->data, -1);
	frequent = g_list_sort(frequent, (GCompareFunc)compare_int);

	project(&gs, frequent, map);

	g_hash_table_destroy(map);
	clean_database(gs.database);
	return 0;
}
