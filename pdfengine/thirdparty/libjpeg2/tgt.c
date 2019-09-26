

#include "opj_includes.h"

opj_tgt_tree_t *tgt_create(int numleafsh, int numleafsv) {
	int nplh[32];
	int nplv[32];
	opj_tgt_node_t *node = NULL;
	opj_tgt_node_t *parentnode = NULL;
	opj_tgt_node_t *parentnode0 = NULL;
	opj_tgt_tree_t *tree = NULL;
	int i, j, k;
	int numlvls;
	int n;

	tree = (opj_tgt_tree_t *) opj_malloc(sizeof(opj_tgt_tree_t));
	if(!tree) return NULL;
	tree->numleafsh = numleafsh;
	tree->numleafsv = numleafsv;

	numlvls = 0;
	nplh[0] = numleafsh;
	nplv[0] = numleafsv;
	tree->numnodes = 0;
	do {
		n = nplh[numlvls] * nplv[numlvls];
		nplh[numlvls + 1] = (nplh[numlvls] + 1) / 2;
		nplv[numlvls + 1] = (nplv[numlvls] + 1) / 2;
		tree->numnodes += n;
		++numlvls;
	} while (n > 1);
	
	
	if (tree->numnodes == 0) {
		opj_free(tree);
		return NULL;
	}

	tree->nodes = (opj_tgt_node_t*) opj_calloc(tree->numnodes, sizeof(opj_tgt_node_t));
	if(!tree->nodes) {
		opj_free(tree);
		return NULL;
	}

	node = tree->nodes;
	parentnode = &tree->nodes[tree->numleafsh * tree->numleafsv];
	parentnode0 = parentnode;
	
	for (i = 0; i < numlvls - 1; ++i) {
		for (j = 0; j < nplv[i]; ++j) {
			k = nplh[i];
			while (--k >= 0) {
				node->parent = parentnode;
				++node;
				if (--k >= 0) {
					node->parent = parentnode;
					++node;
				}
				++parentnode;
			}
			if ((j & 1) || j == nplv[i] - 1) {
				parentnode0 = parentnode;
			} else {
				parentnode = parentnode0;
				parentnode0 += nplh[i];
			}
		}
	}
	node->parent = 0;
	
	tgt_reset(tree);
	
	return tree;
}

void tgt_destroy(opj_tgt_tree_t *tree) {
	opj_free(tree->nodes);
	opj_free(tree);
}

void tgt_reset(opj_tgt_tree_t *tree) {
	int i;

	if (NULL == tree)
		return;
	
	for (i = 0; i < tree->numnodes; i++) {
		tree->nodes[i].value = 999;
		tree->nodes[i].low = 0;
		tree->nodes[i].known = 0;
	}
}

void tgt_setvalue(opj_tgt_tree_t *tree, int leafno, int value) {
	opj_tgt_node_t *node;
	node = &tree->nodes[leafno];
	while (node && node->value > value) {
		node->value = value;
		node = node->parent;
	}
}

void tgt_encode(opj_bio_t *bio, opj_tgt_tree_t *tree, int leafno, int threshold) {
	opj_tgt_node_t *stk[31];
	opj_tgt_node_t **stkptr;
	opj_tgt_node_t *node;
	int low;

	stkptr = stk;
	node = &tree->nodes[leafno];
	while (node->parent) {
		*stkptr++ = node;
		node = node->parent;
	}
	
	low = 0;
	for (;;) {
		if (low > node->low) {
			node->low = low;
		} else {
			low = node->low;
		}
		
		while (low < threshold) {
			if (low >= node->value) {
				if (!node->known) {
					bio_write(bio, 1, 1);
					node->known = 1;
				}
				break;
			}
			bio_write(bio, 0, 1);
			++low;
		}
		
		node->low = low;
		if (stkptr == stk)
			break;
		node = *--stkptr;
	}
}

int tgt_decode(opj_bio_t *bio, opj_tgt_tree_t *tree, int leafno, int threshold) {
	opj_tgt_node_t *stk[31];
	opj_tgt_node_t **stkptr;
	opj_tgt_node_t *node;
	int low;

	stkptr = stk;
	node = &tree->nodes[leafno];
	while (node->parent) {
		*stkptr++ = node;
		node = node->parent;
	}
	
	low = 0;
	for (;;) {
		if (low > node->low) {
			node->low = low;
		} else {
			low = node->low;
		}
		while (low < threshold && low < node->value) {
			if (bio_read(bio, 1)) {
				node->value = low;
			} else {
				++low;
			}
		}
		node->low = low;
		if (stkptr == stk) {
			break;
		}
		node = *--stkptr;
	}
	
	return (node->value < threshold) ? 1 : 0;
}
