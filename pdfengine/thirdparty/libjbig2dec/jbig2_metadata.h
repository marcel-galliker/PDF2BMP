

#ifndef _JBIG2_METADATA_H
#define _JBIG2_METADATA_H

typedef enum {
    JBIG2_ENCODING_ASCII,
    JBIG2_ENCODING_UCS16
} Jbig2Encoding;

typedef struct _Jbig2Metadata Jbig2Metadata;

Jbig2Metadata *jbig2_metadata_new(Jbig2Ctx *ctx, Jbig2Encoding encoding);
void jbig2_metadata_free(Jbig2Ctx *ctx, Jbig2Metadata *md);
int jbig2_metadata_add(Jbig2Ctx *ctx, Jbig2Metadata *md,
                        const char *key, const int key_length,
                        const char *value, const int value_length);

struct _Jbig2Metadata {
    Jbig2Encoding encoding;
    char **keys, **values;
    int entries, max_entries;
};

int jbig2_comment_ascii(Jbig2Ctx *ctx, Jbig2Segment *segment,
                                const uint8_t *segment_data);
int jbig2_comment_unicode(Jbig2Ctx *ctx, Jbig2Segment *segment,
                               const uint8_t *segment_data);

#endif 
