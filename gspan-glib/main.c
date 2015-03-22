#include <stdio.h>
#include <float.h>
#include <stdlib.h>
#include <string.h>

#include <graph.h>
#include <gspan.h>

void clean_database(GList *database)
{
	g_list_free_full(database, (GDestroyNotify)graph_free_cb);
}

int main(int argc, char **argv)
{
	struct gspan gs;
	GList *database = NULL;
	GList *frequent = NULL;

	printf("Reading graph database... ");
	database = read_graphs(argv[1], NULL);
	gs.support = atof(argv[2]);
	gs.nsupport = (int)(gs.support*(double)g_list_length(database));

	printf("Support level %f : %d/%d\n", gs.support, gs.nsupport, 
						g_list_length(database));
	printf("Finding frequent labels... ");
	frequent = find_frequent_node_labels(database, gs.nsupport);
	printf("%d labels\n", g_list_length(frequent));

        print_graph((struct graph *)g_list_first(database)->data, -1);

	clean_database(database);

	printf("Pruning infrequent nodes... ");
	gs.database = read_graphs(argv[1], frequent);
	printf("done.\n");
	printf("Database contains %d graphs\n", g_list_length(database));

	print_graph((struct graph *)g_list_first(gs.database)->data, -1);

	clean_database(gs.database);
	return 0;
}
