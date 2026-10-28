// Copyright 2023-2024 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

/// @file DirectionalMap.ispc

#include "attributes.isph"

#include <moonray/rendering/shading/ispc/MapApi.isph>
#include <moonshine/common/interpolation/ispc/Interpolation.isph>

struct DirectionalMapStaticData
{
    uniform int   sErrorMissingRefN;
};
ISPC_UTIL_EXPORT_UNIFORM_STRUCT_TO_HEADER(DirectionalMapStaticData);

struct DirectionalMap
{
    uniform const Xform * uniform mXform;
    uniform int mRefPKey;
    uniform int mRefNKey;

    const uniform NormalMap * uniform mNormalMap;
    uniform intptr_t mSampleNormalFunc;

    uniform DirectionalMapStaticData * uniform mStaticData;
};
ISPC_UTIL_EXPORT_UNIFORM_STRUCT_TO_HEADER(DirectionalMap);

enum PrimeSource {
    DIRECTIONAL_PRIME_SOURCE_OBSERVER = 0,
    DIRECTIONAL_PRIME_SOURCE_CUSTOM   = 1,
    DIRECTIONAL_PRIME_SOURCE_AXIS     = 2,
    DIRECTIONAL_PRIME_SOURCE_LOCATION = 3,
};
ISPC_UTIL_EXPORT_ENUM_TO_HEADER(PrimeSource);

enum ClampBehavior {
    DIRECTIONAL_BEHAVIOR_CLAMP = 0,
    DIRECTIONAL_BEHAVIOR_ABS   = 1,
    DIRECTIONAL_BEHAVIOR_UNCLAMPED = 2
};
ISPC_UTIL_EXPORT_ENUM_TO_HEADER(ClampBehavior);

enum FalloffType {
    DIRECTIONAL_FALLOFF_COSINE = 0,
    DIRECTIONAL_FALLOFF_LINEAR = 1
};
ISPC_UTIL_EXPORT_ENUM_TO_HEADER(FalloffType);

enum ObjectAxis {
    DIRECTIONAL_OBJECT_AXIS_POS_X = 0,
    DIRECTIONAL_OBJECT_AXIS_NEG_X = 1,
    DIRECTIONAL_OBJECT_AXIS_POS_Y = 2,
    DIRECTIONAL_OBJECT_AXIS_NEG_Y = 3,
    DIRECTIONAL_OBJECT_AXIS_POS_Z = 4,
    DIRECTIONAL_OBJECT_AXIS_NEG_Z = 5,
};
ISPC_UTIL_EXPORT_ENUM_TO_HEADER(ObjectAxis);

enum Polarity {
    DIRECTIONAL_POLARITY_PERPENDICULAR = 0,
    DIRECTIONAL_POLARITY_PARALLEL = 1
};
ISPC_UTIL_EXPORT_ENUM_TO_HEADER(Polarity);

Vec3f
computeObjectAxis(const uniform Map *uniform map,
                  const uniform int axis,
                  const varying State &state)
{
    const uniform DirectionalMap* uniform me = MAP_GET_ISPC_PTR(DirectionalMap, map);

    const uniform Vec3f axes[6] = {
        Vec3f_ctor( 1.0f,  0.0f,  0.0f),
        Vec3f_ctor(-1.0f,  0.0f,  0.0f),
        Vec3f_ctor( 0.0f,  1.0f,  0.0f),
        Vec3f_ctor( 0.0f, -1.0f,  0.0f),
        Vec3f_ctor( 0.0f,  0.0f,  1.0f),
        Vec3f_ctor( 0.0f,  0.0f, -1.0f)
    };

    Vec3f dir = transformVector(
                    me->mXform,
                    SHADING_SPACE_OBJECT,
                    SHADING_SPACE_RENDER,
                    state,
                    axes[axis]);

    return normalize(dir);
}

static Color
sample(const uniform Map *uniform map,
       uniform ShadingTLState * uniform tls,
       const varying State &state)
{
    const uniform DirectionalMap* uniform me = MAP_GET_ISPC_PTR(DirectionalMap, map);

    // determine our 'prime' direction
    varying Vec3f prime;
    varying Vec3f P = getP(state);
    const uniform int dirSource = getAttrPrimeDirection(map);

    switch (dirSource) {
        case DIRECTIONAL_PRIME_SOURCE_AXIS :
            prime = computeObjectAxis(map, getAttrObjectAxis(map), state);
            break;

        case DIRECTIONAL_PRIME_SOURCE_LOCATION :
            if (getAttrUseReferenceSpace(map)) {
                SHADER_DATA_getRefP(me->mRefPKey, tls, state, &P);
                prime = P; // refP is already in object space
            } else {
                // transform our shading point into the local space of our object
                prime = transformPoint(me->mXform,
                                       SHADING_SPACE_RENDER,
                                       SHADING_SPACE_OBJECT,
                                       state, P);
            }

            // treat our point as a vector and flip it so that it points
            // from the shading position (or reference position) to the
            // object, and transform that vector back into render space
            prime = transformVector(me->mXform,
                                    SHADING_SPACE_OBJECT,
                                    SHADING_SPACE_RENDER,
                                    state, neg(prime));

            if (lengthSqr(prime) > 0.0f) {
                prime = normalize(prime);
            }
            break;

        case DIRECTIONAL_PRIME_SOURCE_OBSERVER :
            prime = getWo(state);
            break;

        case DIRECTIONAL_PRIME_SOURCE_CUSTOM :
        default :
            prime = evalAttrCustomDirection(map, tls, state);
            prime = transformVector(me->mXform,
                                    SHADING_SPACE_WORLD,
                                    SHADING_SPACE_RENDER,
                                    state, prime);

            if (lengthSqr(prime) > 0.0f) {
                prime = normalize(prime);
            }
            break;
    } // end switch

    // get N
    varying Vec3f N;
    if (getAttrUseReferenceSpace(map)) {
        if (!SHADER_DATA_getRefN(me->mRefPKey, me->mRefNKey, tls, state, &N)) {
            // There really isn't a right thing to do here, its too late
            // to fatal() the shader.
            logEvent(map, me->mStaticData->sErrorMissingRefN);
        }

        // TODO: this ignores normal mapping.  Do we have a way to
        // compute the perturbed normal in reference space ?

        N = transformNormal(me->mXform,
                            SHADING_SPACE_OBJECT,
                            SHADING_SPACE_RENDER,
                            state, N);
    } else {
        SampleNormalPtr sampleNormalFn = (SampleNormalPtr) me->mSampleNormalFunc;
        if (sampleNormalFn != NULL) {
            const uniform float normalDial = getAttrInputNormalDial(map);
            evalNormal(map,
                       tls,
                       state,
                       me->mNormalMap,
                       sampleNormalFn,
                       normalDial,
                       &N); // undapted normal
        } else {
            N = getN(state);
        }
    }

    varying float cosTheta = dot(N, prime);
    switch (getAttrClamping(map)) {
        case DIRECTIONAL_BEHAVIOR_ABS :
            cosTheta = abs(cosTheta);
            break;
        case DIRECTIONAL_BEHAVIOR_UNCLAMPED :
            cosTheta = (cosTheta + 1.0f) * 0.5f;
            break;
        case DIRECTIONAL_BEHAVIOR_CLAMP :
        default :
            break;
    }
    cosTheta = saturate(cosTheta);

    if (getAttrPolarity(map) == DIRECTIONAL_POLARITY_PERPENDICULAR) {
        cosTheta = 1.0 - cosTheta;
    }

    const uniform int falloffType = getAttrFalloffType(map);
    varying float falloff;
    if (falloffType == DIRECTIONAL_FALLOFF_COSINE) {
        falloff = cosTheta;
    } else if (getAttrClamping(map) == DIRECTIONAL_BEHAVIOR_UNCLAMPED) {
        falloff = 1.0f - dw_acos(cosTheta * 2.0f - 1.0f) /sPi;
    } else {    // DIRECTIONAL_FALLOFF_LINEAR
        falloff = 1.0f - dw_acos(cosTheta) / sHalfPi;
    }

    const varying float biasVal = saturate(evalAttrBias(map, tls, state));
    falloff = bias(falloff, biasVal);

    if (getAttrUseSmoothstep(map)) {
        falloff = INTERPOLATION_smoothStep(falloff,
                                           evalAttrSmoothStart(map, tls, state),
                                           evalAttrSmoothEnd(map, tls, state));
    }

    const varying Color clrA = evalAttrColorA(map, tls, state);
    const varying Color clrB = evalAttrColorB(map, tls, state);
    const varying Color result = lerp(clrA, clrB, falloff);

    return result;
}
DEFINE_MAP_SHADER(DirectionalMap, sample)


