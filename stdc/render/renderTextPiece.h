#ifndef __RENDERTEXTPIECE_H__
#define __RENDERTEXTPIECE_H__

#include "../inc/config.h"
#include "renderNode.h"

#ifdef __cplusplus
extern "C" {
#endif

enum NETextPieceType {
    NETPIE_TEXT,
    NETPIE_HOLDER_BEGIN,
    NETPIE_HOLDER_END
};

typedef struct _NRenderTextPiece {

    NRenderNode     d;

    wchr*           text;
    int16           len;

    int16           type;

} NRenderTextPiece;

NRenderTextPiece* renderTextPiece_create(void);
void renderTextPiece_delete(NRenderTextPiece** piece);

void renderTextPiece_layout(NLayoutStat* stat, NRenderNode* rn, NStyle* style, nbool force);
void renderTextPiece_paint(NRenderNode* rn, NStyle* style, NRect* rect);

#ifdef __cplusplus
}
#endif

#endif
