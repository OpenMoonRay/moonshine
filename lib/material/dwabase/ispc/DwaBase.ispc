// Copyright 2023-2024 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

/// @file DwaBase.ispc

#include <moonray/rendering/shading/ispc/Shading.isph>
#include <moonray/rendering/shading/ispc/Xform.isph>
#include <moonray/rendering/shading/ispc/Util.isph>
#include <moonshine/material/glitter/ispc/Glitter.isph>
#include "DwaBaseLayerable.isph"
#include "DwaBase.isph"

ISPC_UTIL_EXPORT_UNIFORM_STRUCT_TO_HEADER(DwaBaseAttributeFuncs);
ISPC_UTIL_EXPORT_UNIFORM_STRUCT_TO_HEADER(DwaBaseParameterHints);
ISPC_UTIL_EXPORT_UNIFORM_STRUCT_TO_HEADER(Model);
ISPC_UTIL_EXPORT_UNIFORM_STRUCT_TO_HEADER(DwaBase);

#define GETBOOLATTR(fnPtr, me, defaultValue)               fnPtr ? ((GetBoolAttrFnType)(fnPtr))(me) : defaultValue
#define GETINTATTR(fnPtr, me, defaultValue)                fnPtr ? ((GetIntAttrFnType)(fnPtr))(me) : defaultValue
#define GETFLOATATTR(fnPtr, me, tls, state, defaultValue)  fnPtr ? ((GetFloatAttrFnType)(fnPtr))(me, tls, state) : defaultValue
#define GETCOLORATTR(fnPtr, me, tls, state, defaultValue)  fnPtr ? ((GetColorAttrFnType)(fnPtr))(me, tls, state) : defaultValue
#define EVALFLOATATTR(fnPtr, me, tls, state, defaultValue) fnPtr ? ((EvalFloatAttrFnType)(fnPtr))(me, tls, state) : defaultValue
#define EVALCOLORATTR(fnPtr, me, tls, state, defaultValue) fnPtr ? ((EvalColorAttrFnType)(fnPtr))(me, tls, state) : defaultValue
#define EVALVEC2FATTR(fnPtr, me, tls, state, defaultValue) fnPtr ? ((EvalVec2fAttrFnType)(fnPtr))(me, tls, state) : defaultValue
#define EVALVEC3FATTR(fnPtr, me, tls, state, defaultValue) fnPtr ? ((EvalVec3fAttrFnType)(fnPtr))(me, tls, state) : defaultValue


/* Internal Helper Functions to Evaluate Attributes based on
 * isCaustics settings.
 */
/*  Helper Functions*/

inline void
resolveNormalParams(const uniform DwaBase * uniform dwaBase,
                    const uniform Material *uniform me,
                    uniform ShadingTLState * uniform tls,
                    const varying State &state,
                    varying DwaBaseParameters * uniform params)
{
    if (dwaBase->mNormalMap) {
        params->mNormalDial = EVALFLOATATTR(dwaBase->mAttrFuncs.mEvalAttrInputNormalDial, me, tls, state, 1.0f);
        if (dwaBase->mAttrFuncs.mGetAttrNormalAAStrategy) {
            params->mNormalAAStrategy = GETINTATTR(dwaBase->mAttrFuncs.mGetAttrNormalAAStrategy, me, 0);
            params->mNormalAADial = GETFLOATATTR(dwaBase->mAttrFuncs.mGetAttrNormalAADial, me, tls, state, 1.0);
        }
        if (params->mNormalAAStrategy == NORMAL_AA_STRATEGY_TOKSVIG) {
            // For Toksvig we need the length of the normal before
            // it's normalized therefore we pass in the length by
            // reference.
            Vec3f unadaptedN;
            params->mNormal = evalNormal(me,
                                         tls,
                                         state,
                                         dwaBase->mNormalMap,
                                         (SampleNormalPtr)dwaBase->mSampleNormalFunc,
                                         params->mNormalDial,
                                         &params->mNormalLength,
                                         &unadaptedN);
            params->mDiffuseNormal = unadaptedN;
        } else {
            Vec3f unadaptedN;
            params->mNormal = evalNormal(me,
                                         tls,
                                         state,
                                         dwaBase->mNormalMap,
                                         (SampleNormalPtr)dwaBase->mSampleNormalFunc,
                                         params->mNormalDial,
                                         &unadaptedN);
            params->mDiffuseNormal = unadaptedN;
        }

        params->mEvalSubsurfaceNormalFn = dwaBase->mAttrFuncs.mEvalSubsurfaceNormal;
    } else {
        params->mNormal = state.mN;
        params->mDiffuseNormal = state.mN;
    }
}

extern void
DWABASE_resolveGlitterParams(const uniform DwaBase * uniform dwaBase,
                             const uniform Material *uniform me,
                             uniform ShadingTLState * uniform tls,
                             const varying State &state,
                             const uniform bool castsCaustics,
                             varying DwaBaseParameters * uniform params)
{
    if (dwaBase->mHints.mRequiresGlitterParams) {
        params->mGlitterPointerVector = dwaBase->mGlitterPointer;
        params->mGlitterUniformParameters = dwaBase->mGlitterUniformParameters;

        GLITTER_VaryingParameters& vParams = params->mGlitterVaryingParameters;

        if (dwaBase->mHints.mGlitterIsOne) {
            vParams.mGlitterMask = 1.0f;
        } else {
            vParams.mGlitterMask =
                saturate(EVALFLOATATTR(dwaBase->mAttrFuncs.mEvalAttrGlitter, me, tls, state, 0.0f));
        }

        // Gather varying attributes
        vParams.mFlakeStyleFrequency[0] =
            EVALFLOATATTR(dwaBase->mAttrFuncs.mEvalAttrGlitterStyleAFrequency, me, tls, state, 1.0f);

        vParams.mFlakeStyleFrequency[1] =
            EVALFLOATATTR(dwaBase->mAttrFuncs.mEvalAttrGlitterStyleBFrequency, me, tls, state, 1.0f);

        // These conditionals ensure that if the style is "off" then its flake size is 0
        // so that the max() calculation in NoiseWorley::searchPoints is correct.
        if (!isZero(vParams.mFlakeStyleFrequency[0])) {
            vParams.mFlakeColor[0] =
                EVALCOLORATTR(dwaBase->mAttrFuncs.mEvalAttrGlitterColorA, me, tls, state, Color_ctor(1.0f));
            vParams.mFlakeSize[0] =
                EVALFLOATATTR(dwaBase->mAttrFuncs.mEvalAttrGlitterSizeA, me, tls, state, 1.0f);
            vParams.mFlakeRoughness[0] =
                GETFLOATATTR(dwaBase->mAttrFuncs.mGetAttrGlitterRoughnessA, me, tls, state, 0.14f);
        } else {
            vParams.mFlakeColor[0] = sBlack;
            vParams.mFlakeSize[0] = 0.f;
            vParams.mFlakeRoughness[0] = 0.14f;
        }

        if (!isZero(vParams.mFlakeStyleFrequency[1])) {
            vParams.mFlakeColor[1] =
                EVALCOLORATTR(dwaBase->mAttrFuncs.mEvalAttrGlitterColorB, me, tls, state, Color_ctor(1.0f));
            vParams.mFlakeSize[1] =
                EVALFLOATATTR(dwaBase->mAttrFuncs.mEvalAttrGlitterSizeB, me, tls, state, 1.0f);
            vParams.mFlakeRoughness[1] =
                GETFLOATATTR(dwaBase->mAttrFuncs.mGetAttrGlitterRoughnessB, me, tls, state, 0.14f);
        } else {
            vParams.mFlakeColor[1] = sBlack;
            vParams.mFlakeSize[1] = 0.f;
            vParams.mFlakeRoughness[1] = 0.14f;
        }

        vParams.mFlakeHSVColorVariation =
            Vec3f_ctor(EVALFLOATATTR(dwaBase->mAttrFuncs.mEvalAttrGlitterHueVariation, me, tls, state, 0.0f),
                       EVALFLOATATTR(dwaBase->mAttrFuncs.mEvalAttrGlitterSaturationVariation, me, tls, state, 0.0f),
                       EVALFLOATATTR(dwaBase->mAttrFuncs.mEvalAttrGlitterValueVariation, me, tls, state, 0.0f));

        vParams.mFlakeDensity =
            EVALFLOATATTR(dwaBase->mAttrFuncs.mEvalAttrGlitterDensity, me, tls, state, 1.0f);

        vParams.mFlakeJitter =
            EVALFLOATATTR(dwaBase->mAttrFuncs.mEvalAttrGlitterJitter, me, tls, state, 1.0f);

        vParams.mFlakeOrientationRandomness =
            GETFLOATATTR(dwaBase->mAttrFuncs.mGetAttrGlitterOrientationRandomness, me, tls, state, 0.15f);

        vParams.mCompensateDeformation =
            GETBOOLATTR(dwaBase->mAttrFuncs.mGetAttrGlitterCompensateDeformation, me, true);

        vParams.mApproximateForSecRays =
            GETBOOLATTR(dwaBase->mAttrFuncs.mGetAttrGlitterApproximateForSecondaryRays, me, true);
    }
}

inline void
evaluateToonRampColorMultipliers(const uniform ToonDiffuseAttributeFuncs& attrFuncs,
                                 const uniform Material *uniform me,
                                 uniform ShadingTLState * uniform tls,
                                 const varying State &state,
                                 Color rampColors[DWABASE_MAX_TOOND_RAMP_POINTS])
{
    MNRY_ASSERT(DWABASE_MAX_TOOND_RAMP_POINTS == 10);
    rampColors[0] = EVALCOLORATTR(attrFuncs.mEvalAttrRampColorMultiplier0,
                                  me, tls, state, Color_ctor(1.0f));
    rampColors[1] = EVALCOLORATTR(attrFuncs.mEvalAttrRampColorMultiplier1,
                                  me, tls, state, Color_ctor(1.0f));
    rampColors[2] = EVALCOLORATTR(attrFuncs.mEvalAttrRampColorMultiplier2,
                                  me, tls, state, Color_ctor(1.0f));
    rampColors[3] = EVALCOLORATTR(attrFuncs.mEvalAttrRampColorMultiplier3,
                                  me, tls, state, Color_ctor(1.0f));
    rampColors[4] = EVALCOLORATTR(attrFuncs.mEvalAttrRampColorMultiplier4,
                                  me, tls, state, Color_ctor(1.0f));
    rampColors[5] = EVALCOLORATTR(attrFuncs.mEvalAttrRampColorMultiplier5,
                                  me, tls, state, Color_ctor(1.0f));
    rampColors[6] = EVALCOLORATTR(attrFuncs.mEvalAttrRampColorMultiplier6,
                                  me, tls, state, Color_ctor(1.0f));
    rampColors[7] = EVALCOLORATTR(attrFuncs.mEvalAttrRampColorMultiplier7,
                                  me, tls, state, Color_ctor(1.0f));
    rampColors[8] = EVALCOLORATTR(attrFuncs.mEvalAttrRampColorMultiplier8,
                                  me, tls, state, Color_ctor(1.0f));
    rampColors[9] = EVALCOLORATTR(attrFuncs.mEvalAttrRampColorMultiplier9,
                                  me, tls, state, Color_ctor(1.0f));
}

inline void
evaluateToonRampOffsets(const uniform ToonDiffuseAttributeFuncs& attrFuncs,
                        const uniform Material *uniform me,
                        uniform ShadingTLState * uniform tls,
                        const varying State &state,
                        float offsets[DWABASE_MAX_TOOND_RAMP_POINTS])
{
    MNRY_ASSERT(DWABASE_MAX_TOOND_RAMP_POINTS == 10);
    offsets[0] = EVALFLOATATTR(attrFuncs.mEvalAttrRampPositionOffset0,
                               me, tls, state, 0.0f);
    offsets[1] = EVALFLOATATTR(attrFuncs.mEvalAttrRampPositionOffset1,
                               me, tls, state, 0.0f);
    offsets[2] = EVALFLOATATTR(attrFuncs.mEvalAttrRampPositionOffset2,
                               me, tls, state, 0.0f);
    offsets[3] = EVALFLOATATTR(attrFuncs.mEvalAttrRampPositionOffset3,
                               me, tls, state, 0.0f);
    offsets[4] = EVALFLOATATTR(attrFuncs.mEvalAttrRampPositionOffset4,
                               me, tls, state, 0.0f);
    offsets[5] = EVALFLOATATTR(attrFuncs.mEvalAttrRampPositionOffset5,
                               me, tls, state, 0.0f);
    offsets[6] = EVALFLOATATTR(attrFuncs.mEvalAttrRampPositionOffset6,
                               me, tls, state, 0.0f);
    offsets[7] = EVALFLOATATTR(attrFuncs.mEvalAttrRampPositionOffset7,
                               me, tls, state, 0.0f);
    offsets[8] = EVALFLOATATTR(attrFuncs.mEvalAttrRampPositionOffset8,
                               me, tls, state, 0.0f);
    offsets[9] = EVALFLOATATTR(attrFuncs.mEvalAttrRampPositionOffset9,
                               me, tls, state, 0.0f);
}

inline void
evaluateToonSpecRampMultipliers(const uniform ToonSpecularAttributeFuncs& attrFuncs,
                                const uniform Material *uniform me,
                                uniform ShadingTLState * uniform tls,
                                const varying State &state,
                                float multipliers[DWABASE_MAX_TOOND_RAMP_POINTS])
{
    MNRY_ASSERT(DWABASE_MAX_TOOND_RAMP_POINTS == 10);
    multipliers[0] = EVALFLOATATTR(attrFuncs.mEvalAttrRampMultiplier0,
                               me, tls, state, 0.0f);
    multipliers[1] = EVALFLOATATTR(attrFuncs.mEvalAttrRampMultiplier1,
                               me, tls, state, 0.0f);
    multipliers[2] = EVALFLOATATTR(attrFuncs.mEvalAttrRampMultiplier2,
                               me, tls, state, 0.0f);
    multipliers[3] = EVALFLOATATTR(attrFuncs.mEvalAttrRampMultiplier3,
                               me, tls, state, 0.0f);
    multipliers[4] = EVALFLOATATTR(attrFuncs.mEvalAttrRampMultiplier4,
                               me, tls, state, 0.0f);
    multipliers[5] = EVALFLOATATTR(attrFuncs.mEvalAttrRampMultiplier5,
                               me, tls, state, 0.0f);
    multipliers[6] = EVALFLOATATTR(attrFuncs.mEvalAttrRampMultiplier6,
                               me, tls, state, 0.0f);
    multipliers[7] = EVALFLOATATTR(attrFuncs.mEvalAttrRampMultiplier7,
                               me, tls, state, 0.0f);
    multipliers[8] = EVALFLOATATTR(attrFuncs.mEvalAttrRampMultiplier8,
                               me, tls, state, 0.0f);
    multipliers[9] = EVALFLOATATTR(attrFuncs.mEvalAttrRampMultiplier9,
                               me, tls, state, 0.0f);
}

inline void
evaluateToonSpecRampOffsets(const uniform ToonSpecularAttributeFuncs& attrFuncs,
                            const uniform Material *uniform me,
                            uniform ShadingTLState * uniform tls,
                            const varying State &state,
                            float offsets[DWABASE_MAX_TOOND_RAMP_POINTS])
{
    MNRY_ASSERT(DWABASE_MAX_TOOND_RAMP_POINTS == 10);
    offsets[0] = EVALFLOATATTR(attrFuncs.mEvalAttrRampPositionOffset0,
                               me, tls, state, 0.0f);
    offsets[1] = EVALFLOATATTR(attrFuncs.mEvalAttrRampPositionOffset1,
                               me, tls, state, 0.0f);
    offsets[2] = EVALFLOATATTR(attrFuncs.mEvalAttrRampPositionOffset2,
                               me, tls, state, 0.0f);
    offsets[3] = EVALFLOATATTR(attrFuncs.mEvalAttrRampPositionOffset3,
                               me, tls, state, 0.0f);
    offsets[4] = EVALFLOATATTR(attrFuncs.mEvalAttrRampPositionOffset4,
                               me, tls, state, 0.0f);
    offsets[5] = EVALFLOATATTR(attrFuncs.mEvalAttrRampPositionOffset5,
                               me, tls, state, 0.0f);
    offsets[6] = EVALFLOATATTR(attrFuncs.mEvalAttrRampPositionOffset6,
                               me, tls, state, 0.0f);
    offsets[7] = EVALFLOATATTR(attrFuncs.mEvalAttrRampPositionOffset7,
                               me, tls, state, 0.0f);
    offsets[8] = EVALFLOATATTR(attrFuncs.mEvalAttrRampPositionOffset8,
                               me, tls, state, 0.0f);
    offsets[9] = EVALFLOATATTR(attrFuncs.mEvalAttrRampPositionOffset9,
                               me, tls, state, 0.0f);
}

inline void
resolveToonDiffuseParams(const uniform Material *uniform me,
                         uniform ShadingTLState * uniform tls,
                         const varying State &state,
                         const uniform DwaBase * uniform dwaBase,
                         const uniform ToonDiffuseUniformData& uniformData,
                         const uniform ToonDiffuseAttributeFuncs& attrFuncs,
                         varying DwaBaseParameters * uniform params,
                         varying ToonDiffuseParameters& toonDParams)
{
    toonDParams.mToonDiffuse = 1.0f;
    toonDParams.mModel = GETINTATTR(attrFuncs.mGetAttrModel, me, TOON_DIFFUSE_RAMP_V1);
    if (toonDParams.mModel == TOON_DIFFUSE_OREN_NAYAR) {
        toonDParams.mTerminatorShift =
            clamp(EVALFLOATATTR(attrFuncs.mEvalAttrTerminatorShift, me, tls, state, 0.0f),
                  -1.0f, 1.0f);
        toonDParams.mFlatness =
            clamp(EVALFLOATATTR(attrFuncs.mEvalAttrFlatness, me, tls, state, 0.0f),
                  0.0f, 1.0f);
        toonDParams.mFlatnessFalloff =
            clamp(EVALFLOATATTR(attrFuncs.mEvalAttrFlatnessFalloff, me, tls, state, 0.0f),
                  0.0f, 1.0f);
    }

    // Eval normal separately as a toon normal so adaptNormal
    // can be disabled for only diffuse lobes
    Vec3f unadaptedN;
    Vec3f nml = evalToonNormal(me, tls, state,
                               dwaBase->mNormalMap,
                               (SampleNormalPtr)dwaBase->mSampleNormalFunc,
                               params->mNormalDial,
                               nullptr,
                               &unadaptedN);
    toonDParams.mNormal = unadaptedN;

    if (toonDParams.mModel == TOON_DIFFUSE_RAMP_V1 ||
        toonDParams.mModel == TOON_DIFFUSE_RAMP_V2) {
        toonDParams.mRampWeight = 1.0f;

         // Get the ramp data points and validate them
        toonDParams.mRampNumPoints = min(DWABASE_MAX_TOOND_RAMP_POINTS,
                                         uniformData.mRampNumPoints);
        evaluateToonRampColorMultipliers(attrFuncs, me, tls, state,
                                         toonDParams.mRampColors);

        float offsets[DWABASE_MAX_TOOND_RAMP_POINTS];
        evaluateToonRampOffsets(attrFuncs, me, tls, state, offsets);

        for (int i = 0; i < toonDParams.mRampNumPoints; ++i) {
            toonDParams.mRampInterpolators[i] = dwaBase->mToonDiffuseData.mRampInterpolators[i];
            // position offsets
            toonDParams.mRampPositions[i] = dwaBase->mToonDiffuseData.mRampPositions[i] + offsets[i];
            // color multipliers
            toonDParams.mRampColors[i] = toonDParams.mRampColors[i] * dwaBase->mToonDiffuseData.mRampColors[i];
        }
        toonDParams.mExtendRamp = GETBOOLATTR(attrFuncs.mGetAttrExtendRamp, me, false);
        toonDParams.mRampInputScale = EVALFLOATATTR(attrFuncs.mEvalAttrRampInputScale, me, tls, state, 1.0f);
    }
}

inline void
resolveToonSpecParams(const uniform Material *uniform me,
                      uniform ShadingTLState * uniform tls,
                      const varying State &state,
                      const float specular,
                      const uniform ToonSpecularUniformData& uniformData,
                      const uniform ToonSpecularAttributeFuncs& attrFuncs,
                      varying ToonSpecularParameters& params,
                      const uniform ToonSpecularModel model)
{
    params.mToonSpecular = specular;

    params.mIntensity = EVALFLOATATTR(attrFuncs.mEvalAttrIntensity,
                                      me, tls, state, 1.0f);
    params.mFresnelBlend = EVALFLOATATTR(attrFuncs.mEvalAttrFresnelBlend,
                                         me, tls, state, 1.0f);
    params.mRoughness = EVALFLOATATTR(attrFuncs.mEvalAttrRoughness,
                                      me, tls, state, 1.0f);
    params.mTint =
        clamp(EVALCOLORATTR(attrFuncs.mEvalAttrTint, me, tls, state, Color_ctor(1.0f)),
              0.0f, 1.0f);
    params.mRampInputScale = EVALFLOATATTR(attrFuncs.mEvalAttrRampInputScale, me, tls, state, 1.0f);

    params.mRampNumPoints = min(DWABASE_MAX_TOOND_RAMP_POINTS, uniformData.mRampNumPoints);
    float offsets[DWABASE_MAX_TOOND_RAMP_POINTS];
    evaluateToonSpecRampOffsets(attrFuncs, me, tls, state, offsets);
    float multipliers[DWABASE_MAX_TOOND_RAMP_POINTS];
    evaluateToonSpecRampMultipliers(attrFuncs, me, tls, state, multipliers);
    for (int i = 0; i < params.mRampNumPoints; ++i) {
        params.mRampPositions[i] = uniformData.mRampPositions[i] + offsets[i];
        params.mRampValues[i] = uniformData.mRampValues[i] * multipliers[i];
        params.mRampInterpolators[i] = uniformData.mRampInterpolators[i];
    }

    if (GETBOOLATTR(attrFuncs.mGetAttrEnableInputNormal, me, false)) {
        const float normalDial = EVALFLOATATTR(attrFuncs.mEvalAttrInputNormalDial,
                                               me, tls, state, 1.0f);
        params.mNormal = evalNormal(me, tls, state,
                                         uniformData.mNormalMap,
                                         (SampleNormalPtr)uniformData.mSampleNormalFunc,
                                         normalDial,
                                         NULL); // unadapted normal
    } else {
        params.mNormal = state.mN;
    }

    params.mStretchU = EVALFLOATATTR(attrFuncs.mEvalAttrStretchU,
                                          me, tls, state, 1.0f);
    params.mStretchV = EVALFLOATATTR(attrFuncs.mEvalAttrStretchV,
                                          me, tls, state, 1.0f);
    if (GETBOOLATTR(attrFuncs.mGetAttrUseInputVectors, me, false)) {
        params.mdPds = normalize(EVALVEC3FATTR(attrFuncs.mEvalAttrInputU,
                                      me, tls, state, Vec3f_ctor(0.0)));
        params.mdPdt = normalize(EVALVEC3FATTR(attrFuncs.mEvalAttrInputV,
                                      me, tls, state, Vec3f_ctor(0.0)));
    } else {
        params.mdPds = normalize(state.mdPds);
        params.mdPdt = normalize(state.mdPdt);
    }

    params.mEnableIndirectReflections = GETBOOLATTR(attrFuncs.mGetAttrEnableIndirectReflections, me, false);
    if (params.mEnableIndirectReflections) {
        params.mIndirectReflectionsIntensity = EVALFLOATATTR(attrFuncs.mEvalAttrIndirectReflectionsIntensity,
                                                             me, tls, state, 1.0f);
        params.mIndirectReflectionsRoughness = EVALFLOATATTR(attrFuncs.mEvalAttrIndirectReflectionsRoughness,
                                                             me, tls, state, 0.5f);
    }

    if (model == ToonSpecularHair) {
        params.mHairDir = normalize(state.mdPds);
        params.mHairUV = state.mSt;
    }
}

inline void
resolveToonParams(const uniform DwaBase * uniform dwaBase,
                  const uniform Material *uniform me,
                  uniform ShadingTLState * uniform tls,
                  const varying State &state,
                  varying DwaBaseParameters * uniform params)
{
    if (dwaBase->mHints.mRequiresToonDiffuseParams) {
        resolveToonDiffuseParams(me, tls, state, dwaBase,
                                 dwaBase->mToonDiffuseData,
                                 dwaBase->mAttrFuncs.mToonDiffuseFuncs,
                                 params,
                                 params->mToonDiffuseParams);
    }

    if (dwaBase->mHints.mRequiresToonSpecularParams) {
        resolveToonSpecParams(me, tls, state,
                              params->mSpecular,
                              dwaBase->mToonSpecularData,
                              dwaBase->mAttrFuncs.mToonSpecularFuncs,
                              params->mToonSpecularParams,
                              dwaBase->mUParams.mToonSpecularModel);
        // When the toon specular doesn't have its own input normal enabled, or
        // it is enabled but no specific normal map is connected, fall back to
        // params->mNormal
        if (!GETBOOLATTR(dwaBase->mAttrFuncs.mToonSpecularFuncs.mGetAttrEnableInputNormal, me, false) ||
            dwaBase->mToonSpecularData.mNormalMap == NULL) {
            params->mToonSpecularParams.mNormal = params->mNormal;
        }
    }
}

inline void
resolveHairToonParams(const uniform DwaBase * uniform dwaBase,
                      const uniform Material *uniform me,
                      uniform ShadingTLState * uniform tls,
                      const varying State &state,
                      varying DwaBaseParameters * uniform params)
{
    if (dwaBase->mHints.mRequiresHairToonS1Params) {
        resolveToonSpecParams(me, tls, state,
                              1.0f, // specular
                              dwaBase->mHairToonS1Data,
                              dwaBase->mAttrFuncs.mHairToonS1Funcs,
                              params->mHairToonS1Params,
                              dwaBase->mUParams.mHairToonS1Model);
    }

    if (dwaBase->mHints.mRequiresHairToonS2Params) {
        resolveToonSpecParams(me, tls, state,
                              1.0f, // specular
                              dwaBase->mHairToonS2Data,
                              dwaBase->mAttrFuncs.mHairToonS2Funcs,
                              params->mHairToonS2Params,
                              dwaBase->mUParams.mHairToonS2Model);
    }

    if (dwaBase->mHints.mRequiresHairToonS3Params) {
        resolveToonSpecParams(me, tls, state,
                              1.0f, // specular
                              dwaBase->mHairToonS3Data,
                              dwaBase->mAttrFuncs.mHairToonS3Funcs,
                              params->mHairToonS3Params,
                              dwaBase->mUParams.mHairToonS3Model);
    }
}

inline void
resolveHairParams(const uniform DwaBase * uniform dwaBase,
                  const uniform Material *uniform me,
                  uniform ShadingTLState * uniform tls,
                  const varying State &state,
                  const uniform bool castsCaustics,
                  varying HairParameters * uniform params)
{
    if (castsCaustics || !isCausticPath(state)) {
        params->mHairCastsCaustics = true;
    }

    // Common parameters
    if (dwaBase->mHints.mRequiresHairParams || dwaBase->mHints.mRequiresHairDiffuseParams) {
        params->mHairDir = normalize(state.mdPds);
    }

    if (dwaBase->mHints.mRequiresHairParams && !dwaBase->mHints.mHairDiffuseIsOne) {
        params->mHairColor =
            clamp(EVALCOLORATTR(dwaBase->mAttrFuncs.mEvalAttrHairColor, me, tls, state, Color_ctor(1.0f)),
                  0.0f, 1.0f);
    }

    // HairMaterial_v3 parameters
    if (dwaBase->mHints.mRequiresHairParams) {
        if (dwaBase->mModel == Hair || dwaBase->mHints.mHairIsOne) {
            params->mHair = 1.0f;
        } else {
            params->mHair = saturate(EVALFLOATATTR(dwaBase->mAttrFuncs.mEvalAttrHair, me, tls, state, 0.0));
        }
        if (!isZero(params->mHair) && params->mHairCastsCaustics) {
            /// @brief An Energy Conserving Hair Material based on the following papers:
            /// [1] An Energy-conserving Hair Reflectance Model - D'eon et al Sig'11
            /// [2] A Practical and Controllable Hair and Fur Model for Production Path Tracing - Chiang et al EGSR'16
            /// [3] Importance Sampling for Physically-Based Hair Fiber Models, D'eon et al Sig'13
            /// [4] Light Scattering from Human Hair Fibers - Marschner et al

            static const uniform float sMinAziRoughness = 0.01f;
            static const uniform float sMaxAziRoughness = (1.0f - sEpsilon);

            // Hair Need Azimuthal Roughness To Compute Absorption Coefficients
            // Hair seems unstable below 0.01, put roughness in [0.01,1.0)
            params->mHairTTAzimRoughness =
                clamp(EVALFLOATATTR(dwaBase->mAttrFuncs.mEvalAttrHairTTAzimuthalRoughness, me, tls, state, 1.0f),
                      sMinAziRoughness, sMaxAziRoughness);

            params->mHairUV = state.mSt;
            params->mHairIOR = GETFLOATATTR(dwaBase->mAttrFuncs.mGetAttrRefractiveIndex, me, tls, state, 1.45f);

            params->mHairShowR = GETBOOLATTR(dwaBase->mAttrFuncs.mGetAttrHairShowR, me, true);
            params->mHairRLongRoughness =
                EVALFLOATATTR(dwaBase->mAttrFuncs.mEvalAttrHairRRoughness, me, tls, state, 0.5f);

            if (params->mHairShowR) {
                // No Specular Offset beyond 10 degrees max makes sense for hair
                // Marschner suggests this should be [2,5] degrees
                const float rOffset =
                    clamp(EVALFLOATATTR(dwaBase->mAttrFuncs.mEvalAttrHairROffset, me, tls, state, 0.0f),
                          -10.0f, 10.0f);

                params->mHairRShift = deg2rad(rOffset);
                params->mHairRTint =
                    EVALCOLORATTR(dwaBase->mAttrFuncs.mEvalAttrHairRTintColor, me, tls, state, Color_ctor(1.0f));
            }

            params->mHairShowTT = GETBOOLATTR(dwaBase->mAttrFuncs.mGetAttrHairShowTT, me, true);

            // Roughness according to Table 1 in [4]
            params->mHairTTLongRoughness = 0.5f * params->mHairRLongRoughness;

            if (params->mHairShowTT) {
                if (GETBOOLATTR(dwaBase->mAttrFuncs.mGetAttrHairTTUseRoughness, me, false)) {
                    params->mHairTTLongRoughness =
                        EVALFLOATATTR(dwaBase->mAttrFuncs.mEvalAttrHairTTRoughness, me, tls, state, 0.1f);
                }
                const float ttOffset =
                    clamp(EVALFLOATATTR(dwaBase->mAttrFuncs.mEvalAttrHairTTOffset, me, tls, state, 0.0f),
                          -10.0f, 10.0f);
                params->mHairTTShift = deg2rad(ttOffset);
                params->mHairTTTint =
                    EVALCOLORATTR(dwaBase->mAttrFuncs.mEvalAttrHairTTTintColor, me, tls, state, Color_ctor(1.0f));
                // MOONSHINE-1238: only for HiFi hits
                if (isHifi(state)) {
                    params->mHairTTSaturation = EVALFLOATATTR(dwaBase->mAttrFuncs.mEvalAttrHairTTSaturation,
                                                              me, tls, state, 1.0f);
                } else {
                    params->mHairTTSaturation = 1.0f;
                }
            }

            params->mHairShowTRT = GETBOOLATTR(dwaBase->mAttrFuncs.mGetAttrHairShowTRT, me, true);

            // Roughness according to Table 1 in [4]
            params->mHairTRTLongRoughness = 2.0f * params->mHairRLongRoughness;

            if (params->mHairShowTRT) {
                if (GETBOOLATTR(dwaBase->mAttrFuncs.mGetAttrHairTRTUseRoughness, me, false)) {
                    params->mHairTRTLongRoughness =
                        EVALFLOATATTR(dwaBase->mAttrFuncs.mEvalAttrHairTRTRoughness, me, tls, state, 0.4f);
                }
                const float trtOffset =
                    clamp(EVALFLOATATTR(dwaBase->mAttrFuncs.mEvalAttrHairTRTOffset, me, tls, state, 0.0f),
                          -10.0f, 10.0f);

                params->mHairTRTShift = deg2rad(trtOffset);
                params->mHairTRTTint =
                    EVALCOLORATTR(dwaBase->mAttrFuncs.mEvalAttrHairTRTTintColor, me, tls, state, Color_ctor(1.0f));

                params->mHairShowGlint = GETBOOLATTR(dwaBase->mAttrFuncs.mGetAttrShowHairGlint, me, false);
                if (params->mHairShowGlint) {
                    params->mHairGlintRoughness = EVALFLOATATTR(dwaBase->mAttrFuncs.mEvalAttrHairGlintRoughness, me, tls, state, 0.5f);
                    params->mHairGlintMinTwists = EVALFLOATATTR(dwaBase->mAttrFuncs.mEvalAttrHairGlintMinTwists, me, tls, state, 1.5f);
                    params->mHairGlintMaxTwists = EVALFLOATATTR(dwaBase->mAttrFuncs.mEvalAttrHairGlintMaxTwists, me, tls, state, 2.5f);
                    params->mHairGlintEccentricity = EVALFLOATATTR(dwaBase->mAttrFuncs.mEvalAttrHairGlintEccentricity, me, tls, state, 0.85f);
                    params->mHairGlintSaturation = EVALFLOATATTR(dwaBase->mAttrFuncs.mEvalAttrHairGlintSaturation, me, tls, state, 0.5f);
                }
            }

            params->mHairShowTRRT = GETBOOLATTR(dwaBase->mAttrFuncs.mGetAttrHairShowTRRTPlus, me, true);

            // Section 3.4 in [2]
            // Using a wide Longitudinal Lobe (4*hairParams.mRLongRoughness)
            // Internally it uses an Isotropic Azimuthal term
            params->mHairTRRTLongRoughness = 4.0f * params->mHairRLongRoughness;

            params->mHairFresnelType =
                (HairFresnelType)(GETINTATTR(dwaBase->mAttrFuncs.mGetAttrHairFresnelType, me, 1));

            params->mHairCuticleLayerThickness =
                clamp(EVALFLOATATTR(dwaBase->mAttrFuncs.mEvalAttrHairCuticleLayerThickness, me, tls, state, 0.1f),
                                    0.0f, 1.0f);

            params->mHairUseOptimizedSampling = dwaBase->mHints.mDisableOptimizedHairSampling ? false :
                GETBOOLATTR(dwaBase->mAttrFuncs.mGetAttrHairUseOptimizedSampling, me, true);
        }
    }

    // HairDiffuseMaterial parameters
    if (dwaBase->mHints.mRequiresHairDiffuseParams) {
        if (dwaBase->mModel == HairDiffuse || dwaBase->mHints.mHairDiffuseIsOne) {
            params->mHairDiffuse = 1.0f;
        } else {
            params->mHairDiffuse =
                saturate(EVALFLOATATTR(dwaBase->mAttrFuncs.mEvalAttrHairDiffuse, me, tls, state, 0.0));
        }

        if (!isZero(params->mHairDiffuse)) {
            params->mHairDiffuseUseIndependentFrontAndBackColor =
                GETBOOLATTR(dwaBase->mAttrFuncs.mGetAttrHairDiffuseUseIndependentFrontAndBackColor, me, false);

            params->mHairSubsurfaceBlend = 
                saturate(EVALFLOATATTR(dwaBase->mAttrFuncs.mEvalAttrHairSubsurfaceBlend, me, tls, state, 1.0));

            if (params->mHairDiffuseUseIndependentFrontAndBackColor) {
                params->mHairDiffuseFrontColor =
                    EVALCOLORATTR(dwaBase->mAttrFuncs.mEvalAttrHairDiffuseFrontColor, me, tls, state, Color_ctor(1.0f));
                params->mHairDiffuseBackColor =
                    EVALCOLORATTR(dwaBase->mAttrFuncs.mEvalAttrHairDiffuseBackColor, me, tls, state, Color_ctor(1.0f));
            } else {
                // Use the same hair color for front and back
                params->mHairDiffuseFrontColor =
                    EVALCOLORATTR(dwaBase->mAttrFuncs.mEvalAttrHairColor, me, tls, state, Color_ctor(1.0f));
                params->mHairDiffuseBackColor = params->mHairDiffuseFrontColor;
            }
        }
    }
}

inline void
resolveFuzzParams(const uniform DwaBase * uniform dwaBase,
                  const uniform Material *uniform me,
                  uniform ShadingTLState * uniform tls,
                  const varying State &state,
                  const uniform bool castsCaustics,
                  varying DwaBaseParameters * uniform params)
{
    if (dwaBase->mHints.mRequiresFuzzParams &&
        (castsCaustics || !isCausticPath(state))) {

        params->mFuzz =
            saturate(EVALFLOATATTR(dwaBase->mAttrFuncs.mEvalAttrFuzz, me, tls, state, 1.0));
        params->mFuzzAlbedo =
            clamp(EVALCOLORATTR(dwaBase->mAttrFuncs.mEvalAttrFuzzAlbedo, me, tls, state, Color_ctor(0.0)), 0.0, 1.0);
        params->mFuzzRoughness =
            saturate(EVALFLOATATTR(dwaBase->mAttrFuncs.mEvalAttrFuzzRoughness, me, tls, state, 0.0));

        if (dwaBase->mFuzzNormalMap) {
            params->mFuzzNormalDial = EVALFLOATATTR(dwaBase->mAttrFuncs.mEvalAttrFuzzNormalDial, me, tls, state, 1.0f);
            params->mFuzzNormal = evalNormal(me,
                                             tls,
                                             state,
                                             dwaBase->mFuzzNormalMap,
                                             (SampleNormalPtr)dwaBase->mFuzzSampleNormalFunc,
                                             params->mFuzzNormalDial,
                                             NULL); // unadapted normal
        } else {
            params->mFuzzNormal = state.mN;
        }

        params->mFuzzUseAbsorbingFibers = GETBOOLATTR(dwaBase->mAttrFuncs.mGetAttrFuzzUseAbsorbingFibers, me, false);
    }
}

inline void
resolveOuterSpecularParams(const uniform DwaBase * uniform dwaBase,
                           const uniform Material *uniform me,
                           uniform ShadingTLState * uniform tls,
                           const varying State &state,
                           const uniform bool castsCaustics,
                           varying DwaBaseParameters * uniform params)
{
    params->mOuterSpecularNormal = getN(state);
    if (dwaBase->mHints.mRequiresOuterSpecularParams &&
        (castsCaustics || !isCausticPath(state))) {
        params->mOuterSpecular = saturate(EVALFLOATATTR(dwaBase->mAttrFuncs.mEvalAttrOuterSpecular, me, tls, state, 0.0));
        params->mOuterSpecularRefractiveIndex = max(sEpsilon, GETFLOATATTR(dwaBase->mAttrFuncs.mGetAttrOuterSpecularRefractiveIndex, me, tls, state, 1.5));
        params->mOuterSpecularRoughness = saturate(EVALFLOATATTR(dwaBase->mAttrFuncs.mEvalAttrOuterSpecularRoughness, me, tls, state, 0.0));

        if (GETBOOLATTR(dwaBase->mAttrFuncs.mGetAttrUseOuterSpecularNormal, me, false)) {
            if (dwaBase->mOuterSpecularNormalMap) {
                params->mOuterSpecularNormalDial = EVALFLOATATTR(dwaBase->mAttrFuncs.mEvalAttrOuterSpecularNormalDial, me, tls, state, 1.0f);
                params->mOuterSpecularNormal = evalNormal(me,
                                                          tls,
                                                          state,
                                                          dwaBase->mOuterSpecularNormalMap,
                                                          (SampleNormalPtr)dwaBase->mOuterSpecularSampleNormalFunc,
                                                          params->mOuterSpecularNormalDial,
                                                          &params->mOuterSpecularNormalLength,
                                                          NULL); // unadapted normal
            }
        }

        if (dwaBase->mHints.mRequiresOuterSpecularAbsorptionParams) {
            params->mOuterSpecularThickness =
                max(0.0f, EVALFLOATATTR(dwaBase->mAttrFuncs.mEvalAttrOuterSpecularThickness, me, tls, state, 0.0));
            params->mOuterSpecularAttenuationColor =
                clamp(EVALCOLORATTR(dwaBase->mAttrFuncs.mEvalAttrOuterSpecularAttenuationColor, me, tls, state, Color_ctor(0.0)), 0.0, 1.0);
        }
    }
}

inline void
resolveIridescenceParams(const uniform DwaBase * uniform dwaBase,
                         const uniform Material *uniform me,
                         uniform ShadingTLState * uniform tls,
                         const varying State &state,
                         const uniform bool castsCaustics,
                         varying IridescenceParameters * uniform params)
{
    if (dwaBase->mHints.mRequiresIridescenceParams &&
        (castsCaustics || !isCausticPath(state))) {
        params->mIridescence = saturate(EVALFLOATATTR(dwaBase->mAttrFuncs.mEvalAttrIridescence, me, tls, state, 0.0f));
        params->mIridescenceApplyTo = (IridescenceLobe)(GETINTATTR(dwaBase->mAttrFuncs.mGetAttrIridescenceApplyTo, me, 0));
        params->mIridescenceColorControl = (SHADING_IridescenceColorMode)(GETINTATTR(dwaBase->mAttrFuncs.mGetAttrIridescenceColorControl, me, 0));
        params->mIridescencePrimaryColor = clamp(EVALCOLORATTR(dwaBase->mAttrFuncs.mEvalAttrIridescencePrimaryColor, me, tls, state, Color_ctor(0.0f)), 0, 1);
        params->mIridescenceSecondaryColor = clamp(EVALCOLORATTR(dwaBase->mAttrFuncs.mEvalAttrIridescenceSecondaryColor, me, tls, state, Color_ctor(0.0f)), 0, 1);
        params->mIridescenceFlipHueDirection = GETBOOLATTR(dwaBase->mAttrFuncs.mGetAttrIridescenceFlipHueDirection, me, false);
        params->mIridescenceThickness = max(0.0f, EVALFLOATATTR(dwaBase->mAttrFuncs.mEvalAttrIridescenceThickness, me, tls, state, 0.0f));
        params->mIridescenceExponent = max(0.0f, EVALFLOATATTR(dwaBase->mAttrFuncs.mEvalAttrIridescenceExponent, me, tls, state, 0.0f));
        params->mIridescenceAt0 = saturate(EVALFLOATATTR(dwaBase->mAttrFuncs.mEvalAttrIridescenceAt0, me, tls, state, 0.0f));
        params->mIridescenceAt90 = saturate(EVALFLOATATTR(dwaBase->mAttrFuncs.mEvalAttrIridescenceAt90, me, tls, state, 1.0f));

        params->mIridescenceRampInterpolationMode = 
            (ColorRampControlSpace)(GETINTATTR(dwaBase->mAttrFuncs.mGetAttrIridescenceRampInterpolationMode, me, 0));
        // Get the ramp data points and validate them
        params->mIridescenceRampNumPoints = min(DWABASE_MAX_IRIDESCENCE_RAMP_POINTS,
            dwaBase->mIridescenceData.mRampNumPoints);

        // Copy ramp point data gathered in update()
        for (int i = 0; i < params->mIridescenceRampNumPoints; ++i) {
            params->mIridescenceRampInterpolators[i] = dwaBase->mIridescenceData.mRampInterpolators[i];
            params->mIridescenceRampPositions[i] = dwaBase->mIridescenceData.mRampPositions[i];
            params->mIridescenceRampColors[i] = dwaBase->mIridescenceData.mRampColors[i];

        }
    }
}

inline void
resolveSpecularParams(const uniform DwaBase * uniform dwaBase,
                      const uniform Material *uniform me,
                      uniform ShadingTLState * uniform tls,
                      const varying State &state,
                      const uniform bool castsCaustics,
                      varying DwaBaseParameters * uniform params)
{
    if (dwaBase->mHints.mRequiresSpecularParams &&
        (castsCaustics || !isCausticPath(state))) {
        params->mSpecular = saturate(EVALFLOATATTR(dwaBase->mAttrFuncs.mEvalAttrSpecular, me, tls, state, 0.0));
        params->mRoughness = saturate(EVALFLOATATTR(dwaBase->mAttrFuncs.mEvalAttrRoughness, me, tls, state, 0.0));

        if (dwaBase->mHints.mRequiresAnisotropyParams) {
            params->mAnisotropy = EVALFLOATATTR(dwaBase->mAttrFuncs.mEvalAttrAnisotropy, me, tls, state, 0.0);
            params->mAnisotropy = clamp(params->mAnisotropy, -1.0f, 1.0f);
            if (!isZero(params->mAnisotropy)) {
                params->mShadingTangent = EVALVEC2FATTR(dwaBase->mAttrFuncs.mEvalAttrShadingTangent, me, tls, state, Vec2f_ctor(0.0));
            }
        }
    }

    resolveIridescenceParams(dwaBase, me, tls, state, castsCaustics, &params->mIridescenceParameters);

    if (dwaBase->mHints.mRequiresMetallicParams &&
        (castsCaustics || !isCausticPath(state))) {
        if (dwaBase->mModel == Metal || dwaBase->mHints.mMetallicIsOne) {
            params->mMetallic = 1.0f;
        } else {
            params->mMetallic = saturate(EVALFLOATATTR(dwaBase->mAttrFuncs.mEvalAttrMetallic, me, tls, state, 0.0));
        }
        params->mMetallicColor = clamp(EVALCOLORATTR(dwaBase->mAttrFuncs.mEvalAttrMetallicColor, me, tls, state, Color_ctor(0.0)),0,1);
        params->mMetallicEdgeColor = clamp(EVALCOLORATTR(dwaBase->mAttrFuncs.mEvalAttrMetallicEdgeColor, me, tls, state, Color_ctor(0.0)),0,1);
    }
}

inline void
resolveTransmissionParams(const uniform DwaBase * uniform dwaBase,
                          const uniform Material *uniform me,
                          uniform ShadingTLState * uniform tls,
                          const varying State &state,
                          const uniform bool castsCaustics,
                          varying DwaBaseParameters * uniform params)
{
    if (!dwaBase->mHints.mMetallicIsOne && dwaBase->mHints.mRequiresTransmissionParams &&
        (castsCaustics || !isCausticPath(state))) {
        // Evaluate roughness here if specular params are not required
        if (!dwaBase->mHints.mRequiresSpecularParams) {
            params->mRoughness = saturate(EVALFLOATATTR(dwaBase->mAttrFuncs.mEvalAttrRoughness, me, tls, state, 0.0f));
        }
        if (dwaBase->mModel == Refractive) {
            params->mTransmission = 1.0f;
        } else {
            params->mTransmission = saturate(EVALFLOATATTR(dwaBase->mAttrFuncs.mEvalAttrTransmission, me, tls, state, 0.0f));
        }
        params->mTransmissionColor = clamp(EVALCOLORATTR(dwaBase->mAttrFuncs.mEvalAttrTransmissionColor, me, tls, state, Color_ctor(0.0f)), 0.0f, 1.0f);

        if (dwaBase->mHints.mRequiresTransmissionRefractiveIndex) {
            params->mUseIndependentTransmissionRefractiveIndex = true;
            params->mIndependentTransmissionRefractiveIndex =
                    max(sEpsilon, GETFLOATATTR(dwaBase->mAttrFuncs.mGetAttrIndependentTransmissionRefractiveIndex, me, tls, state, 1.5f));
        }

        params->mUseIndependentTransmissionRoughness =
                GETBOOLATTR(dwaBase->mAttrFuncs.mGetAttrUseIndependentTransmissionRoughness, me, false);
        if (params->mUseIndependentTransmissionRoughness) {
            params->mIndependentTransmissionRoughness =
                    saturate(EVALFLOATATTR(dwaBase->mAttrFuncs.mEvalAttrIndependentTransmissionRoughness,
                                           me, tls, state, 0.0f));
        } else {
            params->mIndependentTransmissionRoughness = params->mRoughness;
        }
        if (GETBOOLATTR(dwaBase->mAttrFuncs.mGetAttrUseDispersion, me, false)) {
            params->mDispersionAbbeNumber = GETFLOATATTR(dwaBase->mAttrFuncs.mGetAttrDispersionAbbeNumber,
                                                         me,
                                                         tls,
                                                         state,
                                                         0.0f);
            params->mDispersionAbbeNumber = max(0.0f, params->mDispersionAbbeNumber);
        } else {
            // Disable Dispersion
            params->mDispersionAbbeNumber = 0.0f;
        }
    }
}

inline void
resolveAccentParams(const uniform DwaBase * uniform dwaBase,
                    const uniform Material *uniform me,
                    uniform ShadingTLState * uniform tls,
                    const varying State &state,
                    const uniform bool castsCaustics,
                    varying DwaBaseParameters * uniform params)
{
    params->mAccentParams.mSubsurface = 1.0f;
    params->mAccentParams.mSubsurfaceNormal = params->mDiffuseNormal;
    params->mAccentParams.mSubsurfaceNormalDial = params->mNormalDial;
    params->mAccentParams.mSubsurfaceColor = params->mAlbedo;
}

inline void
resolveFabricSpecularParams(const uniform DwaBase * uniform dwaBase,
                            const uniform Material *uniform me,
                            uniform ShadingTLState * uniform tls,
                            const varying State &state,
                            const uniform bool castsCaustics,
                            varying DwaBaseParameters * uniform params)
{
    if (!dwaBase->mHints.mMetallicIsOne && !dwaBase->mHints.mTransmissionIsOne &&
        dwaBase->mHints.mRequiresFabricParams) {

        if (dwaBase->mHints.mRequiresFabricParams) {
            params->mFabricSpecular = (dwaBase->mModel == Fabric) ? 1.0f : 0.0f;
        }

        // For fabric, the specular lobes are effectively scaled
        // down as "fabric diffuse scattering" is increased.  This is somewhat
        // opposite how the other shading models work where the diffuse energy
        // is determined by a OneMinusFresnel.  Here, we handle this energy
        // distribution now as we resolve the parameters. At lobe creation time
        // the diffuse is then scaled by 1-fabricSpecular.
        const float fabricDiffuseScattering = saturate(EVALFLOATATTR(dwaBase->mAttrFuncs.mEvalAttrFabricDiffuseScattering, me, tls, state, 0.0));
        params->mFabricSpecular *= (1.0f - fabricDiffuseScattering);

        if (!castsCaustics && isCausticPath(state)) {
            // If this is a caustic path and we aren't casting caustics, we can
            // reset these values and exit early, now that we have computed
            params->mFabricAttenuation = (1.0f - params->mFabricSpecular);
            return;
        }

        // Common parameters between Fabric and Velvet.
        params->mWarpColor = clamp(EVALCOLORATTR(dwaBase->mAttrFuncs.mEvalAttrWarpColor, me, tls, state, Color_ctor(0.0)),0,1);
        params->mWarpRoughness = saturate(EVALFLOATATTR(dwaBase->mAttrFuncs.mEvalAttrWarpRoughness, me, tls, state, 0.0));

        if (dwaBase->mHints.mRequiresFabricParams) {    // Not velvet
            // Clamp the thread elevation to slightly less than 90 degrees
            params->mWarpThreadElevation = clamp(EVALFLOATATTR(dwaBase->mAttrFuncs.mEvalAttrWarpThreadElevation, me, tls, state, 0.0), -89.0f, 89.0f);
            params->mUseIndependentWeftAttributes = GETBOOLATTR(dwaBase->mAttrFuncs.mGetAttrUseIndependentWeftAttributes, me, false);

            if (params->mUseIndependentWeftAttributes) {
                params->mWeftColor = clamp(EVALCOLORATTR(dwaBase->mAttrFuncs.mEvalAttrWeftColor, me, tls, state, Color_ctor(0.0)),0,1);
                params->mWeftRoughness = saturate(EVALFLOATATTR(dwaBase->mAttrFuncs.mEvalAttrWeftRoughness, me, tls, state, 0.0));
            } else {
                params->mWeftColor = params->mWarpColor;
                params->mWeftRoughness = params->mWarpRoughness;
            }

            const bool useUVs = GETBOOLATTR(dwaBase->mAttrFuncs.mGetAttrUseUVsForThreadDirection,
                                            me,
                                            true);
            if (useUVs) {
                // Use dPds as the tangent for Fabric ReferenceFrame
                params->mFabricTangent = normalize(getdPds(state));
            } else {
                // For fabrics like satin, silk, or patterned cloth such as plaid, stripes, etc. the UV coordinates
                // should be carefully arranged like real fabric patterns (ie. a template from which the parts of
                // a garment are traced onto fabric before being cut out and assembled.).  This ensures that
                // any pattern and thread directions are consistently aligned and look pleasing across the seams
                // as the parts are sewn together.
                // 
                // We provide this alternative for computing the shading tangent for when the model's UVs are
                // not well arranged. We compute a shading tangent on-the-fly that is based on a render-space
                // X vector (1 0 0).  This provides a consistent shading tangent across the entire frame to base
                // the thread direction from, although the result is that final direction varies based on the view
                // direction.  For some materials, such as satin, this somewhat arbitrary direction may not be
                // noticeable to the viewer.
                params->mFabricTangent = Vec3f_ctor(1.0f, 0.0f, 0.0f);
            }
            params->mWarpThreadDirection = normalize(EVALVEC3FATTR(dwaBase->mAttrFuncs.mEvalAttrWarpThreadDirection, me, tls, state, Vec3f_ctor(0.0)));
            params->mWarpThreadCoverage = saturate(GETFLOATATTR(dwaBase->mAttrFuncs.mGetAttrWarpThreadCoverage, me, tls, state, 0.0));

        }
    }
}

/// Evaluates all the necessary attributes for DwaBase Materials
/*
 * From DwaBaseLayerable.isph
 * #define DWABASELAYERABLE_RESOLVE_SUBSURFACE_FUNC_PARAMS              \
    const uniform Material* uniform me,                     \
    const varying State& state
 */
extern uniform int
DWABASE_resolveSubsurfaceType(DWABASELAYERABLE_RESOLVE_SUBSURFACE_FUNC_ARGS)
{
    const uniform DwaBase * uniform dwaBase =
            (const uniform DwaBase* uniform)getDwaBaseMaterialStruct(me);

    return dwaBase->mUParams.mSubsurface;
}

/// Evaluates all the necessary attributes for DwaBase Materials
/*
 * From DwaBaseLayerable.isph
 * #define DWABASELAYERABLE_RESOLVE_PARAMS_FUNC_PARAMS              \
    const uniform Material* uniform me,                     \
    uniform ShadingTLState *uniform tls,                    \
    const varying State& state                              \
    const uniform bool castsCaustics,                       \
    varying DwaBaseParameters* uniform params
 */
extern bool
DWABASE_resolveParameters(DWABASELAYERABLE_RESOLVE_PARAMS_FUNC_ARGS)
{
    const uniform DwaBase * uniform dwaBase =
            (const uniform DwaBase* uniform)getDwaBaseMaterialStruct(me);

    // make sure all fields are initialized
    DWABASELAYERABLE_initParameters(params);

    // We want to avoid evaluating any attributes which are not ultimately needed,
    // especially in the context of indirect or caustic light paths.  We use the
    // 'hints' that were computed in update() to efficiently skip evaluation of
    // unneeded attributes.

    resolveNormalParams(dwaBase, me, tls, state, params);


    params->mRefractiveIndex = max(sEpsilon,
        GETFLOATATTR(dwaBase->mAttrFuncs.mGetAttrRefractiveIndex, me, tls, state, 1.5f));

    resolveSpecularParams(dwaBase, me, tls, state, castsCaustics, params);
    resolveHairParams(dwaBase, me, tls, state, castsCaustics, &params->mHairParameters);
    resolveToonParams(dwaBase, me, tls, state, params);
    resolveHairToonParams(dwaBase, me, tls, state, params);
    DWABASE_resolveGlitterParams(dwaBase, me, tls, state, castsCaustics, params);
    resolveFuzzParams(dwaBase, me, tls, state, castsCaustics, params);
    resolveOuterSpecularParams(dwaBase, me, tls, state, castsCaustics, params);
    resolveFabricSpecularParams(dwaBase, me, tls, state, castsCaustics, params);
    resolveTransmissionParams(dwaBase, me, tls, state, castsCaustics, params);

    if (dwaBase->mHints.mRequiresDiffuseParams) {
        params->mAlbedo =
                clamp(EVALCOLORATTR(dwaBase->mAttrFuncs.mEvalAttrAlbedo, me, tls, state, Color_ctor(0.0f)),
                      0.0f, 1.0f);
    }

    if (dwaBase->mHints.mRequiresDiffuseParams || dwaBase->mHints.mRequiresToonDiffuseParams) {
        params->mDiffuseRoughness =
            saturate(EVALFLOATATTR(dwaBase->mAttrFuncs.mEvalAttrDiffuseRoughness, me, tls, state, 0.0f));
    }

    if ((dwaBase->mHints.mRequiresSubsurfaceParams || dwaBase->mHints.mRequiresHairDiffuseParams)
        && isSubsurfaceAllowed(state)) {
        params->mScatteringRadius =
            clamp(EVALCOLORATTR(dwaBase->mAttrFuncs.mEvalAttrScatteringColor, me, tls, state, Color_ctor(0.0f)),
                  0.0f, 1.0f) *
            max(0.0f,
                EVALFLOATATTR(dwaBase->mAttrFuncs.mEvalAttrScatteringRadius, me, tls, state, 0.0f));
        params->mCreaseAttenuation =
            max(0.0f,
                EVALFLOATATTR(dwaBase->mAttrFuncs.mEvalAttrCreaseAttenuation, me, tls, state, 0.0f));
        params->mSSSResolveSelfIntersections =
                GETBOOLATTR(dwaBase->mAttrFuncs.mGetAttrSSSResolveSelfIntersections, me, true);
    }

    // In the block above, isSubsurfaceAllowed(state) is variable based on whether state is a primary
    // ray or secondary ray. So some lanes of the bsdf may have bssrdfs and some may have
    // lambertian lobes, not based on material properties, but instead based on the ray.
    // mSubsurfaceTraceSet is uniform for a material, so we must always get it, even if
    // isSubsurfaceAllowed(state) evaluates to false for some lanes.
    params->mSubsurfaceTraceSet = dwaBase->mSubsurfaceTraceSet;

    if (dwaBase->mHints.mRequiresDiffuseTransmissionParams) {
        const float diffuseTransmission =
            clamp(EVALFLOATATTR(dwaBase->mAttrFuncs.mEvalAttrDiffuseTransmission, me, tls, state, 1.0f),
                  0.0f, 1.0f);
        params->mDiffuseTransmission = diffuseTransmission *
            clamp(EVALCOLORATTR(dwaBase->mAttrFuncs.mEvalAttrDiffuseTransmissionColor, me, tls, state, Color_ctor(0.0)),0,1);
        params->mDiffuseTransmissionBlendingBehavior = GETINTATTR(dwaBase->mAttrFuncs.mGetAttrDiffuseTransmissionBlendingBehavior, me, 1);
    }

    if (dwaBase->mHints.mRequiresEmissionParams) {
        params->mEmission = max(sBlack, EVALCOLORATTR(dwaBase->mAttrFuncs.mEvalAttrEmission, me, tls, state, Color_ctor(0.0)));
    }

    resolveAccentParams(dwaBase, me, tls, state, castsCaustics, params);

    DWABASE_applyColorCorrectParameters(tls, state, params);

    params->mDiffuseLightSet = dwaBase->mDiffuseLightSet;
    params->mSpecularLightSet = dwaBase->mSpecularLightSet;

    // ---------------
    // for debug_pixel
    // ---------------
    //DWABASELAYERABLE_printParameters(params);

    return true;
}

/// Determines whether to evaluate specular lobes
extern uniform bool
DWABASE_castsCaustics(const uniform Material *uniform me)
{
    const uniform DwaBase * uniform dwaBase =
            (const uniform DwaBase* uniform)getDwaBaseMaterialStruct(me);

    uniform bool attrCastsCaustics = GETBOOLATTR(dwaBase->mAttrFuncs.mGetAttrCastsCaustics, me, false);
    return attrCastsCaustics;
}

extern float
DWABASE_resolvePresence(const uniform Material* uniform me,
                        uniform ShadingTLState *uniform tls,
                        const varying State& state)
{
    const uniform DwaBase * uniform dwaBase =
            (const uniform DwaBase* uniform)getDwaBaseMaterialStruct(me);
    return saturate(EVALFLOATATTR(dwaBase->mAttrFuncs.mEvalAttrPresence, me, tls, state, 1.0));
}

extern uniform bool
DWABASE_resolvePreventLightCulling(const uniform Material* uniform me,
                                   uniform ShadingTLState *uniform tls,
                                   const varying State& state)
{
    const uniform DwaBase * uniform dwaBase =
            (const uniform DwaBase* uniform)getDwaBaseMaterialStruct(me);
    return (uniform bool)(GETBOOLATTR(dwaBase->mAttrFuncs.mGetAttrPreventLightCulling, me, false));
}

extern Vec3f
DWABASE_resolveSubsurfaceNormal(const uniform Material* uniform me,
                                uniform ShadingTLState *uniform tls,
                                const varying State& state)
{
    const uniform DwaBase * uniform dwaBase =
            (const uniform DwaBase* uniform)getDwaBaseMaterialStruct(me);
    return EVALVEC3FATTR(dwaBase->mAttrFuncs.mEvalNormalInput, me, tls, state, state.mN);
}

// expose a way to retrieve a function pointer to the required
// ISPC functions from c++
export void * uniform DwaBase_getCastsCausticsFunc()
{
    return DWABASE_castsCaustics;
}

export void * uniform DwaBase_getResolveSubsurfaceTypeFunc()
{
    return DWABASE_resolveSubsurfaceType;
}

export void * uniform DwaBase_getResolveParametersFunc()
{
    return DWABASE_resolveParameters;
}

export void * uniform DwaBase_getResolvePresenceFunc()
{
    return DWABASE_resolvePresence;
}

export void * uniform DwaBase_getResolvePreventLightCullingFunc()
{
    return DWABASE_resolvePreventLightCulling;
}

export void * uniform DwaBase_getresolveSubsurfaceNormalFunc()
{
    return DWABASE_resolveSubsurfaceNormal;
}


