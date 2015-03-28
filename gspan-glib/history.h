#include <glib.h>

struct history {
	GList *edges;
	GList *has_edges;
	GList *has_nodes;
	//GHashTable *has_edges;
	//GHashTable *has_nodes;
};

struct history *alloc_history(void);
void free_history(struct history *h);
int build_history(struct history *h, struct pre_dfs *pdfs);
