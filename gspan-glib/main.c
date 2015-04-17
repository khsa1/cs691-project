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
	int id, np;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &id);
	MPI_Comm_size(MPI_COMM_WORLD, &np);

	struct gspan gs;
	GList *database = NULL;
	GList *frequent = NULL;
	GHashTable *map;

	memset(&gs, 0, sizeof(struct gspan));

	if(id==0) printf("Reading graph database... ");
	database = read_graphs(argv[1], NULL);
	gs.support = atof(argv[2]);
	gs.nsupport = (int)(gs.support*(double)g_list_length(database));

	if(id==0) printf("Support level %f : %d/%d\n", gs.support, gs.nsupport, 
						g_list_length(database));
	if(id==0) printf("Finding frequent labels... ");
	
	map = g_hash_table_new(g_direct_hash, g_direct_equal);

	frequent = find_frequent_node_labels(database, gs.nsupport, map);
	if(id==0) printf("%d labels\n", g_list_length(frequent));

        //print_graph((struct graph *)g_list_first(database)->data, -1);

	clean_database(database);

	if(id==0) printf("Pruning infrequent nodes... ");
	gs.database = read_graphs(argv[1], frequent);
	if(id==0) printf("done.\n");
	if(id==0) printf("Database contains %d graphs\n", g_list_length(gs.database));

	//print_graph((struct graph *)g_list_first(gs.database)->data, -1);
	frequent = g_list_sort(frequent, (GCompareFunc)compare_int);

	project(&gs, frequent, map,id,np);
	g_hash_table_destroy(map);
	clean_database(gs.database);

	MPI_Finalize();

	return 0;
}
