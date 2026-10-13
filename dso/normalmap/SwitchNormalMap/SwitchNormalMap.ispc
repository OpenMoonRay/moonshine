// Copyright 2023-2024 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

/// @file SwitchNormalMap.ispc

#include "attributes.isph"

#include <moonray/rendering/shading/ispc/MapApi.isph>

enum SwitchNormalConstants {
    SN_MAX_INPUTS = 64,
};
ISPC_UTIL_EXPORT_ENUM_TO_HEADER(SwitchNormalConstants);

struct SwitchNormalMap
{
    uniform intptr_t mNormalMap[64];
    uniform intptr_t mSampleNormalFn[64];
};
ISPC_UTIL_EXPORT_UNIFORM_STRUCT_TO_HEADER(SwitchNormalMap);

static varying Vec3f
sampleNormal(const uniform NormalMap *uniform map,
             uniform ShadingTLState *uniform tls,
             const varying State &state)
{
    const uniform SwitchNormalMap * uniform me = NORMALMAP_GET_ISPC_PTR(SwitchNormalMap, map);
    varying Vec3f norm = state.mN;

    varying int choice = max(0, (varying int) round(evalAttrChoice(map, tls, state)));
    choice = choice % SN_MAX_INPUTS;

    foreach_unique(val in choice) {
        const uniform NormalMap* uniform nm =
            (const uniform NormalMap* uniform) (me->mNormalMap[val]);

        if (nm) {
            SampleNormalPtr sampleNormalFn = (SampleNormalPtr) me->mSampleNormalFn[val];
            if (sampleNormalFn != NULL) {
                sampleNormalFn(nm, tls, &state, &norm);
            }
        }
    }

    return norm;
}

DEFINE_NORMALMAP_SHADER(SwitchNormalMap, sampleNormal)

