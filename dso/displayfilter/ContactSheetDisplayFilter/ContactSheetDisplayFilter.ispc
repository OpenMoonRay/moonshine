// Copyright 2026-2027 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

#include <moonray/rendering/displayfilter/DisplayFilter.isph>
#include <moonray/rendering/displayfilter/DisplayFilterMacros.isph>

struct ContactSheetDisplayFilter
{
    bool  mMask;
    bool  mInvertMask;
    float mMix;
    int   mGridSize;
    int   mNumInputs;
};

export const uniform ContactSheetDisplayFilter * uniform
ContactSheetDisplayFilter_get(const uniform DisplayFilter * uniform displayFilter)
{
    return DISPLAYFILTER_GET_ISPC_CPTR(ContactSheetDisplayFilter, displayFilter);
}


static void
filter(const uniform DisplayFilter * uniform me,
       const uniform InputBuffer * const uniform * const uniform inputBuffers,
       const varying DisplayFilterState * const uniform state,
       varying Color * uniform result)
{
    const uniform ContactSheetDisplayFilter * uniform self = ContactSheetDisplayFilter_get(me);

    const uniform int gridSize = max(1, self->mGridSize);

    const varying int dstX = state->mOutputPixelX;
    const varying int dstY = state->mOutputPixelY;

    const varying int cellWidth =  state->mImageWidth / self->mGridSize;
    const varying int cellHeight = state->mImageHeight / self->mGridSize;

    const varying int row = self->mGridSize - dstY / cellHeight - 1 ;
    const varying int column = dstX / cellWidth;

    const varying int bufferIndex = row * self->mGridSize + column;

    const varying int localX = dstX % cellWidth;
    const varying int localY = dstY % cellHeight;

    const varying int srcX0 = localX * gridSize;
    const varying int srcY0 = localY * gridSize;

    foreach_unique(idx in bufferIndex)
    {
        if (idx < 0 || idx >= self->mNumInputs || inputBuffers[idx] == NULL)
        {
            *result = Color_ctor(0.0f, 0.0f, 0.0f);
        }
        else
        {
            const uniform InputBuffer const * uniform src = inputBuffers[idx];
            varying Color accumulate = Color_ctor(0.0f, 0.0f, 0.0f);

            for (uniform int dy = 0; dy < gridSize; ++dy)
            {
                for (uniform int dx = 0; dx < gridSize; ++dx)
                {
                    accumulate = accumulate + InputBuffer_getPixel(src, srcX0 + dx, srcY0 + dy);
                }
            }

            const uniform float invArea = 1.0f / (float)(gridSize * gridSize);
            *result = accumulate * invArea;
        }
    }
}

DEFINE_DISPLAY_FILTER(ContactSheetDisplayFilter, filter)

//---------------------------------------------------------------------------
