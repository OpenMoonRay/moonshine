// Copyright 2023-2024 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

///
/// @file Blending.cc
/// $Id$
///

#include "Blending.h"

using namespace scene_rdl2::math;
using namespace moonray::shading;


namespace moonshine {

namespace {
// Helper function to evaluate bssrdf normal function.
scene_rdl2::math::Vec3f
evalSubsurfaceNormalIfDefined(const dwabase::DwaBaseLayerable *material,
                              TLState *tls,
                              const State& state)
{
    if (material) {
        scene_rdl2::rdl2::EvalNormalFunc evalSSNormal = material->getEvalSubsurfaceNormalFunc();
        if (evalSSNormal) {
            return evalSSNormal(material, tls, state);
        }
    }
    return state.getN();
}

// Parameter Blending functions
// ----------------------------------------------------------------
// These functions are responsible for blending the various
// Base material parameters. The blending has been split into
// separate functions for improved readability. Each function
// sparsely initializes the subset of members of the passed
// ispc::DwaBaseParameters 'params' structure that are related
// to a specific component or feature.
//
// Care must be taken to avoid blending certain params of two
// sub-materials - namely those that are related to a disabled
// feature or component of the material.
//
// For example:
// When material 1 uses the metallic model but material 0 does
// not -- we should not blend the 'metallic color' attrs together.
// We should instead just use material 1's metallic colors directly.
// This is because it is likely that material 0's metallic color
// attrs were not explicity set by the user as they are ignored
// by material 0.
//
// In the cases where neither sub-material is using the component/
// feature, no blending is performed.  The other attrs associated with
// that component/feature are not evaluated and the fields in the
// DwaBaseParams struct are left uninitialized.
// ----------------------------------------------------------------

scene_rdl2::math::Color
colorSpaceLerp(const scene_rdl2::math::Color& color0,
               const scene_rdl2::math::Color& color1,
               const float mask,
               const ispc::BlendColorSpace colorSpace)
{
    switch (colorSpace) {
    case ispc::COLOR_BLEND_RGB:
        return lerp(color0, color1, mask);
    case ispc::COLOR_BLEND_HSV:
        return scene_rdl2::math::hsvToRgb(lerp(scene_rdl2::math::rgbToHsv(color0),
                                   scene_rdl2::math::rgbToHsv(color1),
                                   mask));
    case ispc::COLOR_BLEND_HSL:
        return scene_rdl2::math::hslToRgb(lerp(scene_rdl2::math::rgbToHsl(color0),
                                   scene_rdl2::math::rgbToHsl(color1),
                                   mask));
    default:
        return lerp(color0, color1, mask);
    }
}

void blendFuzzParams(const ispc::BlendColorSpace colorSpace,
                     const float mask,
                     const ispc::DwaBaseParameters &params0,
                     const ispc::DwaBaseParameters &params1,
                     ispc::DwaBaseParameters &params)
{
    params.mFuzz = lerp(params0.mFuzz,
                        params1.mFuzz,
                        mask);
    if (!isZero(params.mFuzz)) {
        if (!isZero(params0.mFuzz) &&
            !isZero(params1.mFuzz)) {

            params.mFuzzRoughness = lerp(params0.mFuzzRoughness,
                                         params1.mFuzzRoughness,
                                         mask);
            asCpp(params.mFuzzAlbedo) = colorSpaceLerp(asCpp(params0.mFuzzAlbedo),
                                                       asCpp(params1.mFuzzAlbedo),
                                                       mask,
                                                       colorSpace);
            asCpp(params.mFuzzNormal) = safeNormalize(lerp(asCpp(params0.mFuzzNormal),
                                                           asCpp(params1.mFuzzNormal),
                                                           mask));
            params.mFuzzUseAbsorbingFibers = (params0.mFuzzUseAbsorbingFibers ||
                                              params1.mFuzzUseAbsorbingFibers);
        } else if (!isZero(params1.mFuzz)) {
            params.mFuzzRoughness = params1.mFuzzRoughness;
            asCpp(params.mFuzzNormal) = asCpp(params1.mFuzzNormal);
            asCpp(params.mFuzzAlbedo) = asCpp(params1.mFuzzAlbedo);
            params.mFuzzUseAbsorbingFibers = params1.mFuzzUseAbsorbingFibers;
        } else { // params0.mFuzz != 0.0f
            params.mFuzzRoughness = params0.mFuzzRoughness;
            asCpp(params.mFuzzNormal) = asCpp(params0.mFuzzNormal);
            asCpp(params.mFuzzAlbedo) = asCpp(params0.mFuzzAlbedo);
            params.mFuzzUseAbsorbingFibers = params0.mFuzzUseAbsorbingFibers;
        }
    }
}

void
copyOuterSpecularParams(const ispc::DwaBaseParameters& src,
                              ispc::DwaBaseParameters& dst)
{
    dst.mOuterSpecularRefractiveIndex = src.mOuterSpecularRefractiveIndex;
    dst.mOuterSpecularRoughness = src.mOuterSpecularRoughness;
    dst.mOuterSpecularThickness = src.mOuterSpecularThickness;
    asCpp(dst.mOuterSpecularAttenuationColor) = asCpp(src.mOuterSpecularAttenuationColor);
    asCpp(dst.mOuterSpecularNormal) = asCpp(src.mOuterSpecularNormal);
    dst.mOuterSpecularNormalLength = src.mOuterSpecularNormalLength;
}

void blendOuterSpecularParams(const ispc::BlendColorSpace colorSpace,
                              const float mask,
                              const ispc::DwaBaseParameters &params0,
                              const ispc::DwaBaseParameters &params1,
                              ispc::DwaBaseParameters &params)
{
    params.mOuterSpecular = lerp(params0.mOuterSpecular,
                                 params1.mOuterSpecular,
                                 mask);
    if (!isZero(params.mOuterSpecular)) {
        if (!isZero(params0.mOuterSpecular) &&
            !isZero(params1.mOuterSpecular)) {
            params.mOuterSpecularRefractiveIndex = lerp(params0.mOuterSpecularRefractiveIndex,
                                                        params1.mOuterSpecularRefractiveIndex,
                                                        mask);
            params.mOuterSpecularRoughness = lerp(params0.mOuterSpecularRoughness,
                                                  params1.mOuterSpecularRoughness,
                                                  mask);
            params.mOuterSpecularThickness = lerp(params0.mOuterSpecularThickness,
                                                  params1.mOuterSpecularThickness,
                                                  mask);
            asCpp(params.mOuterSpecularAttenuationColor) = colorSpaceLerp(asCpp(params0.mOuterSpecularAttenuationColor),
                                                                          asCpp(params1.mOuterSpecularAttenuationColor),
                                                                          mask,
                                                                          colorSpace);
            asCpp(params.mOuterSpecularNormal) = safeNormalize(lerp(asCpp(params0.mOuterSpecularNormal),
                                                                    asCpp(params1.mOuterSpecularNormal),
                                                                    mask));
            params.mOuterSpecularNormalLength = lerp(params0.mOuterSpecularNormalLength,
                                                     params1.mOuterSpecularNormalLength,
                                                     mask);
        } else if (!isZero(params1.mOuterSpecular)) {
            copyOuterSpecularParams(params1, params);
        } else { // params0.mOuterSpecular != 0.0f
            copyOuterSpecularParams(params0, params);
        }
    }
}

void
copyGlitterFlakeParams(const ispc::GLITTER_VaryingParameters& src,
                             ispc::GLITTER_VaryingParameters& dst)
{
    // flake style A
    dst.mFlakeStyleFrequency[0] = src.mFlakeStyleFrequency[0];
    asCpp(dst.mFlakeColor[0]) = asCpp(src.mFlakeColor[0]);
    dst.mFlakeSize[0] = src.mFlakeSize[0];
    dst.mFlakeRoughness[0] = src.mFlakeRoughness[0];

    // flake style B
    dst.mFlakeStyleFrequency[1] = src.mFlakeStyleFrequency[1];
    asCpp(dst.mFlakeColor[1]) = asCpp(src.mFlakeColor[1]);
    dst.mFlakeSize[1] = src.mFlakeSize[1];
    dst.mFlakeRoughness[1] = src.mFlakeRoughness[1];

    asCpp(dst.mFlakeHSVColorVariation) = asCpp(src.mFlakeHSVColorVariation);
    dst.mFlakeDensity = src.mFlakeDensity;
    dst.mFlakeJitter = src.mFlakeJitter;
    dst.mFlakeOrientationRandomness = src.mFlakeOrientationRandomness;
    dst.mCompensateDeformation = src.mCompensateDeformation;
    dst.mApproximateForSecRays = src.mApproximateForSecRays;
}

void blendGlitterParams(const ispc::BlendColorSpace colorSpace,
                        const float mask,
                        const moonshine::glitter::Glitter* layerMaterialGlitterPointer,
                        const ispc::DwaBaseParameters &params0,
                        const ispc::DwaBaseParameters &params1,
                        ispc::DwaBaseParameters &params)
{
    // Assign some variables to make the code below shorter and more readable
    const ispc::GLITTER_VaryingParameters& vParams0 = params0.mGlitterVaryingParameters;
    const ispc::GLITTER_VaryingParameters& vParams1 = params1.mGlitterVaryingParameters;
    ispc::GLITTER_VaryingParameters& vParams = params.mGlitterVaryingParameters;

    vParams.mGlitterMask = lerp(vParams0.mGlitterMask,
                                vParams1.mGlitterMask,
                                mask);
    if (isZero(vParams.mGlitterMask)) return;

    if (!isZero(vParams0.mGlitterMask) &&
        !isZero(vParams1.mGlitterMask)) {

        if (!layerMaterialGlitterPointer) return;
        params.mGlitterPointerScalar = (intptr_t)layerMaterialGlitterPointer;
        params.mGlitterUniformParameters = layerMaterialGlitterPointer->getIspcUniformParameters();

        vParams.mFlakeStyleFrequency[0] = lerp(vParams0.mFlakeStyleFrequency[0],
                                               vParams1.mFlakeStyleFrequency[0],
                                               mask);
        asCpp(vParams.mFlakeColor[0]) = colorSpaceLerp(asCpp(vParams0.mFlakeColor[0]),
                                                       asCpp(vParams1.mFlakeColor[0]),
                                                       mask,
                                                       colorSpace);
        vParams.mFlakeSize[0] = lerp(vParams0.mFlakeSize[0],
                                     vParams1.mFlakeSize[0],
                                     mask);
        vParams.mFlakeRoughness[0] = lerp(vParams0.mFlakeRoughness[0],
                                          vParams1.mFlakeRoughness[0],
                                          mask);

        vParams.mFlakeStyleFrequency[1] = lerp(vParams0.mFlakeStyleFrequency[1],
                                               vParams1.mFlakeStyleFrequency[1],
                                               mask);
        asCpp(vParams.mFlakeColor[1]) = colorSpaceLerp(asCpp(vParams0.mFlakeColor[1]),
                                                       asCpp(vParams1.mFlakeColor[1]),
                                                       mask,
                                                       colorSpace);
        vParams.mFlakeSize[1] = lerp(vParams0.mFlakeSize[1],
                                     vParams1.mFlakeSize[1],
                                     mask);
        vParams.mFlakeRoughness[1] = lerp(vParams0.mFlakeRoughness[1],
                                          vParams1.mFlakeRoughness[1],
                                          mask);

        asCpp(vParams.mFlakeHSVColorVariation) = lerp(asCpp(vParams0.mFlakeHSVColorVariation),
                                                      asCpp(vParams1.mFlakeHSVColorVariation),
                                                      mask);
        vParams.mFlakeDensity = lerp(vParams0.mFlakeDensity,
                                     vParams1.mFlakeDensity,
                                     mask);
        vParams.mFlakeJitter = lerp(vParams0.mFlakeJitter,
                                    vParams1.mFlakeJitter,
                                    mask);
        vParams.mFlakeOrientationRandomness = lerp(vParams0.mFlakeOrientationRandomness,
                                                   vParams1.mFlakeOrientationRandomness,
                                                   mask);
        vParams.mCompensateDeformation = vParams0.mCompensateDeformation ||
                                         vParams1.mCompensateDeformation;
        vParams.mApproximateForSecRays = vParams0.mApproximateForSecRays ||
                                         vParams1.mApproximateForSecRays;
    } else if (!isZero(vParams0.mGlitterMask)) {
        if (!params0.mGlitterPointerScalar) return;
        params.mGlitterPointerScalar = params0.mGlitterPointerScalar;
        params.mGlitterUniformParameters = params0.mGlitterUniformParameters;
        copyGlitterFlakeParams(vParams0, vParams);
    } else {
        if (!params1.mGlitterPointerScalar) return;
        params.mGlitterPointerScalar = params1.mGlitterPointerScalar;
        params.mGlitterUniformParameters = params1.mGlitterUniformParameters;
        copyGlitterFlakeParams(vParams1, vParams);
    }
}

void blendCommonSpecularParams(const float mask,
                               const ispc::DwaBaseParameters &params0,
                               const ispc::DwaBaseParameters &params1,
                               ispc::DwaBaseParameters &params)
{
    params.mSpecular = lerp(params0.mSpecular, params1.mSpecular, mask);
    if (!isZero(params.mSpecular)) {
        if (!isZero(params0.mSpecular) &&
            !isZero(params1.mSpecular)) {
            params.mRoughness = lerp(params0.mRoughness,
                                     params1.mRoughness,
                                     mask);
            if (!isZero(params0.mAnisotropy) &&
                !isZero(params1.mAnisotropy)) {
                params.mAnisotropy = lerp(params0.mAnisotropy,
                                          params1.mAnisotropy,
                                          mask);
                // TODO: lerping between two tangent vectors may not be what we want
                asCpp(params.mShadingTangent) = safeNormalize(lerp(asCpp(params0.mShadingTangent),
                                                                   asCpp(params1.mShadingTangent),
                                                                   mask));
            } else if (!isZero(params1.mAnisotropy)) {
                params.mAnisotropy = params1.mAnisotropy * mask;
                asCpp(params.mShadingTangent) = asCpp(params1.mShadingTangent);
            } else { // !isZero(params0.mAnisotropy)
                params.mAnisotropy = params0.mAnisotropy * (1.0f - mask);
                asCpp(params.mShadingTangent) = asCpp(params0.mShadingTangent);
            }
        } else if (!isZero(params1.mSpecular)) {
            params.mRoughness = params1.mRoughness;
            params.mAnisotropy = params1.mAnisotropy;
            asCpp(params.mShadingTangent) = asCpp(params1.mShadingTangent);
        } else { // !isZero(params0.mSpecular)
            params.mRoughness = params0.mRoughness;
            params.mAnisotropy = params0.mAnisotropy;
            asCpp(params.mShadingTangent) = asCpp(params0.mShadingTangent);
        }
    }
}

void
copyIridescenceParams(const ispc::IridescenceParameters& src,
                            ispc::IridescenceParameters& dst)
{
    dst.mIridescenceApplyTo = src.mIridescenceApplyTo;
    dst.mIridescenceColorControl = src.mIridescenceColorControl;
    asCpp(dst.mIridescencePrimaryColor) = asCpp(src.mIridescencePrimaryColor);
    asCpp(dst.mIridescenceSecondaryColor) = asCpp(src.mIridescenceSecondaryColor);
    dst.mIridescenceFlipHueDirection = src.mIridescenceFlipHueDirection;
    dst.mIridescenceThickness = src.mIridescenceThickness;
    dst.mIridescenceExponent = src.mIridescenceExponent;
    dst.mIridescenceAt0 = src.mIridescenceAt0;
    dst.mIridescenceAt90 = src.mIridescenceAt90;

    // ramp attrs
    dst.mIridescenceRampInterpolationMode = src.mIridescenceRampInterpolationMode;
    dst.mIridescenceRampNumPoints = src.mIridescenceRampNumPoints;
    for (int i = 0; i < src.mIridescenceRampNumPoints; ++i) {
        dst.mIridescenceRampPositions[i] = src.mIridescenceRampPositions[i];
        dst.mIridescenceRampColors[i] = src.mIridescenceRampColors[i];
        dst.mIridescenceRampInterpolators[i] = src.mIridescenceRampInterpolators[i];
    }
}

void
blendIridescenceParams(const ispc::BlendColorSpace colorSpace,
                       const float mask,
                       const ispc::DwaBaseParameters &params0,
                       const ispc::DwaBaseParameters &params1,
                       ispc::DwaBaseParameters &params)
{
    ispc::IridescenceParameters& iridescenceParams =  params.mIridescenceParameters;
    const ispc::IridescenceParameters& iridescenceParams0 = params0.mIridescenceParameters;
    const ispc::IridescenceParameters& iridescenceParams1 = params1.mIridescenceParameters;

    if (!isZero(params.mSpecular)) {
        iridescenceParams.mIridescence = lerp(iridescenceParams0.mIridescence,
                                              iridescenceParams1.mIridescence,
                                              mask);
        if (!isZero(iridescenceParams.mIridescence)) {
            if (!isZero(iridescenceParams0.mIridescence) &&
                !isZero(iridescenceParams1.mIridescence)) {

                if (iridescenceParams0.mIridescenceApplyTo != iridescenceParams1.mIridescenceApplyTo) {
                    // values are mixed: Apply to clearcoat/outer specular wins
                    iridescenceParams.mIridescenceApplyTo = ispc::IRIDESCENCE_OUTER_SPECULAR;
                } else {
                    iridescenceParams.mIridescenceApplyTo = iridescenceParams1.mIridescenceApplyTo;
                }
                if (iridescenceParams0.mIridescenceColorControl != iridescenceParams1.mIridescenceColorControl) {
                    // values are mixed: Ramp wins
                    iridescenceParams.mIridescenceColorControl = ispc::SHADING_IRIDESCENCE_COLOR_USE_RAMP;
                } else {
                    iridescenceParams.mIridescenceColorControl = iridescenceParams1.mIridescenceColorControl;
                }
                asCpp(iridescenceParams.mIridescencePrimaryColor) =
                        colorSpaceLerp(asCpp(iridescenceParams0.mIridescencePrimaryColor),
                                       asCpp(iridescenceParams1.mIridescencePrimaryColor),
                                       mask,
                                       colorSpace);
                asCpp(iridescenceParams.mIridescenceSecondaryColor) =
                        colorSpaceLerp(asCpp(iridescenceParams0.mIridescenceSecondaryColor),
                                       asCpp(iridescenceParams1.mIridescenceSecondaryColor),
                                       mask,
                                       colorSpace);
                iridescenceParams.mIridescenceFlipHueDirection =
                        (mask >= 0.5f) ? iridescenceParams1.mIridescenceFlipHueDirection :
                                         iridescenceParams0.mIridescenceFlipHueDirection;
                iridescenceParams.mIridescenceThickness = lerp(iridescenceParams0.mIridescenceThickness,
                                                               iridescenceParams1.mIridescenceThickness,
                                                               mask);
                iridescenceParams.mIridescenceExponent = lerp(iridescenceParams0.mIridescenceExponent,
                                                              iridescenceParams1.mIridescenceExponent,
                                                              mask);
                iridescenceParams.mIridescenceAt0 = lerp(iridescenceParams0.mIridescenceAt0,
                                                         iridescenceParams1.mIridescenceAt0,
                                                         mask);
                iridescenceParams.mIridescenceAt90 = lerp(iridescenceParams0.mIridescenceAt90,
                                                          iridescenceParams1.mIridescenceAt90,
                                                          mask);

                // ramp attrs
                if (iridescenceParams0.mIridescenceRampInterpolationMode !=
                    iridescenceParams1.mIridescenceRampInterpolationMode) {
                    // values are mixed: HSV wins
                    iridescenceParams.mIridescenceRampInterpolationMode = ispc::COLOR_RAMP_CONTROL_SPACE_HSV;
                } else {
                    iridescenceParams.mIridescenceRampInterpolationMode =
                            iridescenceParams1.mIridescenceRampInterpolationMode;
                }
                iridescenceParams.mIridescenceRampNumPoints = max(iridescenceParams0.mIridescenceRampNumPoints,
                                                                  iridescenceParams1.mIridescenceRampNumPoints);
                const int blendAttrsCount = min(iridescenceParams0.mIridescenceRampNumPoints,
                                                iridescenceParams1.mIridescenceRampNumPoints);
                for (int i = 0; i < blendAttrsCount; ++i) {
                    iridescenceParams.mIridescenceRampPositions[i] = lerp(iridescenceParams0.mIridescenceRampPositions[i],
                                                                          iridescenceParams1.mIridescenceRampPositions[i],
                                                                          mask);
                    asCpp(iridescenceParams.mIridescenceRampColors[i]) =
                            lerp(asCpp(iridescenceParams0.mIridescenceRampColors[i]),
                                 asCpp(iridescenceParams1.mIridescenceRampColors[i]),
                                       mask);
                    // Always use the material being layered's interpolator?
                    iridescenceParams.mIridescenceRampInterpolators[i] =
                            iridescenceParams1.mIridescenceRampInterpolators[i];
                }

                if (iridescenceParams0.mIridescenceRampNumPoints > iridescenceParams1.mIridescenceRampNumPoints) {
                    for (int i = blendAttrsCount; i < iridescenceParams.mIridescenceRampNumPoints; ++i) {
                        iridescenceParams.mIridescenceRampPositions[i] = iridescenceParams0.mIridescenceRampPositions[i];
                        iridescenceParams.mIridescenceRampColors[i] = iridescenceParams0.mIridescenceRampColors[i];
                        iridescenceParams.mIridescenceRampInterpolators[i] =
                                iridescenceParams0.mIridescenceRampInterpolators[i];
                    }
                } else {
                    for (int i = blendAttrsCount; i < iridescenceParams.mIridescenceRampNumPoints; ++i) {
                        iridescenceParams.mIridescenceRampPositions[i] = iridescenceParams1.mIridescenceRampPositions[i];
                        iridescenceParams.mIridescenceRampColors[i] = iridescenceParams1.mIridescenceRampColors[i];
                        iridescenceParams.mIridescenceRampInterpolators[i] =
                                iridescenceParams1.mIridescenceRampInterpolators[i];
                    }
                }
            } else if (!isZero(iridescenceParams0.mIridescence)) {
                copyIridescenceParams(iridescenceParams0, iridescenceParams);
            } else { // iridescenceParams1.mIridescence > 0.0f
                copyIridescenceParams(iridescenceParams1, iridescenceParams);
            }
        }
    }
}

void blendMetallicParams(const ispc::BlendColorSpace colorSpace,
                         const float mask,
                         const ispc::DwaBaseParameters &params0,
                         const ispc::DwaBaseParameters &params1,
                         ispc::DwaBaseParameters &params)
{
    if (!isZero(params.mSpecular)) {
        params.mMetallic = lerp(params0.mMetallic,
                                params1.mMetallic,
                                mask);
        if (!isZero(params.mMetallic)) {
            if (!isZero(params0.mMetallic) &&
                !isZero(params1.mMetallic)) {

                asCpp(params.mMetallicColor) = colorSpaceLerp(asCpp(params0.mMetallicColor),
                                                              asCpp(params1.mMetallicColor),
                                                              mask,
                                                              colorSpace);
                asCpp(params.mMetallicEdgeColor) = colorSpaceLerp(asCpp(params0.mMetallicEdgeColor),
                                                                  asCpp(params1.mMetallicEdgeColor),
                                                                  mask,
                                                                  colorSpace);
            } else if (!isZero(params0.mMetallic)) {
                asCpp(params.mMetallicColor) = asCpp(params0.mMetallicColor);
                asCpp(params.mMetallicEdgeColor) = asCpp(params0.mMetallicEdgeColor);
            } else { // params1.mMetallic > 0.0f
                asCpp(params.mMetallicColor) = asCpp(params1.mMetallicColor);
                asCpp(params.mMetallicEdgeColor) = asCpp(params1.mMetallicEdgeColor);
            }
        }
    }
}

void copyFabricParams(const ispc::DwaBaseParameters& src,
                            ispc::DwaBaseParameters& dst)
{
    asCpp(dst.mWarpColor) = asCpp(src.mWarpColor);
    dst.mWarpRoughness = src.mWarpRoughness;
    asCpp(dst.mFabricTangent) = asCpp(src.mFabricTangent);
    asCpp(dst.mWeftColor) = asCpp(src.mWeftColor);
    dst.mWeftRoughness = src.mWeftRoughness;
    asCpp(dst.mWarpThreadDirection) = asCpp(src.mWarpThreadDirection);
    dst.mWarpThreadElevation = src.mWarpThreadElevation;
    dst.mWarpThreadCoverage = src.mWarpThreadCoverage;
}

void blendFabricParams(const ispc::BlendColorSpace colorSpace,
                       const float mask,
                       const ispc::DwaBaseParameters &params0,
                       const ispc::DwaBaseParameters &params1,
                       ispc::DwaBaseParameters &params)
{
    if (!isOne(params.mMetallic)) {
        params.mFabricSpecular = lerp(params0.mFabricSpecular,
                                      params1.mFabricSpecular,
                                      mask);

        if (!isZero(params.mFabricSpecular)) {
            if (!isZero(params0.mFabricSpecular) &&
                !isZero(params1.mFabricSpecular)) {

                asCpp(params.mWarpColor) = colorSpaceLerp(asCpp(params0.mWarpColor),
                                                          asCpp(params1.mWarpColor),
                                                          mask,
                                                          colorSpace);
                params.mWarpRoughness = lerp(params0.mWarpRoughness,
                                             params1.mWarpRoughness,
                                             mask);
                asCpp(params.mFabricTangent) = lerp(asCpp(params0.mFabricTangent),
                                                    asCpp(params1.mFabricTangent),
                                                    mask);
                asCpp(params.mWeftColor) = colorSpaceLerp(asCpp(params0.mWeftColor),
                                                          asCpp(params1.mWeftColor),
                                                          mask,
                                                          colorSpace);
                params.mWeftRoughness = lerp(params0.mWeftRoughness,
                                             params1.mWeftRoughness,
                                             mask);
                asCpp(params.mWarpThreadDirection) = normalize(lerp(asCpp(params0.mWarpThreadDirection),
                                                                    asCpp(params1.mWarpThreadDirection),
                                                                    mask));
                params.mWarpThreadElevation = lerp(params0.mWarpThreadElevation,
                                                   params1.mWarpThreadElevation,
                                                   mask);
                params.mWarpThreadCoverage = lerp(params0.mWarpThreadCoverage,
                                                  params1.mWarpThreadCoverage,
                                                  mask);
            } else if (!isZero(params0.mFabricSpecular)) {
                copyFabricParams(params0, params);
            } else { // params1.mFabricSpecular > 0.0f
                copyFabricParams(params1, params);
            }
        }
    }
}

void blendTransmissionParams(const ispc::BlendColorSpace colorSpace,
                             const float mask,
                             const ispc::DwaBaseParameters &params0,
                             const ispc::DwaBaseParameters &params1,
                             ispc::DwaBaseParameters &params)
{
    if (!isOne(params.mMetallic) &&
        !isZero(params.mFabricAttenuation)) {

        params.mTransmission = lerp(params0.mTransmission,
                                    params1.mTransmission,
                                    mask);
        if (!isZero(params.mTransmission)) {
            if (!isZero(params0.mTransmission) &&
                !isZero(params1.mTransmission)) {

                asCpp(params.mTransmissionColor) = colorSpaceLerp(asCpp(params0.mTransmissionColor),
                                                                  asCpp(params1.mTransmissionColor),
                                                                  mask,
                                                                  colorSpace);
            } else if (!isZero(params0.mTransmission)) {
                asCpp(params.mTransmissionColor) = asCpp(params0.mTransmissionColor);
            } else { // params1.mTransmission > 0.0f
                asCpp(params.mTransmissionColor) = asCpp(params1.mTransmissionColor);
            }

            if (params0.mUseIndependentTransmissionRefractiveIndex &&
                params1.mUseIndependentTransmissionRefractiveIndex) {

                params.mUseIndependentTransmissionRefractiveIndex = true;
                params.mIndependentTransmissionRefractiveIndex = lerp(params0.mIndependentTransmissionRefractiveIndex,
                                                                      params1.mIndependentTransmissionRefractiveIndex,
                                                                      mask);
            } else if (params0.mUseIndependentTransmissionRefractiveIndex) {
                params.mUseIndependentTransmissionRefractiveIndex = true;
                if (isZero(params1.mTransmission)) {
                    // If 1 is not transmissive, ignore it's IOR
                    params.mIndependentTransmissionRefractiveIndex = params0.mIndependentTransmissionRefractiveIndex;
                } else {
                    params.mIndependentTransmissionRefractiveIndex = lerp(params0.mIndependentTransmissionRefractiveIndex,
                                                                          params1.mRefractiveIndex,
                                                                          mask);
                }
            } else if (params1.mUseIndependentTransmissionRefractiveIndex) {
                params.mUseIndependentTransmissionRefractiveIndex = true;
                if (isZero(params0.mTransmission)) {
                    // If 0 is not transmissive, ignore it's IOR
                    params.mIndependentTransmissionRefractiveIndex = params1.mIndependentTransmissionRefractiveIndex;
                } else {
                    params.mIndependentTransmissionRefractiveIndex = lerp(params0.mRefractiveIndex,
                                                                          params1.mIndependentTransmissionRefractiveIndex,
                                                                          mask);
                }
            } else {
                params.mUseIndependentTransmissionRefractiveIndex = false;
            }

            if (params0.mUseIndependentTransmissionRoughness &&
                params1.mUseIndependentTransmissionRoughness) {

                params.mUseIndependentTransmissionRoughness = true;
                params.mIndependentTransmissionRoughness = lerp(params0.mIndependentTransmissionRoughness,
                                                                params1.mIndependentTransmissionRoughness,
                                                                mask);
            } else if (params0.mUseIndependentTransmissionRoughness) {
                params.mUseIndependentTransmissionRoughness = true;
                if (isZero(params1.mTransmission)) {
                    // If 1 is not transmissive, ignore its roughness
                    params.mIndependentTransmissionRoughness = params0.mIndependentTransmissionRoughness;
                } else {
                    params.mIndependentTransmissionRoughness = lerp(params0.mIndependentTransmissionRoughness,
                                                                    params1.mRoughness,
                                                                    mask);
                }
            } else if (params1.mUseIndependentTransmissionRoughness) {
                params.mUseIndependentTransmissionRoughness = true;
                if (isZero(params0.mTransmission)) {
                    // If 0 is not transmissive, ignore its roughness
                    params.mIndependentTransmissionRoughness = params1.mIndependentTransmissionRoughness;
                } else {
                    params.mIndependentTransmissionRoughness = lerp(params0.mRoughness,
                                                                    params1.mIndependentTransmissionRoughness,
                                                                    mask);
                }
            } else {
                params.mUseIndependentTransmissionRoughness = false;
            }

            // Dispersion Layering
            if (!isZero(params0.mDispersionAbbeNumber) &&
                !isZero(params1.mDispersionAbbeNumber)) {

                params.mDispersionAbbeNumber = lerp(params0.mDispersionAbbeNumber,
                                                    params1.mDispersionAbbeNumber,
                                                    mask);
            } else if (!isZero(params0.mDispersionAbbeNumber)) {
                params.mDispersionAbbeNumber = params0.mDispersionAbbeNumber;
            } else if (!isZero(params1.mDispersionAbbeNumber)) {
                params.mDispersionAbbeNumber = params1.mDispersionAbbeNumber;
            }
        }
    }
}

void copyToonDiffuseRampParams(const ispc::ToonDiffuseParameters& src,
                               ispc::ToonDiffuseParameters& dst)
{
    dst.mRampNumPoints = src.mRampNumPoints;
    for (int i = 0; i < src.mRampNumPoints; ++i) {
        dst.mRampPositions[i] = src.mRampPositions[i];
        dst.mRampColors[i] = src.mRampColors[i];
        dst.mRampInterpolators[i] = src.mRampInterpolators[i];
    }
    dst.mExtendRamp = src.mExtendRamp;
}

void copyToonDiffuseOrenNayarParams(const ispc::ToonDiffuseParameters& src,
                                    ispc::ToonDiffuseParameters& dst)
{
    dst.mTerminatorShift = src.mTerminatorShift;
    dst.mFlatness = src.mFlatness;
    dst.mFlatnessFalloff = src.mFlatnessFalloff;
}

void copyToonDiffuseParams(const ispc::ToonDiffuseParameters& src,
                           ispc::ToonDiffuseParameters& dst)
{
    dst.mModel = src.mModel;
    dst.mNormal = src.mNormal;
    copyToonDiffuseRampParams(src, dst);
    copyToonDiffuseOrenNayarParams(src, dst);
}


void blendToonDiffuseParams(const ispc::BlendColorSpace colorSpace,
                            const float mask,
                            const ispc::ToonDiffuseParameters &params0,
                            const ispc::ToonDiffuseParameters &params1,
                            ispc::ToonDiffuseParameters &params)
{
    params.mToonDiffuse = lerp(params0.mToonDiffuse,
                               params1.mToonDiffuse,
                               mask);
    if (isZero(params.mToonDiffuse)) {
        return;
    }

    if (!isZero(params0.mToonDiffuse) &&
        !isZero(params1.mToonDiffuse)) {

        if (params0.mModel != params1.mModel) {
            // Ramp wins over non-ramp; RAMP_V2 wins over RAMP
            if (params0.mModel == ispc::TOON_DIFFUSE_RAMP_V2 || params1.mModel == ispc::TOON_DIFFUSE_RAMP_V2) {
                params.mModel = ispc::TOON_DIFFUSE_RAMP_V2;
            } else {
                params.mModel = ispc::TOON_DIFFUSE_RAMP_V1;
            }
        } else {
            params.mModel = params1.mModel;
        }

        // Blend normals
        asCpp(params.mNormal) = safeNormalize(lerp(asCpp(params0.mNormal),
                                                   asCpp(params1.mNormal),
                                                   mask));

        // Oren-nayer params
        if (params0.mModel == ispc::TOON_DIFFUSE_OREN_NAYAR &&
            (params1.mModel == ispc::TOON_DIFFUSE_RAMP_V1 || params1.mModel == ispc::TOON_DIFFUSE_RAMP_V2)) {
            copyToonDiffuseOrenNayarParams(params0, params);
        } else if ((params0.mModel == ispc::TOON_DIFFUSE_RAMP_V1 || params0.mModel == ispc::TOON_DIFFUSE_RAMP_V2) &&
                   params1.mModel == ispc::TOON_DIFFUSE_OREN_NAYAR) {
            copyToonDiffuseOrenNayarParams(params1, params);
        } else {
            params.mTerminatorShift = lerp(params0.mTerminatorShift,
                                           params1.mTerminatorShift,
                                           mask);
            params.mFlatness = lerp(params0.mFlatness,
                                    params1.mFlatness,
                                    mask);
            params.mFlatnessFalloff = lerp(params0.mFlatnessFalloff,
                                           params1.mFlatnessFalloff,
                                           mask);
        }

        // Ramp params
        params.mRampWeight = lerp(params0.mRampWeight,
                                  params1.mRampWeight,
                                  mask);
        if (params0.mModel == ispc::TOON_DIFFUSE_OREN_NAYAR &&
            (params1.mModel == ispc::TOON_DIFFUSE_RAMP_V1 || params1.mModel == ispc::TOON_DIFFUSE_RAMP_V2)) {
            copyToonDiffuseRampParams(params1, params);
        } else if ((params0.mModel == ispc::TOON_DIFFUSE_RAMP_V1 || params0.mModel == ispc::TOON_DIFFUSE_RAMP_V2) &&
                   params1.mModel == ispc::TOON_DIFFUSE_OREN_NAYAR) {
            copyToonDiffuseRampParams(params0, params);
        } else {
            params.mRampNumPoints = max(params0.mRampNumPoints,
                                        params1.mRampNumPoints);
            const int blendAttrsCount = min(params0.mRampNumPoints,
                                            params1.mRampNumPoints);
            for (int i = 0; i < blendAttrsCount; ++i) {
                params.mRampPositions[i] = lerp(params0.mRampPositions[i],
                                                params1.mRampPositions[i],
                                                mask);
                asCpp(params.mRampColors[i]) = colorSpaceLerp(asCpp(params0.mRampColors[i]),
                                                              asCpp(params1.mRampColors[i]),
                                                              mask,
                                                              colorSpace);
                // Always use the material being layered's interpolator?
                params.mRampInterpolators[i] = params1.mRampInterpolators[i];
            }

            if (params0.mRampNumPoints > params1.mRampNumPoints) {
                for (int i = blendAttrsCount; i < params.mRampNumPoints; ++i) {
                    params.mRampPositions[i] = params0.mRampPositions[i];
                    params.mRampColors[i] = params0.mRampColors[i];
                    params.mRampInterpolators[i] = params0.mRampInterpolators[i];
                }
            } else {
                for (int i = blendAttrsCount; i < params.mRampNumPoints; ++i) {
                    params.mRampPositions[i] = params1.mRampPositions[i];
                    params.mRampColors[i] = params1.mRampColors[i];
                    params.mRampInterpolators[i] = params1.mRampInterpolators[i];
                }
            }
            if (params0.mExtendRamp != params1.mExtendRamp) {
                // values are mixed: Extending the ramp wins
                params.mExtendRamp = true;
            } else {
                params.mExtendRamp = params1.mExtendRamp;
            }
        }
    } else if (!isZero(params1.mToonDiffuse)) {
        copyToonDiffuseParams(params1, params);
    } else { // params0.mToonDiffuse != 0.0f
        copyToonDiffuseParams(params0, params);
    }
}

void
copyToonSpecularParams(const ispc::ToonSpecularParameters& src,
                       ispc::ToonSpecularParameters& dst)
{
    dst.mIntensity = src.mIntensity;
    dst.mFresnelBlend = src.mFresnelBlend;
    dst.mRoughness = src.mRoughness;
    dst.mTint = src.mTint;
    dst.mRampInputScale = src.mRampInputScale;
    dst.mRampNumPoints = src.mRampNumPoints;
    for (int i = 0; i < src.mRampNumPoints; ++i) {
        dst.mRampPositions[i] = src.mRampPositions[i];
        dst.mRampValues[i] = src.mRampValues[i];
        dst.mRampInterpolators[i] = src.mRampInterpolators[i];
    }
    dst.mNormal = src.mNormal;
    dst.mStretchU = src.mStretchU;
    dst.mStretchV = src.mStretchV;
    dst.mdPds = src.mdPds;
    dst.mdPdt = src.mdPdt;
    dst.mIndirectReflectionsIntensity = src.mIndirectReflectionsIntensity;
    dst.mIndirectReflectionsRoughness = src.mIndirectReflectionsRoughness;
}

void
blendToonSpecularParams(const ispc::BlendColorSpace colorSpace,
                        const float mask,
                        const ispc::ToonSpecularParameters &params0,
                        const ispc::ToonSpecularParameters &params1,
                        ispc::ToonSpecularParameters &params)
{

    params.mToonSpecular = lerp(params0.mToonSpecular,
                                params1.mToonSpecular,
                                mask);

    if (isZero(params.mToonSpecular)) {
        return;
    }

    if (!isZero(params0.mToonSpecular) &&
        !isZero(params1.mToonSpecular)) {

        asCpp(params.mNormal) = safeNormalize(lerp(asCpp(params0.mNormal),
                                                   asCpp(params1.mNormal),
                                                   mask));

        params.mIntensity = lerp(params0.mIntensity,
                                 params1.mIntensity,
                                 mask);

        params.mFresnelBlend = lerp(params0.mFresnelBlend,
                                    params1.mFresnelBlend,
                                    mask);

        params.mRoughness = lerp(params0.mRoughness,
                                 params1.mRoughness,
                                 mask);

        asCpp(params.mTint) = colorSpaceLerp(asCpp(params0.mTint),
                                             asCpp(params1.mTint),
                                             mask, colorSpace);

        params.mRampInputScale = lerp(params0.mRampInputScale,
                                      params1.mRampInputScale,
                                      mask);

        params.mRampNumPoints = max(params0.mRampNumPoints,
                                    params1.mRampNumPoints);

        const int blendAttrsCount = min(params0.mRampNumPoints,
                                        params1.mRampNumPoints);

        for (int i = 0; i < blendAttrsCount; ++i) {
            params.mRampPositions[i] = lerp(params0.mRampPositions[i],
                                            params1.mRampPositions[i],
                                            mask);

            params.mRampValues[i] = lerp(params0.mRampValues[i],
                                         params1.mRampValues[i],
                                         mask);

            params.mRampInterpolators[i] = params1.mRampInterpolators[i];
        }

        if (params0.mRampNumPoints > params1.mRampNumPoints) {
            for (int i = blendAttrsCount; i < params.mRampNumPoints; ++i) {
                params.mRampPositions[i] = params0.mRampPositions[i];
                params.mRampValues[i] = params0.mRampValues[i];
                params.mRampInterpolators[i] = params0.mRampInterpolators[i];
            }
        } else {
            for (int i = blendAttrsCount; i < params.mRampNumPoints; ++i) {
                params.mRampPositions[i] = params1.mRampPositions[i];
                params.mRampValues[i] = params1.mRampValues[i];
                params.mRampInterpolators[i] = params1.mRampInterpolators[i];
            }
        }

        params.mStretchU = lerp(params0.mStretchU,
                                params1.mStretchU,
                                mask);

        params.mStretchV = lerp(params0.mStretchV,
                                params1.mStretchV,
                                mask);

        scene_rdl2::math::asCpp(params.mdPds) = lerp(scene_rdl2::math::asCpp(params0.mdPds),
                                         scene_rdl2::math::asCpp(params1.mdPds),
                                         mask);

        scene_rdl2::math::asCpp(params.mdPdt) = lerp(scene_rdl2::math::asCpp(params0.mdPdt),
                                         scene_rdl2::math::asCpp(params1.mdPdt),
                                         mask);

        params.mIndirectReflectionsIntensity = lerp(params0.mIndirectReflectionsIntensity,
                                                    params1.mIndirectReflectionsIntensity,
                                                    mask);

        params.mIndirectReflectionsRoughness = lerp(params0.mIndirectReflectionsRoughness,
                                                    params1.mIndirectReflectionsRoughness,
                                                    mask);

    } else if (!isZero(params1.mToonSpecular)) {
        copyToonSpecularParams(params1, params);
    } else { // params0.mToonSpecular != 0.0f
        copyToonSpecularParams(params0, params);
    }
}

void blendToonParams(const ispc::BlendColorSpace colorSpace,
                     const float mask,
                     const ispc::DwaBaseParameters &params0,
                     const ispc::DwaBaseParameters &params1,
                     ispc::DwaBaseParameters &params)
{
    blendToonDiffuseParams(colorSpace,
                           mask,
                           params0.mToonDiffuseParams,
                           params1.mToonDiffuseParams,
                           params.mToonDiffuseParams);

    blendToonSpecularParams(colorSpace,
                            mask,
                            params0.mToonSpecularParams,
                            params1.mToonSpecularParams,
                            params.mToonSpecularParams);
}

void blendDiffuseParams(const ispc::BlendColorSpace colorSpace,
                        const float mask,
                        const ispc::DwaBaseParameters &params0,
                        const ispc::DwaBaseParameters &params1,
                        ispc::DwaBaseParameters &params)
{
    const bool mtl0HasDiffuse = (!isBlack(asCpp(params0.mAlbedo)));
    const bool mtl1HasDiffuse = (!isBlack(asCpp(params1.mAlbedo)));
    const bool blocked = (isOne(params.mMetallic) ||
                          isZero(params.mFabricAttenuation) ||
                          isOne(params.mTransmission));

    if (!blocked && (mtl0HasDiffuse || mtl1HasDiffuse)) {
        asCpp(params.mAlbedo) = colorSpaceLerp(asCpp(params0.mAlbedo),
                                               asCpp(params1.mAlbedo),
                                               mask,
                                               colorSpace);
        asCpp(params.mDiffuseTransmission) = lerp(asCpp(params0.mDiffuseTransmission),
                                                  asCpp(params1.mDiffuseTransmission),
                                                  mask);

        if (mtl0HasDiffuse && mtl1HasDiffuse) {
            // both have diffuse
            asCpp(params.mScatteringRadius) = colorSpaceLerp(asCpp(params0.mScatteringRadius),
                                                             asCpp(params1.mScatteringRadius),
                                                             mask,
                                                             colorSpace);
            params.mCreaseAttenuation = lerp(params0.mCreaseAttenuation,
                                             params1.mCreaseAttenuation,
                                             mask);
            params.mDiffuseTransmissionBlendingBehavior = params1.mDiffuseTransmissionBlendingBehavior;
            params.mDiffuseRoughness = lerp(params0.mDiffuseRoughness,
                                            params1.mDiffuseRoughness,
                                            mask);
        } else if (mtl1HasDiffuse) {
            // only mtl1 has diffuse
            asCpp(params.mScatteringRadius) = asCpp(params1.mScatteringRadius);
            params.mCreaseAttenuation = params1.mCreaseAttenuation;
            params.mDiffuseTransmissionBlendingBehavior = params1.mDiffuseTransmissionBlendingBehavior;
            params.mDiffuseRoughness = params1.mDiffuseRoughness;
        } else {
            // only mtl0 has diffuse
            asCpp(params.mScatteringRadius) = asCpp(params0.mScatteringRadius);
            params.mCreaseAttenuation = params0.mCreaseAttenuation;
            params.mDiffuseTransmissionBlendingBehavior = params0.mDiffuseTransmissionBlendingBehavior;
            params.mDiffuseRoughness = params0.mDiffuseRoughness;
        }
    }
}

void blendRefractiveIndexParams(const float mask,
                                const ispc::DwaBaseParameters &params0,
                                const ispc::DwaBaseParameters &params1,
                                ispc::DwaBaseParameters &params)
{
    if (!isZero(params0.mFabricAttenuation) ||
        !isZero(params1.mFabricAttenuation)) {
        const bool usesIOR0 = !isOne(params0.mMetallic) &&
                              (!isZero(params0.mSpecular) || !isZero(params0.mTransmission)) &&
                              !isOne(params0.mFabricSpecular);
        const bool usesIOR1 = !isOne(params1.mMetallic) &&
                              (!isZero(params1.mSpecular) || !isZero(params1.mTransmission)) &&
                              !isOne(params1.mFabricSpecular);
        if (usesIOR0 && usesIOR1) {
            params.mRefractiveIndex = lerp(params0.mRefractiveIndex,
                                           params1.mRefractiveIndex,
                                           mask);
        } else if (usesIOR1) {
            params.mRefractiveIndex = params1.mRefractiveIndex;
        } else if (usesIOR0) {
            params.mRefractiveIndex = params0.mRefractiveIndex;
        }
    }
}

void blendCommonParams(const ispc::BlendColorSpace colorSpace,
                       const float mask,
                       const ispc::DwaBaseParameters &params0,
                       const ispc::DwaBaseParameters &params1,
                       ispc::DwaBaseParameters &params)
{

    asCpp(params.mDiffuseNormal) = safeNormalize(lerp(asCpp(params0.mDiffuseNormal),
                                                      asCpp(params1.mDiffuseNormal),
                                                      mask));
    asCpp(params.mNormal) = safeNormalize(lerp(asCpp(params0.mNormal),
                                               asCpp(params1.mNormal),
                                               mask));
    params.mNormalLength = lerp(params0.mNormalLength,
                                params1.mNormalLength,
                                mask);
    if (params0.mNormalAAStrategy != params1.mNormalAAStrategy) {
        // values are mixed: Toksvig wins
        params.mNormalAAStrategy = ispc::NORMAL_AA_STRATEGY_TOKSVIG;
    } else {
        params.mNormalAAStrategy = params1.mNormalAAStrategy;
    }
    if (params.mNormalAAStrategy == ispc::NORMAL_AA_STRATEGY_NONE) {
        params.mNormalAADial = lerp(params0.mNormalAADial,
                                    params1.mNormalAADial,
                                    mask);
    }
    asCpp(params.mEmission) = colorSpaceLerp(asCpp(params0.mEmission),
                                             asCpp(params1.mEmission),
                                             mask,
                                             colorSpace);
}

void blendColorCorrectParams(const ispc::BlendColorSpace colorSpace,
                             const float mask,
                             const ispc::DwaBaseParameters &params0,
                             const ispc::DwaBaseParameters &params1,
                             ispc::DwaBaseParameters &params)
{
    const size_t maxCC = max(params0.mNumColorCorrections, params1.mNumColorCorrections);
    for (size_t cc = 0; cc < maxCC; ++cc) {
        if (!params0.mColorCorrectParams[cc].mOn && !params1.mColorCorrectParams[cc].mOn) {
            continue;
        } else if (params0.mColorCorrectParams[cc].mOn && !params1.mColorCorrectParams[cc].mOn) {
            params.mColorCorrectParams[cc].mOn = params0.mColorCorrectParams[cc].mOn;
            params.mColorCorrectParams[cc].mMix = params0.mColorCorrectParams[cc].mMix;
            params.mColorCorrectParams[cc].mHueShift = params0.mColorCorrectParams[cc].mHueShift;
            params.mColorCorrectParams[cc].mSaturation = params0.mColorCorrectParams[cc].mSaturation;
            params.mColorCorrectParams[cc].mGain = params0.mColorCorrectParams[cc].mGain;
            params.mColorCorrectParams[cc].mTmiEnabled = params0.mColorCorrectParams[cc].mTmiEnabled;
            params.mColorCorrectParams[cc].mTmi = params0.mColorCorrectParams[cc].mTmi;
        } else if (!params0.mColorCorrectParams[cc].mOn && params1.mColorCorrectParams[cc].mOn) {
            params.mColorCorrectParams[cc].mOn = params1.mColorCorrectParams[cc].mOn;
            params.mColorCorrectParams[cc].mMix = params1.mColorCorrectParams[cc].mMix;
            params.mColorCorrectParams[cc].mHueShift = params1.mColorCorrectParams[cc].mHueShift;
            params.mColorCorrectParams[cc].mSaturation = params1.mColorCorrectParams[cc].mSaturation;
            params.mColorCorrectParams[cc].mGain = params1.mColorCorrectParams[cc].mGain;
            params.mColorCorrectParams[cc].mTmiEnabled = params1.mColorCorrectParams[cc].mTmiEnabled;
            params.mColorCorrectParams[cc].mTmi = params1.mColorCorrectParams[cc].mTmi;
        } else {
            params.mColorCorrectParams[cc].mOn = true;

            params.mColorCorrectParams[cc].mMix = lerp(params0.mColorCorrectParams[cc].mMix,
                                                       params1.mColorCorrectParams[cc].mMix,
                                                       mask);

            params.mColorCorrectParams[cc].mHueShift = lerp(params0.mColorCorrectParams[cc].mHueShift,
                                                            params1.mColorCorrectParams[cc].mHueShift,
                                                            mask);

            params.mColorCorrectParams[cc].mSaturation = lerp(params0.mColorCorrectParams[cc].mSaturation,
                                                              params1.mColorCorrectParams[cc].mSaturation,
                                                              mask);

            params.mColorCorrectParams[cc].mGain = lerp(params0.mColorCorrectParams[cc].mGain,
                                                        params1.mColorCorrectParams[cc].mGain,
                                                        mask);

            params.mColorCorrectParams[cc].mTmiEnabled = params0.mColorCorrectParams[cc].mTmiEnabled ||
                                                         params1.mColorCorrectParams[cc].mTmiEnabled;
            if (params.mColorCorrectParams[cc].mTmiEnabled) {
                asCpp(params.mColorCorrectParams[cc].mTmi) = colorSpaceLerp(asCpp(params0.mColorCorrectParams[cc].mTmi),
                                                                            asCpp(params1.mColorCorrectParams[cc].mTmi),
                                                                            mask,
                                                                            colorSpace);
            }
        }
    }
}

void blendAccentParams(const ispc::BlendColorSpace colorSpace,
                       const float mask,
                       const ispc::DwaBaseParameters &params0,
                       const ispc::DwaBaseParameters &params1,
                       ispc::DwaBaseParameters &params)
{
    params.mAccentParams.mSubsurface = lerp(params0.mAccentParams.mSubsurface,
                                            params1.mAccentParams.mSubsurface,
                                            mask);

   asCpp(params.mAccentParams.mSubsurfaceNormal) = safeNormalize(lerp(asCpp(params0.mAccentParams.mSubsurfaceNormal),
                                                                      asCpp(params1.mAccentParams.mSubsurfaceNormal),
                                                                      mask));

    params.mAccentParams.mSubsurfaceNormalDial = lerp(params0.mAccentParams.mSubsurfaceNormalDial,
                                                      params1.mAccentParams.mSubsurfaceNormalDial,
                                                      mask);

    asCpp(params.mAccentParams.mSubsurfaceColor) = colorSpaceLerp(asCpp(params0.mAccentParams.mSubsurfaceColor),
                                                                  asCpp(params1.mAccentParams.mSubsurfaceColor),
                                                                  mask,
                                                                  colorSpace);
}

//---------------------------------------------------------------------------
// Blend hair helper functions

void
copyHairRParams(const ispc::HairParameters& src,
                ispc::HairParameters& dst)
{
    dst.mHairShowR                  = src.mHairShowR;
    dst.mHairRShift                 = src.mHairRShift;
    dst.mHairRLongRoughness         = src.mHairRLongRoughness;
    asCpp(dst.mHairRTint)           = asCpp(src.mHairRTint);
}

void
copyHairTTParams(const ispc::HairParameters& src,
                 ispc::HairParameters& dst)
{
    dst.mHairShowTT                 = src.mHairShowTT;
    dst.mHairTTShift                = src.mHairTTShift;
    dst.mHairTTLongRoughness        = src.mHairTTLongRoughness;
    dst.mHairTTAzimRoughness        = src.mHairTTAzimRoughness;
    asCpp(dst.mHairTTTint)          = asCpp(src.mHairTTTint);
    dst.mHairTTSaturation           = src.mHairTTSaturation;
}

void
copyHairTRTParams(const ispc::HairParameters& src,
                  ispc::HairParameters& dst)
{
    dst.mHairShowTRT                = src.mHairShowTRT;
    dst.mHairTRTShift               = src.mHairTRTShift;
    dst.mHairTRTLongRoughness       = src.mHairTRTLongRoughness;
    asCpp(dst.mHairTRTTint)         = asCpp(src.mHairTRTTint);
}

void
copyHairGlintParams(const ispc::HairParameters& src,
                    ispc::HairParameters& dst)
{
    dst.mHairShowGlint                = src.mHairShowGlint;
    dst.mHairGlintRoughness           = src.mHairGlintRoughness;
    dst.mHairGlintMinTwists           = src.mHairGlintMinTwists;
    dst.mHairGlintMaxTwists           = src.mHairGlintMaxTwists;
    dst.mHairGlintEccentricity        = src.mHairGlintEccentricity;
    dst.mHairGlintSaturation          = src.mHairGlintSaturation;
}

void
copyHairTRRTParams(const ispc::HairParameters& src,
                   ispc::HairParameters& dst)
{
    dst.mHairShowTRRT               = src.mHairShowTRRT;
    dst.mHairTRRTLongRoughness      = src.mHairTRRTLongRoughness;
}

void
copyHairParams(const ispc::HairParameters& src,
                     ispc::HairParameters& dst)
{
    asCpp(dst.mHairUV)              = asCpp(src.mHairUV);
    dst.mHairIOR                    = src.mHairIOR;
    dst.mHairFresnelType            = src.mHairFresnelType;
    dst.mHairCuticleLayerThickness  = src.mHairCuticleLayerThickness;
    dst.mHairUseOptimizedSampling   = src.mHairUseOptimizedSampling;
    asCpp(dst.mHairColor)           = asCpp(src.mHairColor);

    copyHairRParams(src, dst);
    copyHairTTParams(src, dst);
    copyHairTRTParams(src, dst);
    copyHairTRRTParams(src, dst);
}

void
copyHairDiffuseParams(const ispc::HairParameters& src,
                            ispc::HairParameters& dst)
{
    asCpp(dst.mHairDiffuseFrontColor)               = asCpp(src.mHairDiffuseFrontColor);
    asCpp(dst.mHairDiffuseBackColor)                = asCpp(src.mHairDiffuseBackColor);
    dst.mHairDiffuseUseIndependentFrontAndBackColor = src.mHairDiffuseUseIndependentFrontAndBackColor;
}

// Hair Parameter Blending functions
// ----------------------------------------------------------------

void
blendEmissionParams(const ispc::BlendColorSpace colorSpace,
                    const float mask,
                    ispc::DwaBaseParameters &params0,
                    ispc::DwaBaseParameters &params1,
                    ispc::DwaBaseParameters &params)
{
    asCpp(params.mEmission) = colorSpaceLerp(asCpp(params0.mEmission),
                                             asCpp(params1.mEmission),
                                             mask,
                                             colorSpace);
}

void
blendHairCommonParams(const ispc::BlendColorSpace colorSpace,
                    const float mask,
                    ispc::HairParameters &params0,
                    ispc::HairParameters &params1,
                    ispc::HairParameters &params)
{
    params.mHairCastsCaustics = params0.mHairCastsCaustics || params1.mHairCastsCaustics;

    // The mHairDir should be the same in both params0 and params1
    // since it is the geometry's dPds vector
    params.mHairDir = params1.mHairDir;
}

void
blendHairSpecParams(const ispc::BlendColorSpace colorSpace,
                    const float mask,
                    ispc::HairParameters &params0,
                    ispc::HairParameters &params1,
                    ispc::HairParameters &params)
{
    params.mHair = lerp(params0.mHair,
                        params1.mHair,
                        mask);

    // Can we exit early?
    if (isZero(params.mHair)) { return; }
    if (isZero(params1.mHair) && !isZero(params0.mHair)) {
        copyHairParams(params0, params);
        return;
    } else if (isZero(params0.mHair) && !isZero(params1.mHair)) {
        copyHairParams(params1, params);
        return;
    }

    // Hair present on both materials, blend the params

    // If the hair fresnel types don't match we error before
    // this function is called so they must match.
    params.mHairFresnelType = params1.mHairFresnelType;

    asCpp(params.mHairUV) = lerp(asCpp(params0.mHairUV),
                                 asCpp(params1.mHairUV),
                                 mask);
    params.mHairIOR = lerp(params0.mHairIOR,
                           params1.mHairIOR,
                           mask);
    params.mHairCuticleLayerThickness = lerp(params0.mHairCuticleLayerThickness,
                                             params1.mHairCuticleLayerThickness,
                                             mask);
    params.mHairUseOptimizedSampling = params0.mHairUseOptimizedSampling ||
                                       params1.mHairUseOptimizedSampling;

    // Used in Color->Sigma computations for all lobes
    params.mHairTTAzimRoughness = lerp(params0.mHairTTAzimRoughness,
                                       params1.mHairTTAzimRoughness,
                                       mask);
    // HairR
    if (params0.mHairShowR && params1.mHairShowR) {
        params.mHairShowR = true;
        params.mHairRShift = lerp(params0.mHairRShift,
                                  params1.mHairRShift,
                                  mask);
        params.mHairRLongRoughness = lerp(params0.mHairRLongRoughness,
                                          params1.mHairRLongRoughness,
                                          mask);
        asCpp(params.mHairRTint) = colorSpaceLerp(asCpp(params0.mHairRTint),
                                                  asCpp(params1.mHairRTint),
                                                  mask,
                                                  colorSpace);
    } else if (params0.mHairShowR) {
        copyHairRParams(params0, params);
    } else {
        copyHairRParams(params1, params);
    }

    // HairTT
    if (params0.mHairShowTT && params1.mHairShowTT) {
        params.mHairShowTT = true;
        params.mHairTTShift = lerp(params0.mHairTTShift,
                                   params1.mHairTTShift,
                                   mask);
        params.mHairTTLongRoughness = lerp(params0.mHairTTLongRoughness,
                                           params1.mHairTTLongRoughness,
                                           mask);
        asCpp(params.mHairTTTint) = colorSpaceLerp(asCpp(params0.mHairTTTint),
                                                   asCpp(params1.mHairTTTint),
                                                   mask,
                                                   colorSpace);
        params.mHairTTSaturation    = lerp(params0.mHairTTSaturation,
                                           params1.mHairTTSaturation,
                                           mask);
    } else if (params0.mHairShowTT) {
        copyHairTTParams(params0, params);
    } else {
        copyHairTTParams(params1, params);
    }

    // HairTRT
    if (params0.mHairShowTRT && params1.mHairShowTRT) {
        params.mHairShowTRT = true;
        params.mHairTRTShift = lerp(params0.mHairTRTShift,
                                    params1.mHairTRTShift,
                                    mask);
        params.mHairTRTLongRoughness = lerp(params0.mHairTRTLongRoughness,
                                            params1.mHairTRTLongRoughness,
                                            mask);
        asCpp(params.mHairTRTTint) = colorSpaceLerp(asCpp(params0.mHairTRTTint),
                                                    asCpp(params1.mHairTRTTint),
                                                    mask,
                                                    colorSpace);
        // Hair Glint
        if (params0.mHairShowGlint && params1.mHairShowGlint) {
            params.mHairShowGlint = true;
            params.mHairGlintRoughness = lerp(params0.mHairGlintRoughness,
                                              params1.mHairGlintRoughness,
                                              mask);
            params.mHairGlintMinTwists = lerp(params0.mHairGlintMinTwists,
                                              params1.mHairGlintMinTwists,
                                              mask);
            params.mHairGlintMaxTwists = lerp(params0.mHairGlintMaxTwists,
                                              params1.mHairGlintMaxTwists,
                                              mask);
            params.mHairGlintEccentricity = lerp(params0.mHairGlintEccentricity,
                                                 params1.mHairGlintEccentricity,
                                                 mask);
            params.mHairGlintSaturation = lerp(params0.mHairGlintSaturation,
                                               params1.mHairGlintSaturation,
                                               mask);
        } else if (params0.mHairShowGlint) {
            copyHairGlintParams(params0, params);
        } else {
            copyHairGlintParams(params1, params);
        }
    } else if (params0.mHairShowTRT) {
        copyHairTRTParams(params0, params);
    } else {
        copyHairTRTParams(params1, params);
    }

    // HairTRRT
    if (params0.mHairShowTRRT && params1.mHairShowTRRT) {
        params.mHairShowTRRT = true;
        params.mHairTRRTLongRoughness = lerp(params0.mHairTRRTLongRoughness,
                                             params1.mHairTRRTLongRoughness,
                                             mask);
    } else if (params0.mHairShowTRRT) {
        copyHairTRRTParams(params0, params);
    } else {
        copyHairTRRTParams(params1, params);
    }

    // Hair Color
    if (!isOne(params0.mHairDiffuse) && !isOne(params1.mHairDiffuse)) {
        asCpp(params.mHairColor) = colorSpaceLerp(asCpp(params0.mHairColor),
                                                  asCpp(params1.mHairColor),
                                                  mask,
                                                  colorSpace);
    } else if (!isOne(params0.mHairDiffuse)) {
        asCpp(params.mHairColor) = asCpp(params0.mHairColor);
    } else {
        asCpp(params.mHairColor) = asCpp(params1.mHairColor);
    }
}

void
blendHairDiffuseParams(const ispc::BlendColorSpace colorSpace,
                       const float mask,
                       ispc::DwaBaseParameters &params0,
                       ispc::DwaBaseParameters &params1,
                       ispc::DwaBaseParameters &params)
{
    const ispc::HairParameters hParams0 = params0.mHairParameters;
    const ispc::HairParameters hParams1 = params1.mHairParameters;

    params.mHairParameters.mHairDiffuse = lerp(hParams0.mHairDiffuse,
                                               hParams1.mHairDiffuse,
                                               mask);

    if (isZero(hParams1.mHairDiffuse) && !isZero(hParams0.mHairDiffuse)) {
        copyHairDiffuseParams(hParams0, params.mHairParameters);
        asCpp(params.mNormal) = asCpp(params0.mNormal);
    } else if (isZero(hParams0.mHairDiffuse) && !isZero(hParams1.mHairDiffuse)) {
        copyHairDiffuseParams(hParams1, params.mHairParameters);
        asCpp(params.mNormal) = asCpp(params1.mNormal);
    } else {
        params.mHairParameters.mHairDiffuseUseIndependentFrontAndBackColor =
            hParams0.mHairDiffuseUseIndependentFrontAndBackColor ||
            hParams1.mHairDiffuseUseIndependentFrontAndBackColor;

        asCpp(params.mHairParameters.mHairDiffuseFrontColor) =
            colorSpaceLerp(asCpp(hParams0.mHairDiffuseFrontColor),
                           asCpp(hParams1.mHairDiffuseFrontColor),
                           mask,
                           colorSpace);

        asCpp(params.mHairParameters.mHairDiffuseBackColor) =
            colorSpaceLerp(asCpp(hParams0.mHairDiffuseBackColor),
                           asCpp(hParams1.mHairDiffuseBackColor),
                           mask,
                           colorSpace);
       asCpp(params.mNormal) = safeNormalize(lerp(asCpp(params0.mNormal),
                                                  asCpp(params1.mNormal),
                                                  mask));
    }

    // experimental hair SSS
    const bool mtl0HasScatter = !isBlack(asCpp(params0.mScatteringRadius))
                                && !isZero(hParams0.mHairSubsurfaceBlend);
    const bool mtl1HasScatter = !isBlack(asCpp(params1.mScatteringRadius))
                                && !isZero(hParams1.mHairSubsurfaceBlend);

    // hair diffuse has an extra parameter to blend / choose kajiya kay or SSS (mHairSubsurfaceBlend)
    // if a submtl is diffuse but no scatter, treat it as 0 for blending
    // if a submtl is partly diffuse but no scatter, the blending toward 0 is weighted
    // if a submtl is not diffuse, copy it over from the diffuse
    // if both have scatter, blend normally
    if (mtl0HasScatter && !mtl1HasScatter) {
        asCpp(params.mScatteringRadius) = asCpp(params0.mScatteringRadius);
        if (isZero(hParams1.mHairDiffuse)) {
            params.mHairParameters.mHairSubsurfaceBlend = hParams0.mHairSubsurfaceBlend;
        } else {
            params.mHairParameters.mHairSubsurfaceBlend = lerp(hParams0.mHairSubsurfaceBlend,
                                                               0.0f,
                                                               hParams1.mHairDiffuse * mask);
        }
    } else if (mtl1HasScatter && !mtl0HasScatter) {
        asCpp(params.mScatteringRadius) = asCpp(params1.mScatteringRadius);
        if (isZero(hParams0.mHairDiffuse)) {
            params.mHairParameters.mHairSubsurfaceBlend = hParams1.mHairSubsurfaceBlend;
        } else {
            params.mHairParameters.mHairSubsurfaceBlend = lerp(hParams1.mHairSubsurfaceBlend,
                                                               0.0f,
                                                               hParams0.mHairDiffuse * (1.0f - mask));
        }
    } else {
        asCpp(params.mScatteringRadius) = colorSpaceLerp(asCpp(params0.mScatteringRadius),
                                                         asCpp(params1.mScatteringRadius),
                                                         mask,
                                                         colorSpace);
        params.mHairParameters.mHairSubsurfaceBlend = lerp(hParams0.mHairSubsurfaceBlend,
                                                           hParams1.mHairSubsurfaceBlend,
                                                           mask);
    }
}

void
blendHairToonParams(const ispc::BlendColorSpace colorSpace,
                    const float mask,
                    const ispc::DwaBaseParameters &params0,
                    const ispc::DwaBaseParameters &params1,
                    ispc::DwaBaseParameters &params)
{
    blendToonSpecularParams(colorSpace,
                            mask,
                            params0.mHairToonS1Params,
                            params1.mHairToonS1Params,
                            params.mHairToonS1Params);

    blendToonSpecularParams(colorSpace,
                            mask,
                            params0.mHairToonS2Params,
                            params1.mHairToonS2Params,
                            params.mHairToonS2Params);

    blendToonSpecularParams(colorSpace,
                            mask,
                            params0.mHairToonS3Params,
                            params1.mHairToonS3Params,
                            params.mHairToonS3Params);
}


//---------------------------------------------------------------------------

bool resolveSingleMaterialParameters(const dwabase::DwaBaseLayerable* layerable,
                                     TLState *tls,
                                     const State& state,
                                     const bool castsCaustics,
                                     ispc::DwaBaseParameters &params,
                                     bool subMtl0HasGlitter,
                                     bool subMtl1HasGlitter,
                                     const glitter::Glitter* glitterPtr)
{
    // DwaBaseParameters stores a pointer to a single Glitter
    // object from which we call createLobes.   The uniform
    // parameters for this object are set during construction
    // so we cannot blend them at shade time. Therefore, if
    // both materials have glitter enabled we use the Glitter
    // object and uniform glitter parameters stored on this
    // layer material.
    if (subMtl0HasGlitter && subMtl1HasGlitter) {
        bool result = layerable->resolveParameters(tls, state, castsCaustics, params);
        if (result) {
            if (glitterPtr) {
                params.mGlitterPointerScalar = (intptr_t)glitterPtr;
                params.mGlitterUniformParameters = glitterPtr->getIspcUniformParameters();
            } else {
                result = false;
            }
        }
        return result;
    } else {
        return layerable->resolveParameters(tls, state, castsCaustics, params);
    }
}

void setCommonFuncPtrs(const scene_rdl2::rdl2::SceneObject* sceneObject,
                       const dwabase::DwaBaseLayerable* mtl,
                       ispc::SubMtlData& subMtlData)
{
    if (mtl) {
        subMtlData.mHasGlitter = mtl->hasGlitter();
        subMtlData.mGetCastsCausticsFunc = (intptr_t) mtl->getCastsCausticsISPCFunc();
        subMtlData.mResolveSubsurfaceTypeFunc = (intptr_t) mtl->getResolveSubsurfaceTypeISPCFunc();
        subMtlData.mResolveParametersFunc = (intptr_t) mtl->getResolveParametersISPCFunc();
        subMtlData.mResolvePresenceFunc = (intptr_t) mtl->getResolvePresenceISPCFunc();
        subMtlData.mResolveSubsurfaceNormalFunc = (intptr_t) mtl->getResolveSubsurfaceNormalISPCFunc();
        subMtlData.mCreateLobesFunc = (intptr_t) mtl->getCreateLobesISPCFunc();
    } else {
        subMtlData.mHasGlitter = false;
        subMtlData.mGetCastsCausticsFunc = (intptr_t) nullptr;
        subMtlData.mResolveSubsurfaceTypeFunc = (intptr_t) nullptr;
        subMtlData.mResolveParametersFunc = (intptr_t) nullptr;
        subMtlData.mResolvePresenceFunc = (intptr_t) nullptr;
        subMtlData.mResolveSubsurfaceNormalFunc = (intptr_t) nullptr;
        subMtlData.mCreateLobesFunc = (intptr_t) nullptr;
    }

    // Check if the ispc resolve functions are valid
    // if not, make the ispc material pointer null
    if (mtl &&
        subMtlData.mGetCastsCausticsFunc &&
        subMtlData.mResolveSubsurfaceTypeFunc &&
        subMtlData.mResolveParametersFunc &&
        subMtlData.mResolvePresenceFunc &&
        subMtlData.mResolveSubsurfaceNormalFunc) {
        subMtlData.mDwaBaseLayerable = (intptr_t) sceneObject;
    } else {
        subMtlData.mDwaBaseLayerable = (intptr_t) nullptr;
    }
}

} // end anonymous namespace

//---------------------------------------------------------------------------
// Functions used in update()

namespace dwabase {
// Attempt to cast material to DwaBaseLayerable and gather func info for ISPC
const DwaBaseLayerable*
registerLayerable(const scene_rdl2::rdl2::SceneObject* sceneObject, ispc::SubMtlData& subMtlData)
{
    const DwaBaseLayerable* mtl = dynamic_cast<const DwaBaseLayerable*>(sceneObject);

    setCommonFuncPtrs(sceneObject, mtl, subMtlData);

    if (mtl) {
        subMtlData.mResolvePreventLightCullingFunc = (intptr_t) mtl->getResolvePreventLightCullingISPCFunc();
    } else {
        subMtlData.mResolvePreventLightCullingFunc = (intptr_t) nullptr;
    }

    if (subMtlData.mResolvePreventLightCullingFunc) {
        subMtlData.mDwaBaseLayerable = (intptr_t) sceneObject;
    } else {
        subMtlData.mDwaBaseLayerable = (intptr_t) nullptr;
    }

    return mtl;
}

const DwaBaseLayerable*
registerHairLayerable(const scene_rdl2::rdl2::SceneObject* sceneObject, ispc::SubMtlData& subMtlData)
{
    const DwaBaseLayerable* mtl = dynamic_cast<const DwaBaseLayerable*>(sceneObject);

    setCommonFuncPtrs(sceneObject, mtl, subMtlData);

    return mtl;
}


void blendUniformParameters(const ispc::DwaBaseUniformParameters &uParams0,
                            const ispc::DwaBaseUniformParameters &uParams1,
                            ispc::DwaBaseUniformParameters &uParams,
                            int fallbackSpecularModel,
                            int fallbackOuterSpecularModel,
                            bool fallbackOuterSpecularUseBending,
                            int fallbackBSSRDF,
                            bool fallbackThinGeometry,
                            bool fallbackPreventLightCulling)
{
    if (uParams0.mSpecularModel != uParams1.mSpecularModel) {
        uParams.mSpecularModel = fallbackSpecularModel;
    }
    if (uParams0.mOuterSpecularModel != uParams1.mOuterSpecularModel) {
        uParams.mOuterSpecularModel = fallbackOuterSpecularModel;
    }
    if (uParams0.mOuterSpecularUseBending != uParams1.mOuterSpecularUseBending) {
        uParams.mOuterSpecularUseBending = fallbackOuterSpecularUseBending;
    }
    if (uParams.mSubsurface != fallbackBSSRDF) {
        // If mSubsurface == fallbackBSSRDF, mixed subsurface models have already been detected, use fallback.
        if (uParams0.mSubsurface != uParams1.mSubsurface) {
            if (uParams0.mSubsurface == ispc::SubsurfaceType::SUBSURFACE_NONE) {
                uParams.mSubsurface = uParams1.mSubsurface;
            } else if (uParams1.mSubsurface == ispc::SubsurfaceType::SUBSURFACE_NONE) {
                uParams.mSubsurface = uParams0.mSubsurface;
            } else {
                uParams.mSubsurface = fallbackBSSRDF;
            }
        } else if (uParams.mSubsurface == ispc::SubsurfaceType::SUBSURFACE_NONE) {
            // If the current subsurface types match,
            // only update the subsurface type if it was NONE
            // so we don't replace valid subsurface types with NONE
            uParams.mSubsurface = uParams0.mSubsurface;
        }
    }
    if (uParams0.mThinGeometry != uParams1.mThinGeometry) {
        uParams.mThinGeometry = fallbackThinGeometry;
    }
    if (uParams0.mPreventLightCulling != uParams1.mPreventLightCulling) {
        uParams.mPreventLightCulling = fallbackPreventLightCulling;
    }
}

//---------------------------------------------------------------------------
// Functions used in shade()

float blendPresence(TLState *tls,
                    const State& state,
                    const DwaBaseLayerable* layerable0,
                    const DwaBaseLayerable* layerable1,
                    float mask)
{
    if (!layerable0) return 1.f; // no background material (DwaLayerMaterial) / no first material (DwaMixMaterial)
    if (!layerable1) return layerable0->resolvePresence(tls, state);

    // is the mask value binary 0|1 ?
    if (isZero(mask)) {
        return layerable0->resolvePresence(tls, state);
    } else if (isOne(mask)) {
        return layerable1->resolvePresence(tls, state);
    }

    // resolve both sub-material params and blend
    const float presence0 = layerable0->resolvePresence(tls, state);
    const float presence1 = layerable1->resolvePresence(tls, state);

    return lerp(presence0, presence1, mask);
}

float blendRefractiveIndex(TLState *tls,
                           const State& state,
                           const DwaBaseLayerable* layerable0,
                           const DwaBaseLayerable* layerable1,
                           float mask)
{
    if (!layerable0) return 1.f; // no background material (DwaLayerMaterial) / no first material (DwaMixMaterial)
    if (!layerable1) return layerable0->resolveRefractiveIndex(tls, state);

    // is the mask value binary 0|1 ?
    if (isZero(mask)) {
        return layerable0->resolveRefractiveIndex(tls, state);
    } else if (isOne(mask)) {
        return layerable1->resolveRefractiveIndex(tls, state);
    }

    // resolve both sub-material params and blend
    const float ior0 = layerable0->resolveRefractiveIndex(tls, state);
    const float ior1 = layerable1->resolveRefractiveIndex(tls, state);

    return lerp(ior0, ior1, mask);
}

scene_rdl2::math::Vec3f blendSubsurfaceNormal(TLState *tls,
                                  const State& state,
                                  const DwaBaseLayerable* layerable0,
                                  const DwaBaseLayerable* layerable1,
                                  float mask)
{
    // is the mask value binary 0|1 ?
    if (isZero(mask)) {
        return evalSubsurfaceNormalIfDefined(layerable0, tls, state);
    } else if (isOne(mask)) {
        return evalSubsurfaceNormalIfDefined(layerable1, tls, state);
    }

    // resolve both sub-material params and blend
    const scene_rdl2::math::Vec3f normal0 = evalSubsurfaceNormalIfDefined(layerable0, tls, state);
    const scene_rdl2::math::Vec3f normal1 = evalSubsurfaceNormalIfDefined(layerable1, tls, state);

    return safeNormalize(lerp(normal0, normal1, mask));
}

bool blendParameters(TLState *tls,
                     const State& state,
                     const bool castsCaustics,
                     ispc::DwaBaseParameters &params,
                     const ispc::DwaBaseUniformParameters &uParams,
                     ispc::BlendColorSpace colorSpace,
                     const glitter::Glitter* glitterPtr,
                     intptr_t evalSubsurfaceNormal,
                     bool subMtl0HasGlitter,
                     bool subMtl1HasGlitter,
                     const DwaBaseLayerable* layerable0,
                     const DwaBaseLayerable* layerable1,
                     float mask)
{
    // The uniform parameters were blended in update()

    // First verify that any submaterials that are connected are
    // DwaBaseLayerable, otherwise exit early
    if (!layerable0) {
        // exit early, (background material / first material) is missing or not Layerable
        return false;
    } else if (!layerable1) {
        // early exit
        return layerable0->resolveParameters(tls, state, castsCaustics, params);
    } else if (isZero(mask)) {
        // early exit
        return resolveSingleMaterialParameters(layerable0, tls, state, castsCaustics, params,
                                               subMtl0HasGlitter, subMtl1HasGlitter, glitterPtr);
    } else if (isOne(mask)) {
        // early exit
        return resolveSingleMaterialParameters(layerable1, tls, state, castsCaustics, params,
                                               subMtl0HasGlitter, subMtl1HasGlitter, glitterPtr);
    }

    // resolve both sub-material params and blend
    ispc::DwaBaseParameters params0, params1;
    DwaBaseLayerable::copyColorCorrectParameters(params, params0);
    DwaBaseLayerable::copyColorCorrectParameters(params, params1);

    bool success = layerable0->resolveParameters(tls, state, castsCaustics, params0) &&
                   layerable1->resolveParameters(tls, state, castsCaustics, params1);
    if (!success) { return false; }

    // make sure all fields are initialized
    DwaBaseLayerable::initParameters(params);

    blendGlitterParams(colorSpace, mask, glitterPtr, params0, params1, params);
    blendCommonParams(colorSpace, mask, params0, params1, params);
    blendFuzzParams(colorSpace, mask, params0, params1, params);
    blendOuterSpecularParams(colorSpace, mask, params0, params1, params);
    blendCommonSpecularParams(mask, params0, params1, params);
    blendIridescenceParams(colorSpace, mask, params0, params1, params);
    blendMetallicParams(colorSpace, mask, params0, params1, params);
    blendColorCorrectParams(colorSpace, mask, params0, params1, params);
    blendAccentParams(colorSpace, mask, params0, params1, params);

    // Always blend params.mFabricAttenuation before calling
    // blendRefractiveIndex, blendTransmission, blendDiffuse
    params.mFabricAttenuation = lerp(params0.mFabricAttenuation, params1.mFabricAttenuation, mask);

    blendFabricParams(colorSpace, mask, params0, params1, params);
    blendRefractiveIndexParams(mask, params0, params1, params);
    blendTransmissionParams(colorSpace, mask, params0, params1, params);

    // Blend Toon Diffuse
    blendToonParams(colorSpace, mask, params0, params1, params);

    blendDiffuseParams(colorSpace, mask, params0, params1, params);

    params.mEvalSubsurfaceNormalFn = evalSubsurfaceNormal;

    return true;
}

bool
blendHairParameters(TLState *tls,
                    const State& state,
                    const bool castsCaustics,
                    ispc::DwaBaseParameters &params,
                    const ispc::DwaBaseUniformParameters &uParams,
                    ispc::BlendColorSpace colorSpace,
                    intptr_t evalSubsurfaceNormal,
                    const DwaBaseLayerable* layerable0,
                    const DwaBaseLayerable* layerable1,
                    const DwaBaseLayerable* me,
                    scene_rdl2::logging::LogEvent errorMismatchedFresnelType,
                    float mask)
{
    // mSubsurface is blended in update() as part of
    // blendUniformParameters()

    if (!layerable0) {
        return false; // no background material
    } else if (!layerable1) {
        return layerable0->resolveParameters(tls, state, castsCaustics, params);
    } else if (isZero(mask)) {
        return layerable0->resolveParameters(tls, state, castsCaustics, params);
    } else if (isOne(mask)) {
        return layerable1->resolveParameters(tls, state, castsCaustics, params);
    }
    // resolve both sub-material params and blend
    ispc::DwaBaseParameters params0, params1;
    DwaBaseLayerable::copyColorCorrectParameters(params, params0);
    DwaBaseLayerable::copyColorCorrectParameters(params, params1);

    bool success = layerable0->resolveParameters(tls, state, castsCaustics, params0) &&
                   layerable1->resolveParameters(tls, state, castsCaustics, params1);
    if (!success) { return false; }

    if (!isZero(params0.mHairParameters.mHair) && !isZero(params1.mHairParameters.mHair) &&
        params0.mHairParameters.mHairFresnelType != params1.mHairParameters.mHairFresnelType) {
            moonray::shading::logEvent(me, errorMismatchedFresnelType);
            return false;
    }

    params.mEvalSubsurfaceNormalFn = evalSubsurfaceNormal;

    // make sure all fields are initialized
    DwaBaseLayerable::initParameters(params);

    blendEmissionParams(colorSpace,
                        mask,
                        params0,
                        params1,
                        params);

    blendHairCommonParams(colorSpace,
                          mask,
                          params0.mHairParameters,
                          params1.mHairParameters,
                          params.mHairParameters);

    blendHairSpecParams(colorSpace,
                        mask,
                        params0.mHairParameters,
                        params1.mHairParameters,
                        params.mHairParameters);

    blendHairDiffuseParams(colorSpace,
                           mask,
                           params0,
                           params1,
                           params);

    blendHairToonParams(colorSpace,
                        mask,
                        params0,
                        params1,
                        params);

    return true;
}

} // namespace dwabase
} // namespace moonshine

