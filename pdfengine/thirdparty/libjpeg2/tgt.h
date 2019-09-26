

#ifndef __TGT_H
#define __TGT_H

typedef struct opj_tgt_node {
  struct opj_tgt_node *parent;
  int value;
  int low;
  int known;
} opj_tgt_node_t;

typedef struct opj_tgt_tree {
  int numleafsh;
  int numleafsv;
  int numnodes;
  opj_tgt_node_t *nodes;
} opj_tgt_tree_t;

opj_tgt_tree_t *tgt_create(int numleafsh, int numleafsv);

void tgt_destroy(opj_tgt_tree_t *tree);

void tgt_reset(opj_tgt_tree_t *tree);

void tgt_setvalue(opj_tgt_tree_t *tree, int leafno, int value);

void tgt_encode(opj_bio_t *bio, opj_tgt_tree_t *tree, int leafno, int threshold);

int tgt_decode(opj_bio_t *bio, opj_tgt_tree_t *tree, int leafno, int threshold);

#endif 
