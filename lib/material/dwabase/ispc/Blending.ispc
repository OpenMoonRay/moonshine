// Copyright 2023-2024 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

/// @file Blending.ispc

#include "Blending.isph"
#include <scene_rdl2/common/platform/IspcUtil.isph>

ISPC_UTIL_EXPORT_ENUM_TO_HEADER(BlendColorSpace);
ISPC_UTIL_EXPORT_UNIFORM_STRUCT_TO_HEADER(SubMtlData);


// Parameter Blending functions
// ----------------------------------------------------------------
// These functions are responsible for blending the various
// Base material parameters. The blending has been split into
// separate functions for improved readability. Each function
// sparsely initializes the subset of members of the passed
// DwaBaseParameters 'params' structure that are related
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

varying Color
colorSpaceLerp(const varying Color& color0,
               const varying Color& color1,
               const varying float mask,
               const uniform int colorSpace)
{
    switch (colorSpace) {
    case COLOR_BLEND_RGB:
        return lerp(color0, color1, mask);
    case COLOR_BLEND_HSV:
        return hsvToRgb(lerp(rgbToHsv(color0),
                             rgbToHsv(color1),
                             mask));
    case COLOR_BLEND_HSL:
        return hslToRgb(lerp(rgbToHsl(color0),
                             rgbToHsl(color1),
                             mask));
    default:
        return lerp(color0, color1, mask);
    }
}

void blendFuzzParams(const uniform int colorSpace,
                     const varying float mask,
                     const varying DwaBaseParameters& params0,
                     const varying DwaBaseParameters& params1,
                     varying DwaBaseParameters& params)
{
    params.mFuzz = lerp(params0.mFuzz, params1.mFuzz, mask);
    if (!isZero(params.mFuzz)) {
        if (!isZero(params0.mFuzz) &&
            !isZero(params1.mFuzz)) {

            params.mFuzzRoughness = lerp(params0.mFuzzRoughness,
                                         params1.mFuzzRoughness,
                                         mask);
            params.mFuzzAlbedo = colorSpaceLerp(params0.mFuzzAlbedo,
                                                params1.mFuzzAlbedo,
                                                mask,
                                                colorSpace);
            params.mFuzzUseAbsorbingFibers = params0.mFuzzUseAbsorbingFibers ||
                                             params1.mFuzzUseAbsorbingFibers;
            params.mFuzzNormal = normalize(lerp(params0.mFuzzNormal,
                                                params1.mFuzzNormal,
                                                mask));
        } else if (!isZero(params1.mFuzz)) {
            params.mFuzzRoughness           = params1.mFuzzRoughness;
            params.mFuzzUseAbsorbingFibers  = params1.mFuzzUseAbsorbingFibers;
            params.mFuzzAlbedo              = params1.mFuzzAlbedo;
            params.mFuzzNormal              = params1.mFuzzNormal;
        } else { // params0.mFuzz != 0.0f
            params.mFuzzRoughness           = params0.mFuzzRoughness;
            params.mFuzzUseAbsorbingFibers  = params0.mFuzzUseAbsorbingFibers;
            params.mFuzzAlbedo              = params0.mFuzzAlbedo;
            params.mFuzzNormal              = params0.mFuzzNormal;
        }
    }
}

void
copyOuterSpecularParams(const varying DwaBaseParameters &src,
                              varying DwaBaseParameters &dst)
{
    dst.mOuterSpecularRefractiveIndex = src.mOuterSpecularRefractiveIndex;
    dst.mOuterSpecularRoughness = src.mOuterSpecularRoughness;
    dst.mOuterSpecularThickness = src.mOuterSpecularThickness;
    dst.mOuterSpecularAttenuationColor = src.mOuterSpecularAttenuationColor;
    dst.mOuterSpecularNormal = src.mOuterSpecularNormal;
    dst.mOuterSpecularNormalLength = src.mOuterSpecularNormalLength;
}

void blendOuterSpecularParams(const uniform int colorSpace,
                              const varying float mask,
                              const varying DwaBaseParameters& params0,
                              const varying DwaBaseParameters& params1,
                              varying DwaBaseParameters& params)
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
            params.mOuterSpecularAttenuationColor = colorSpaceLerp(params0.mOuterSpecularAttenuationColor,
                                                                   params1.mOuterSpecularAttenuationColor,
                                                                   mask,
                                                                   colorSpace);
            params.mOuterSpecularNormal = normalize(lerp(params0.mOuterSpecularNormal,
                                                         params1.mOuterSpecularNormal,
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
copyGlitterFlakeParams(const varying GLITTER_VaryingParameters &src,
                             varying GLITTER_VaryingParameters &dst)
{
    // flake style A
    dst.mFlakeStyleFrequency[0] = src.mFlakeStyleFrequency[0];
    dst.mFlakeColor[0] = src.mFlakeColor[0];
    dst.mFlakeSize[0] = src.mFlakeSize[0];
    dst.mFlakeRoughness[0] = src.mFlakeRoughness[0];

    // flake style B
    dst.mFlakeStyleFrequency[1] = src.mFlakeStyleFrequency[1];
    dst.mFlakeColor[1] = src.mFlakeColor[1];
    dst.mFlakeSize[1] = src.mFlakeSize[1];
    dst.mFlakeRoughness[1] = src.mFlakeRoughness[1];

    dst.mFlakeHSVColorVariation = src.mFlakeHSVColorVariation;
    dst.mFlakeDensity = src.mFlakeDensity;
    dst.mFlakeJitter = src.mFlakeJitter;
    dst.mFlakeOrientationRandomness = src.mFlakeOrientationRandomness;
    dst.mCompensateDeformation = src.mCompensateDeformation;
    dst.mApproximateForSecRays = src.mApproximateForSecRays;
}

void blendGlitterParams(const uniform int colorSpace,
                        const varying float mask,
                        const uniform GLITTER_Glitter * uniform layerMaterialGlitterPointer,
                        const uniform GLITTER_UniformParameters * uniform layerMaterialUniformParams,
                        const varying DwaBaseParameters& params0,
                        const varying DwaBaseParameters& params1,
                        varying DwaBaseParameters& params)
{
    // Assign some variables to make the code below shorter and more readable
    const GLITTER_VaryingParameters& vParams0 = params0.mGlitterVaryingParameters;
    const GLITTER_VaryingParameters& vParams1 = params1.mGlitterVaryingParameters;
    GLITTER_VaryingParameters& vParams = params.mGlitterVaryingParameters;

    vParams.mGlitterMask = lerp(vParams0.mGlitterMask,
                                vParams1.mGlitterMask,
                                mask);
    if (isZero(vParams.mGlitterMask)) return;

    if (!isZero(vParams0.mGlitterMask) &&
        !isZero(vParams1.mGlitterMask)) {

        if (!layerMaterialGlitterPointer) return;
        params.mGlitterPointerVector = layerMaterialGlitterPointer;
        params.mGlitterUniformParameters = layerMaterialUniformParams;

        vParams.mFlakeStyleFrequency[0] = lerp(vParams0.mFlakeStyleFrequency[0],
                                               vParams1.mFlakeStyleFrequency[0],
                                               mask);
        vParams.mFlakeColor[0] = colorSpaceLerp(vParams0.mFlakeColor[0],
                                                vParams1.mFlakeColor[0],
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
        vParams.mFlakeColor[1] = colorSpaceLerp(vParams0.mFlakeColor[1],
                                                vParams1.mFlakeColor[1],
                                                mask,
                                                colorSpace);
        vParams.mFlakeSize[1] = lerp(vParams0.mFlakeSize[1],
                                     vParams1.mFlakeSize[1],
                                     mask);
        vParams.mFlakeRoughness[1] = lerp(vParams0.mFlakeRoughness[1],
                                          vParams1.mFlakeRoughness[1],
                                          mask);

        vParams.mFlakeHSVColorVariation = lerp(vParams0.mFlakeHSVColorVariation,
                                               vParams1.mFlakeHSVColorVariation,
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
        if (!params0.mGlitterPointerVector) return;
        params.mGlitterPointerVector = params0.mGlitterPointerVector;
        params.mGlitterUniformParameters = params0.mGlitterUniformParameters;
        copyGlitterFlakeParams(vParams0, vParams);
    } else {
        if (!params1.mGlitterPointerVector) return;
        params.mGlitterPointerVector = params1.mGlitterPointerVector;
        params.mGlitterUniformParameters = params1.mGlitterUniformParameters;
        copyGlitterFlakeParams(vParams1, vParams);
    }
}

void blendCommonSpecularParams(const varying float mask,
                               const varying DwaBaseParameters &params0,
                               const varying DwaBaseParameters &params1,
                               varying DwaBaseParameters &params)
{
    params.mSpecular = lerp(params0.mSpecular,
                            params1.mSpecular,
                            mask);
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
                params.mShadingTangent = normalize(lerp(params0.mShadingTangent,
                                                        params1.mShadingTangent,
                                                        mask));
            } else if (!isZero(params1.mAnisotropy)) {
                // mtl0 Anisotropy is Zero so only mtl1's settings make sense
                // Modulating mtl1's settings based on the mask
                params.mAnisotropy = params1.mAnisotropy * mask;
                params.mShadingTangent = params1.mShadingTangent;
            } else { // !isZero(params0.mAnisotropy)
                // mtl1 Anisotropy is Zero so only mtl0's settings make sense
                // Modulating mtl0's settings based on the mask
                params.mAnisotropy = params0.mAnisotropy * (1.0f - mask);
                params.mShadingTangent = params0.mShadingTangent;
            }
        } else if (!isZero(params1.mSpecular)) {
            params.mRoughness = params1.mRoughness;
            params.mAnisotropy = params1.mAnisotropy;
            params.mShadingTangent = params1.mShadingTangent;
        } else { // !isZero(params0.mSpecular)
            params.mRoughness = params0.mRoughness;
            params.mAnisotropy = params0.mAnisotropy;
            params.mShadingTangent = params0.mShadingTangent;
        }
    }
}

void
copyIridescenceParams(const varying IridescenceParameters &src,
                            varying IridescenceParameters &dst)
{
    dst.mIridescenceApplyTo = src.mIridescenceApplyTo;
    dst.mIridescenceColorControl = src.mIridescenceColorControl;
    dst.mIridescencePrimaryColor = src.mIridescencePrimaryColor;
    dst.mIridescenceSecondaryColor = src.mIridescenceSecondaryColor;
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

void blendIridescenceParams(const uniform int colorSpace,
                            const varying float mask,
                            const varying DwaBaseParameters &params0,
                            const varying DwaBaseParameters &params1,
                            varying DwaBaseParameters &params)
{
    IridescenceParameters& iridescenceParams =  params.mIridescenceParameters;
    const IridescenceParameters &iridescenceParams0 = params0.mIridescenceParameters;
    const IridescenceParameters &iridescenceParams1 = params1.mIridescenceParameters;

    if (!isZero(params.mSpecular)) {
        iridescenceParams.mIridescence = lerp(iridescenceParams0.mIridescence,
                                              iridescenceParams1.mIridescence,
                                              mask);
        if (!isZero(iridescenceParams.mIridescence)) {
            if (!isZero(iridescenceParams0.mIridescence) &&
                !isZero(iridescenceParams1.mIridescence)) {

                if (iridescenceParams0.mIridescenceApplyTo != iridescenceParams1.mIridescenceApplyTo) {
                    // values are mixed: Apply to clearcoat/outer specular wins
                    iridescenceParams.mIridescenceApplyTo = IRIDESCENCE_OUTER_SPECULAR;
                } else {
                    iridescenceParams.mIridescenceApplyTo = iridescenceParams1.mIridescenceApplyTo;
                }
                if (iridescenceParams0.mIridescenceColorControl != iridescenceParams1.mIridescenceColorControl) {
                    // values are mixed: Ramp wins
                    iridescenceParams.mIridescenceColorControl = SHADING_IRIDESCENCE_COLOR_USE_RAMP;
                } else {
                    iridescenceParams.mIridescenceColorControl = iridescenceParams1.mIridescenceColorControl;
                }
                iridescenceParams.mIridescencePrimaryColor =
                        colorSpaceLerp(iridescenceParams0.mIridescencePrimaryColor,
                                       iridescenceParams1.mIridescencePrimaryColor,
                                       mask,
                                       colorSpace);
                iridescenceParams.mIridescenceSecondaryColor =
                        colorSpaceLerp(iridescenceParams0.mIridescenceSecondaryColor,
                                       iridescenceParams1.mIridescenceSecondaryColor,
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
                    iridescenceParams.mIridescenceRampInterpolationMode = COLOR_RAMP_CONTROL_SPACE_HSV;
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
                    iridescenceParams.mIridescenceRampColors[i] = lerp(iridescenceParams0.mIridescenceRampColors[i],
                                                                       iridescenceParams1.mIridescenceRampColors[i],
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
            } else { // params1.mIridescence > 0.0f
                copyIridescenceParams(iridescenceParams1, iridescenceParams);
            }
        }
    }
}

void blendMetallicParams(const uniform int colorSpace,
                         const varying float mask,
                         const varying DwaBaseParameters &params0,
                         const varying DwaBaseParameters &params1,
                         varying DwaBaseParameters &params)
{
    if (!isZero(params.mSpecular)) {
        params.mMetallic = lerp(params0.mMetallic,
                                params1.mMetallic,
                                mask);
        if (!isZero(params.mMetallic)) {
            if (!isZero(params0.mMetallic) &&
                !isZero(params1.mMetallic)) {

                params.mMetallicColor = colorSpaceLerp(params0.mMetallicColor,
                                                       params1.mMetallicColor,
                                                       mask,
                                                       colorSpace);
                params.mMetallicEdgeColor = colorSpaceLerp(params0.mMetallicEdgeColor,
                                                           params1.mMetallicEdgeColor,
                                                           mask,
                                                           colorSpace);
            } else if (!isZero(params0.mMetallic)) {
                params.mMetallicColor = params0.mMetallicColor;
                params.mMetallicEdgeColor = params0.mMetallicEdgeColor;
            } else { // params1.mMetallic > 0.0f
                params.mMetallicColor = params1.mMetallicColor;
                params.mMetallicEdgeColor = params1.mMetallicEdgeColor;
            }
        }
    }
}

void
copyFabricParams(const varying DwaBaseParameters &src,
                       varying DwaBaseParameters &dst)
{
    dst.mWarpColor = src.mWarpColor;
    dst.mWarpRoughness = src.mWarpRoughness;
    dst.mFabricTangent = src.mFabricTangent;
    dst.mWeftColor = src.mWeftColor;
    dst.mWeftRoughness = src.mWeftRoughness;
    dst.mWarpThreadDirection = src.mWarpThreadDirection;
    dst.mWarpThreadElevation = src.mWarpThreadElevation;
    dst.mWarpThreadCoverage = src.mWarpThreadCoverage;
}

void blendFabricParams(const uniform int colorSpace,
                       const varying float mask,
                       const varying DwaBaseParameters &params0,
                       const varying DwaBaseParameters &params1,
                       varying DwaBaseParameters &params)
{
    if (!isOne(params.mMetallic)) {
        params.mFabricSpecular = lerp(params0.mFabricSpecular,
                                      params1.mFabricSpecular,
                                      mask);

        if (!isZero(params.mFabricSpecular)) {
            if (!isZero(params0.mFabricSpecular) &&
                !isZero(params1.mFabricSpecular)) {

                params.mWarpColor = colorSpaceLerp(params0.mWarpColor,
                                                   params1.mWarpColor,
                                                   mask,
                                                   colorSpace);
                params.mWarpRoughness = lerp(params0.mWarpRoughness,
                                             params1.mWarpRoughness,
                                             mask);
                params.mFabricTangent = lerp(params0.mFabricTangent,
                                             params1.mFabricTangent,
                                             mask);
                params.mWeftColor = colorSpaceLerp(params0.mWeftColor,
                                                   params1.mWeftColor,
                                                   mask,
                                                   colorSpace);
                params.mWeftRoughness = lerp(params0.mWeftRoughness,
                                             params1.mWeftRoughness,
                                             mask);
                params.mWarpThreadDirection = normalize(lerp(params0.mWarpThreadDirection,
                                                             params1.mWarpThreadDirection,
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

void blendTransmissionParams(const uniform int colorSpace,
                             const varying float mask,
                             const varying DwaBaseParameters &params0,
                             const varying DwaBaseParameters &params1,
                             varying DwaBaseParameters &params)
{
    if (!isOne(params.mMetallic) &&
        !isZero(params.mFabricAttenuation)) {

        params.mTransmission = lerp(params0.mTransmission,
                                    params1.mTransmission,
                                    mask);
        if (!isZero(params.mTransmission)) {
            if (!isZero(params0.mTransmission) &&
                !isZero(params1.mTransmission)) {

                params.mTransmissionColor = colorSpaceLerp(params0.mTransmissionColor,
                                                           params1.mTransmissionColor,
                                                           mask,
                                                           colorSpace);
            } else if (!isZero(params0.mTransmission)) {
                params.mTransmissionColor = params0.mTransmissionColor;
            } else { // params1.mTransmission > 0.0f
                params.mTransmissionColor = params1.mTransmissionColor;
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

void copyToonDiffuseRampParams(const varying ToonDiffuseParameters& src,
                               varying ToonDiffuseParameters& dst)
{
    dst.mRampNumPoints = src.mRampNumPoints;
    for (int i = 0; i < src.mRampNumPoints; ++i) {
        dst.mRampPositions[i] = src.mRampPositions[i];
        dst.mRampColors[i] = src.mRampColors[i];
        dst.mRampInterpolators[i] = src.mRampInterpolators[i];
    }
    dst.mExtendRamp = src.mExtendRamp;
}

void copyToonDiffuseOrenNayarParams(const varying ToonDiffuseParameters& src,
                                    varying ToonDiffuseParameters& dst)
{
    dst.mTerminatorShift = src.mTerminatorShift;
    dst.mFlatness = src.mFlatness;
    dst.mFlatnessFalloff = src.mFlatnessFalloff;
}

void copyToonDiffuseParams(const varying ToonDiffuseParameters& src,
                           varying ToonDiffuseParameters& dst)
{
    dst.mModel = src.mModel;
    dst.mNormal = src.mNormal;
    copyToonDiffuseRampParams(src, dst);
    copyToonDiffuseOrenNayarParams(src, dst);
}

void blendToonDiffuseParams(const uniform int colorSpace,
                            const varying float mask,
                            const varying ToonDiffuseParameters &params0,
                            const varying ToonDiffuseParameters &params1,
                            varying ToonDiffuseParameters &params)
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
            if (params0.mModel == TOON_DIFFUSE_RAMP_V2 || params1.mModel == TOON_DIFFUSE_RAMP_V2) {
                params.mModel = TOON_DIFFUSE_RAMP_V2;
            } else {
                params.mModel = TOON_DIFFUSE_RAMP_V1;
            }
        } else {
            params.mModel = params1.mModel;
        }

        // Blend normals
        params.mNormal = normalize(lerp(params0.mNormal,
                                        params1.mNormal,
                                        mask));

        // Oren-nayer params
        if (params0.mModel == TOON_DIFFUSE_OREN_NAYAR &&
            (params1.mModel == TOON_DIFFUSE_RAMP_V1 || params1.mModel == TOON_DIFFUSE_RAMP_V2)) {
            copyToonDiffuseOrenNayarParams(params0, params);
        } else if ((params0.mModel == TOON_DIFFUSE_RAMP_V1 || params0.mModel == TOON_DIFFUSE_RAMP_V2) &&
                   params1.mModel == TOON_DIFFUSE_OREN_NAYAR) {
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
        if (params0.mModel == TOON_DIFFUSE_OREN_NAYAR &&
            (params1.mModel == TOON_DIFFUSE_RAMP_V1 || params1.mModel == TOON_DIFFUSE_RAMP_V2)) {
            copyToonDiffuseRampParams(params1, params);
        } else if ((params0.mModel == TOON_DIFFUSE_RAMP_V1 || params0.mModel == TOON_DIFFUSE_RAMP_V2) &&
                   params1.mModel == TOON_DIFFUSE_OREN_NAYAR) {
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
                params.mRampColors[i] = lerp(params0.mRampColors[i],
                                             params1.mRampColors[i],
                                             mask);
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

void copyToonSpecularParams(const varying ToonSpecularParameters& src,
                            varying ToonSpecularParameters& dst)
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

void blendToonSpecularParams(const uniform int colorSpace,
                             const varying float mask,
                             const varying ToonSpecularParameters &params0,
                             const varying ToonSpecularParameters &params1,
                             varying ToonSpecularParameters &params)
{
    params.mToonSpecular = lerp(params0.mToonSpecular,
                                params1.mToonSpecular,
                                mask);

    if (isZero(params.mToonSpecular)) {
        return;
    }

    if (!isZero(params0.mToonSpecular) &&
        !isZero(params1.mToonSpecular)) {

        params.mNormal = normalize(lerp(params0.mNormal,
                                        params1.mNormal,
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

        params.mTint = colorSpaceLerp(params0.mTint,
                                      params1.mTint,
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

        params.mdPds = lerp(params0.mdPds,
                            params1.mdPds,
                            mask);

        params.mdPdt = lerp(params0.mdPdt,
                            params1.mdPdt,
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

void blendToonParams(const uniform int colorSpace,
                     const varying float mask,
                     const varying DwaBaseParameters &params0,
                     const varying DwaBaseParameters &params1,
                     varying DwaBaseParameters &params)
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

void blendDiffuseParams(const uniform int colorSpace,
                        const varying float mask,
                        const varying DwaBaseParameters &params0,
                        const varying DwaBaseParameters &params1,
                        varying DwaBaseParameters &params)
{
    const bool mtl0HasDiffuse = (!isBlack(params0.mAlbedo));
    const bool mtl1HasDiffuse = (!isBlack(params1.mAlbedo));
    const bool blocked = (isOne(params.mMetallic) ||
                         isZero(params.mFabricAttenuation) ||
                         isOne(params.mTransmission));

    if (!blocked && (mtl0HasDiffuse || mtl1HasDiffuse)) {
        params.mAlbedo = colorSpaceLerp(params0.mAlbedo,
                                        params1.mAlbedo,
                                        mask,
                                        colorSpace);
        params.mDiffuseTransmission = lerp(params0.mDiffuseTransmission,
                                           params1.mDiffuseTransmission,
                                           mask);

        if (mtl0HasDiffuse && mtl1HasDiffuse) {
            params.mScatteringRadius = colorSpaceLerp(params0.mScatteringRadius,
                                                      params1.mScatteringRadius,
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
            params.mScatteringRadius = params1.mScatteringRadius;
            params.mCreaseAttenuation = params1.mCreaseAttenuation;
            params.mDiffuseTransmissionBlendingBehavior = params1.mDiffuseTransmissionBlendingBehavior;
            params.mDiffuseRoughness = params1.mDiffuseRoughness;
        } else {
            // only mtl0 has SSS
            params.mScatteringRadius = params0.mScatteringRadius;
            params.mCreaseAttenuation = params0.mCreaseAttenuation;
            params.mDiffuseTransmissionBlendingBehavior = params0.mDiffuseTransmissionBlendingBehavior;
            params.mDiffuseRoughness = params0.mDiffuseRoughness;
        }
    }
}

void blendRefractiveIndexParams(const varying float mask,
                                const varying DwaBaseParameters &params0,
                                const varying DwaBaseParameters &params1,
                                varying DwaBaseParameters &params)
{
    // If both materials are fabric, velvet, or a fabric/velvet blend, we don't need refractive index
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

void blendCommonParams(const uniform int colorSpace,
                       const varying float mask,
                       const varying DwaBaseParameters &params0,
                       const varying DwaBaseParameters &params1,
                       varying DwaBaseParameters &params)
{
    params.mDiffuseNormal = normalize(lerp(params0.mDiffuseNormal,
                                           params1.mDiffuseNormal,
                                           mask));
    params.mNormal = normalize(lerp(params0.mNormal,
                                    params1.mNormal,
                                    mask));
    params.mNormalLength = lerp(params0.mNormalLength,
                                params1.mNormalLength,
                                mask);
    if (params0.mNormalAAStrategy != params1.mNormalAAStrategy) {
        // values are mixed: Toksvig wins
        params.mNormalAAStrategy = NORMAL_AA_STRATEGY_TOKSVIG;
    } else {
        params.mNormalAAStrategy = params1.mNormalAAStrategy;
    }
    if (params.mNormalAAStrategy != NORMAL_AA_STRATEGY_NONE) {
        params.mNormalAADial = lerp(params0.mNormalAADial,
                                    params1.mNormalAADial,
                                    mask);
    }
    params.mEmission = colorSpaceLerp(params0.mEmission,
                                      params1.mEmission,
                                      mask,
                                      colorSpace);
}

void blendColorCorrectParams(const uniform int colorSpace,
                             const varying float mask,
                             const varying DwaBaseParameters &params0,
                             const varying DwaBaseParameters &params1,
                             varying DwaBaseParameters &params)
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
                const varying Color tmi0 = params0.mColorCorrectParams[cc].mTmi;
                const varying Color tmi1 = params1.mColorCorrectParams[cc].mTmi;
                params.mColorCorrectParams[cc].mTmi = colorSpaceLerp(tmi0,
                                                                     tmi1,
                                                                     mask,
                                                                     colorSpace);
            }
        }
    }
}

void blendAccentParams(const uniform int colorSpace,
                       const varying float mask,
                       const varying DwaBaseParameters &params0,
                       const varying DwaBaseParameters &params1,
                       varying DwaBaseParameters &params)
{
    params.mAccentParams.mSubsurface = lerp(params0.mAccentParams.mSubsurface,
                                            params1.mAccentParams.mSubsurface,
                                            mask);

    params.mAccentParams.mSubsurfaceNormal = normalize(lerp(params0.mAccentParams.mSubsurfaceNormal,
                                                            params1.mAccentParams.mSubsurfaceNormal,
                                                            mask));

    params.mAccentParams.mSubsurfaceNormalDial = lerp(params0.mAccentParams.mSubsurfaceNormalDial,
                                                      params1.mAccentParams.mSubsurfaceNormalDial,
                                                      mask);

    params.mAccentParams.mSubsurfaceColor = colorSpaceLerp(params0.mAccentParams.mSubsurfaceColor,
                                                           params1.mAccentParams.mSubsurfaceColor,
                                                           mask,
                                                           colorSpace);
}

//---------------------------------------------------------------------------
// Blend hair helper functions

void copyHairRParams(const varying HairParameters& src,
                           varying HairParameters& dst)
{
    dst.mHairShowR                  = src.mHairShowR;
    dst.mHairRShift                 = src.mHairRShift;
    dst.mHairRLongRoughness         = src.mHairRLongRoughness;
    dst.mHairRTint                  = src.mHairRTint;

}

void copyHairTTParams(const varying HairParameters& src,
                            varying HairParameters& dst)
{
    dst.mHairShowTT                 = src.mHairShowTT;
    dst.mHairTTShift                = src.mHairTTShift;
    dst.mHairTTLongRoughness        = src.mHairTTLongRoughness;
    dst.mHairTTAzimRoughness        = src.mHairTTAzimRoughness;
    dst.mHairTTTint                 = src.mHairTTTint;
    dst.mHairTTSaturation           = src.mHairTTSaturation;
}

void copyHairTRTParams(const varying HairParameters& src,
                             varying HairParameters& dst)
{
    dst.mHairShowTRT                = src.mHairShowTRT;
    dst.mHairTRTShift               = src.mHairTRTShift;
    dst.mHairTRTLongRoughness       = src.mHairTRTLongRoughness;
    dst.mHairTRTTint                = src.mHairTRTTint;
}

void copyHairGlintParams(const varying HairParameters& src,
                               varying HairParameters& dst)
{
    dst.mHairShowGlint              = src.mHairShowGlint;
    dst.mHairGlintRoughness         = src.mHairGlintRoughness;
    dst.mHairGlintMinTwists         = src.mHairGlintMinTwists;
    dst.mHairGlintMaxTwists         = src.mHairGlintMaxTwists;
    dst.mHairGlintEccentricity      = src.mHairGlintEccentricity;
    dst.mHairGlintSaturation        = src.mHairGlintSaturation;
}

void copyHairTRRTParams(const varying HairParameters& src,
                              varying HairParameters& dst)
{
    dst.mHairShowTRRT               = src.mHairShowTRRT;
    dst.mHairTRRTLongRoughness      = src.mHairTRRTLongRoughness;
}

void copyHairParams(const varying HairParameters& src,
                          varying HairParameters& dst)
{
    dst.mHairUV                     = src.mHairUV;
    dst.mHairIOR                    = src.mHairIOR;
    dst.mHairFresnelType            = src.mHairFresnelType;
    dst.mHairCuticleLayerThickness  = src.mHairCuticleLayerThickness;
    dst.mHairUseOptimizedSampling   = src.mHairUseOptimizedSampling;
    dst.mHairColor                  = src.mHairColor;

    copyHairRParams(src, dst);
    copyHairTTParams(src, dst);
    copyHairTRTParams(src, dst);
    copyHairTRRTParams(src, dst);
}

void copyHairDiffuseParams(const varying HairParameters& src,
                                 varying HairParameters& dst)
{
    dst.mHairDiffuseFrontColor                      = src.mHairDiffuseFrontColor;
    dst.mHairDiffuseBackColor                       = src.mHairDiffuseBackColor;
    dst.mHairDiffuseUseIndependentFrontAndBackColor = src.mHairDiffuseUseIndependentFrontAndBackColor;
}

// Hair Parameter Blending functions
// ----------------------------------------------------------------

void
blendEmissionParams(const uniform int colorSpace,
                    const varying float mask,
                    varying DwaBaseParameters& params0,
                    varying DwaBaseParameters& params1,
                    varying DwaBaseParameters& params)
{
    params.mEmission = colorSpaceLerp(params0.mEmission,
                                      params1.mEmission,
                                      mask,
                                      colorSpace);
}

void
blendHairCommonParams(const uniform int colorSpace,
                    const varying float mask,
                    varying HairParameters& params0,
                    varying HairParameters& params1,
                    varying HairParameters& params)
{
    params.mHairCastsCaustics = params0.mHairCastsCaustics || params1.mHairCastsCaustics;

    // The mHairDir should be the same in both paramsA and paramsB
    // since it is the geometry's dPds vector
    params.mHairDir = params1.mHairDir;
}

void
blendHairSpecParams(const uniform int colorSpace,
                    const varying float mask,
                    varying HairParameters& params0,
                    varying HairParameters& params1,
                    varying HairParameters& params)
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

    params.mHairUV = lerp(params0.mHairUV,
                          params1.mHairUV,
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
        params.mHairRTint = colorSpaceLerp(params0.mHairRTint,
                                           params1.mHairRTint,
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
                                   params1.mHairTTShift, mask);
        params.mHairTTLongRoughness = lerp(params0.mHairTTLongRoughness,
                                           params1.mHairTTLongRoughness,
                                           mask);
        params.mHairTTTint = colorSpaceLerp(params0.mHairTTTint,
                                            params1.mHairTTTint,
                                            mask,
                                            colorSpace);
        params.mHairTTSaturation = lerp(params0.mHairTTSaturation,
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
        params.mHairTRTTint = colorSpaceLerp(params0.mHairTRTTint,
                                            params1.mHairTRTTint,
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
        params.mHairColor = colorSpaceLerp(params0.mHairColor,
                                           params1.mHairColor,
                                           mask,
                                           colorSpace);
    } else if (!isOne(params0.mHairDiffuse)) {
        params.mHairColor = params0.mHairColor;
    } else {
        params.mHairColor = params1.mHairColor;
    }
}

void
blendHairDiffuseParams(const uniform int colorSpace,
                       const varying float mask,
                       varying DwaBaseParameters& params0,
                       varying DwaBaseParameters& params1,
                       varying DwaBaseParameters& params)
{
    const varying HairParameters hParams0 = params0.mHairParameters;
    const varying HairParameters hParams1 = params1.mHairParameters;

    params.mHairParameters.mHairDiffuse = lerp(hParams0.mHairDiffuse,
                                               hParams1.mHairDiffuse,
                                               mask);
    if (isZero(hParams1.mHairDiffuse) && !isZero(hParams0.mHairDiffuse)) {
        copyHairDiffuseParams(hParams0, params.mHairParameters);
        params.mNormal = params0.mNormal;
    } else if (isZero(hParams0.mHairDiffuse) && !isZero(hParams1.mHairDiffuse)){
        copyHairDiffuseParams(hParams1, params.mHairParameters);
        params.mNormal = params1.mNormal;
    } else {
        params.mHairParameters.mHairDiffuseUseIndependentFrontAndBackColor =
            hParams0.mHairDiffuseUseIndependentFrontAndBackColor ||
            hParams1.mHairDiffuseUseIndependentFrontAndBackColor;

        params.mHairParameters.mHairDiffuseFrontColor =
            colorSpaceLerp(hParams0.mHairDiffuseFrontColor,
                           hParams1.mHairDiffuseFrontColor,
                           mask,
                           colorSpace);

        params.mHairParameters.mHairDiffuseBackColor =
            colorSpaceLerp(hParams0.mHairDiffuseBackColor,
                           hParams1.mHairDiffuseBackColor,
                           mask,
                           colorSpace);

        params.mNormal = normalize(lerp(params0.mNormal,
                                        params1.mNormal,
                                        mask));
    }

    // experimental hair SSS
    const bool mtl0HasScatter = !isBlack(params0.mScatteringRadius)
                                && !isZero(hParams0.mHairSubsurfaceBlend);
    const bool mtl1HasScatter = !isBlack(params1.mScatteringRadius)
                                && !isZero(hParams1.mHairSubsurfaceBlend);

    // hair diffuse has an extra parameter to blend / choose kajiya kay or SSS (mHairSubsurfaceBlend)
    // if a submtl is diffuse but no scatter, treat it as 0 for blending
    // if a submtl is partly diffuse but no scatter, the blending toward 0 is weighted
    // if a submtl is not diffuse, copy it over from the diffuse
    // if both have scatter, blend normally
    if (mtl0HasScatter && !mtl1HasScatter) {
        params.mScatteringRadius = params0.mScatteringRadius;
        if (isZero(hParams1.mHairDiffuse)) {
            params.mHairParameters.mHairSubsurfaceBlend = hParams0.mHairSubsurfaceBlend;
        } else {
            params.mHairParameters.mHairSubsurfaceBlend = lerp(hParams0.mHairSubsurfaceBlend,
                                                               0.0f,
                                                               hParams1.mHairDiffuse * mask);
        }
    } else if (mtl1HasScatter && !mtl0HasScatter) {
        params.mScatteringRadius = params1.mScatteringRadius;
        if (isZero(hParams0.mHairDiffuse)) {
            params.mHairParameters.mHairSubsurfaceBlend = hParams1.mHairSubsurfaceBlend;
        } else {
            params.mHairParameters.mHairSubsurfaceBlend = lerp(hParams1.mHairSubsurfaceBlend,
                                                               0.0f,
                                                               hParams0.mHairDiffuse * (1.0f - mask));
        }
    } else {
        params.mScatteringRadius = colorSpaceLerp(params0.mScatteringRadius,
                                                  params1.mScatteringRadius,
                                                  mask,
                                                  colorSpace);
        params.mHairParameters.mHairSubsurfaceBlend = lerp(hParams0.mHairSubsurfaceBlend,
                                                           hParams1.mHairSubsurfaceBlend,
                                                           mask);
    }

}

void blendHairToonParams(const uniform int colorSpace,
                         const varying float mask,
                         const varying DwaBaseParameters &params0,
                         const varying DwaBaseParameters &params1,
                         varying DwaBaseParameters &params)
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

inline bool
resolveSingleMaterialParameters(const uniform Material* uniform material,
                                uniform ShadingTLState *uniform tls,
                                const varying State& state,
                                const uniform bool castsCaustics,
                                varying DwaBaseParameters* uniform params,
                                const uniform GLITTER_Glitter * uniform glitterPtr,
                                const uniform GLITTER_UniformParameters * uniform uniformParams,
                                const DWABASELAYERABLE_ResolveParametersFunc resolveFn,
                                const uniform bool material0HasGlitter,
                                const uniform bool material1HasGlitter)

{
    // DwaBaseParameters stores a pointer to a single Glitter
    // object from which we call createLobes.   The uniform
    // parameters for this object are set during construction
    // so we cannot blend them at shade time. Therefore, if
    // both materials have glitter enabled we use the Glitter
    // object and uniform glitter parameters stored on this
    // layer material.
    if (material0HasGlitter && material1HasGlitter) {
        bool result = resolveFn(material, tls, state, castsCaustics, params);
        if (result) {
            if (glitterPtr) {
                params->mGlitterPointerVector = glitterPtr;
                params->mGlitterUniformParameters = uniformParams;
            } else {
                result = false;
            }
        }
        return result;
    } else {
        return resolveFn(material, tls, state, castsCaustics, params);
    }
}

//---------------------------------------------------------------------------


bool
DWABASE_blendParameters(const uniform Material* uniform me,
                        uniform ShadingTLState *uniform tls,
                        const varying State& state,
                        const uniform bool castsCaustics,
                        varying DwaBaseParameters* uniform params,
                        const uniform DwaBaseUniformParameters* uniform uParams,
                        const uniform int colorSpace,
                        const uniform GLITTER_Glitter * uniform glitterPtr,
                        const uniform GLITTER_UniformParameters * uniform glitterUniformParams,
                        uniform intptr_t evalSubsurfaceNormalFn,
                        const uniform SubMtlData& subMtl0,
                        const uniform SubMtlData& subMtl1,
                        const varying float mask)
{
    const DWABASELAYERABLE_ResolveParametersFunc resolveFn0 =
            (DWABASELAYERABLE_ResolveParametersFunc) subMtl0.mResolveParametersFunc;
    const DWABASELAYERABLE_ResolveParametersFunc resolveFn1 =
            (DWABASELAYERABLE_ResolveParametersFunc) subMtl1.mResolveParametersFunc;

    const DWABASELAYERABLE_ResolvePresenceFunc presenceFn0 =
            (DWABASELAYERABLE_ResolvePresenceFunc) subMtl0.mResolvePresenceFunc;
    const DWABASELAYERABLE_ResolvePresenceFunc presenceFn1 =
            (DWABASELAYERABLE_ResolvePresenceFunc) subMtl1.mResolvePresenceFunc;

    const uniform Material* uniform material0 = (const uniform Material * uniform) subMtl0.mDwaBaseLayerable;
    const uniform Material* uniform material1 = (const uniform Material * uniform) subMtl1.mDwaBaseLayerable;

    // The uniform parameters were blended in update()


    // First verify that any submaterials that are connected are
    // DwaBaseLayerable, otherwise exit early
    if (!material0) {
        // exit early, (background material / first material) is missing or not Layerable
        return false;
    } else if (!material1 || allActive(isZero(mask))) {
        // early exit
        return resolveSingleMaterialParameters(material0, tls, state,
                                               castsCaustics,
                                               params,
                                               glitterPtr,
                                               glitterUniformParams,
                                               resolveFn0,
                                               subMtl0.mHasGlitter,
                                               subMtl1.mHasGlitter);
    } else if (allActive(isOne(mask))) {
        // early exit
        return resolveSingleMaterialParameters(material1, tls, state,
                                               castsCaustics,
                                               params,
                                               glitterPtr,
                                               glitterUniformParams,
                                               resolveFn1,
                                               subMtl0.mHasGlitter,
                                               subMtl1.mHasGlitter);
    }

    // resolve both sub-material params and blend
    DwaBaseParameters params0, params1;
    DWABASELAYERABLE_copyColorCorrectParameters(params, &params0);
    DWABASELAYERABLE_copyColorCorrectParameters(params, &params1);

    bool success = resolveFn0(material0, tls, state, castsCaustics, &params0) &&
                   resolveFn1(material1, tls, state, castsCaustics, &params1);
    if (!success) { return false; }

    // make sure all fields are initialized
    DWABASELAYERABLE_initParameters(params);

    blendGlitterParams(colorSpace, mask,
                       glitterPtr, glitterUniformParams,
                       params0, params1, *params);
    blendCommonParams(colorSpace, mask, params0, params1, *params);
    blendFuzzParams(colorSpace, mask, params0, params1, *params);
    blendOuterSpecularParams(colorSpace, mask, params0, params1, *params);
    blendCommonSpecularParams(mask, params0, params1, *params);
    blendIridescenceParams(colorSpace, mask, params0, params1, *params);
    blendMetallicParams(colorSpace, mask, params0, params1, *params);
    blendColorCorrectParams(colorSpace, mask, params0, params1, *params);
    blendAccentParams(colorSpace, mask, params0, params1, *params);

    // Always blend params.mFabricAttenuation before calling
    // blendRefractiveIndex, blendTransmission, blendDiffuse
    params->mFabricAttenuation = lerp(params0.mFabricAttenuation, params1.mFabricAttenuation, mask);

    blendFabricParams(colorSpace, mask, params0, params1, *params);
    blendRefractiveIndexParams(mask, params0, params1, *params);
    blendTransmissionParams(colorSpace, mask, params0, params1, *params);

    // Blend Toon
    blendToonParams(colorSpace, mask, params0, params1, *params);

    blendDiffuseParams(colorSpace, mask, params0, params1, *params);

    params->mEvalSubsurfaceNormalFn = evalSubsurfaceNormalFn;

    return true;
}

bool
DWABASE_blendHairParameters(const uniform Material* uniform me,
                            uniform ShadingTLState *uniform tls,
                            const varying State& state,
                            const uniform bool castsCaustics,
                            varying DwaBaseParameters* uniform params,
                            const uniform DwaBaseUniformParameters* uniform uParams,
                            const uniform int colorSpace,
                            uniform intptr_t evalSubsurfaceNormalFn,
                            const uniform SubMtlData& subMtl0,
                            const uniform SubMtlData& subMtl1,
                            uniform int errorMismatchedFresnelType,
                            const varying float mask)
{
    // Get Access to Function Pointers to the Resolve() and CastsCaustics() Functions
    const DWABASELAYERABLE_ResolveParametersFunc resolveFn0 =
            (DWABASELAYERABLE_ResolveParametersFunc) subMtl0.mResolveParametersFunc;
    const DWABASELAYERABLE_ResolveParametersFunc resolveFn1 =
            (DWABASELAYERABLE_ResolveParametersFunc) subMtl1.mResolveParametersFunc;

    const uniform DWABASELAYERABLE_CastsCausticsFunc castsCausticsFn0 =
            (const uniform DWABASELAYERABLE_CastsCausticsFunc) subMtl0.mGetCastsCausticsFunc;
    const uniform DWABASELAYERABLE_CastsCausticsFunc castsCausticsFn1 =
            (const uniform DWABASELAYERABLE_CastsCausticsFunc) subMtl1.mGetCastsCausticsFunc;

    const DWABASELAYERABLE_ResolvePresenceFunc presenceFn0 =
            (DWABASELAYERABLE_ResolvePresenceFunc) subMtl0.mResolvePresenceFunc;
    const DWABASELAYERABLE_ResolvePresenceFunc presenceFn1 =
            (DWABASELAYERABLE_ResolvePresenceFunc) subMtl1.mResolvePresenceFunc;

    const uniform Material* uniform material0 =(const uniform Material * uniform) subMtl0.mDwaBaseLayerable;
    const uniform Material* uniform material1 =(const uniform Material * uniform) subMtl1.mDwaBaseLayerable;

    // mSubsurface is blended in update() as part of
    // blendUniformParameters()

    // First verify that any submaterials that are connected are
    // DwaBaseLayerable, otherwise exit early
    if (!material0) {
        // exit early, background material is missing or not Layerable
        return false;
    } else if (!material1 || allActive(isZero(mask))) {
        // early exit
        return resolveFn0(material0, tls, state, castsCausticsFn0(material0), params);
    } else if (allActive(isOne(mask))) {
        // early exit
        return resolveFn1(material1, tls, state, castsCausticsFn1(material1), params);
    }
    // resolve both sub-material params and blend
    uniform bool casts0 = castsCausticsFn0(material0);
    uniform bool casts1 = castsCausticsFn1(material1);
    uniform bool layerCastsCaustics = casts0 || casts1;

    DwaBaseParameters params0, params1;
    DWABASELAYERABLE_copyColorCorrectParameters(params, &params0);
    DWABASELAYERABLE_copyColorCorrectParameters(params, &params1);

    bool success = resolveFn0(material0, tls, state, layerCastsCaustics, &params0) &&
                   resolveFn1(material1, tls, state, layerCastsCaustics, &params1);
    if (!success) { return false; }

    // make sure all fields are initialized
    DWABASELAYERABLE_initParameters(params);

    if (!isZero(params0.mHairParameters.mHair) && !isZero(params1.mHairParameters.mHair) &&
        params0.mHairParameters.mHairFresnelType != params1.mHairParameters.mHairFresnelType) {
        logEvent(me, errorMismatchedFresnelType);
        return false;
    }

    blendEmissionParams(colorSpace,
                        mask,
                        params0,
                        params1,
                        *params);

    blendHairCommonParams(colorSpace,
                          mask,
                          params0.mHairParameters,
                          params1.mHairParameters,
                          params->mHairParameters);

    blendHairSpecParams(colorSpace,
                        mask,
                        params0.mHairParameters,
                        params1.mHairParameters,
                        params->mHairParameters);

    blendHairDiffuseParams(colorSpace,
                           mask,
                           params0,
                           params1,
                           *params);

    blendHairToonParams(colorSpace,
                        mask,
                        params0,
                        params1,
                        *params);

    params->mEvalSubsurfaceNormalFn = evalSubsurfaceNormalFn;
    return true;
}

float
DWABASE_blendPresence(uniform ShadingTLState *uniform tls,
                      const varying State& state,
                      const uniform SubMtlData& subMtl0,
                      const uniform SubMtlData& subMtl1,
                      varying float mask)
{
    const DWABASELAYERABLE_ResolvePresenceFunc presenceFn0 =
            (DWABASELAYERABLE_ResolvePresenceFunc) subMtl0.mResolvePresenceFunc;
    const DWABASELAYERABLE_ResolvePresenceFunc presenceFn1 =
            (DWABASELAYERABLE_ResolvePresenceFunc) subMtl1.mResolvePresenceFunc;
    const uniform Material* uniform material0 =(const uniform Material * uniform) subMtl0.mDwaBaseLayerable;
    const uniform Material* uniform material1 =(const uniform Material * uniform) subMtl1.mDwaBaseLayerable;

    // First verify that any submaterials that are connected are
    // DwaBaseLayerable, otherwise exit early
    if (!material0) {
        // exit early, (background material / first material) is missing or not Layerable
        return 1.f;
    }
    else if (!material1 || isZero(mask)) {
        return presenceFn0(material0, tls, state);
    }
    else if (isOne(mask)) {
        return presenceFn1(material1, tls, state);
    }

    const varying float presence0 = presenceFn0(material0, tls, state);
    const varying float presence1 = presenceFn1(material1, tls, state);

    return lerp(presence0, presence1, mask);
}

// TODO: This function is not tested and not completed because subsurface
// scattering currently goes through the scalar integrator. In particular, this
// might not properly handle the "enable sss input normal" toggle.
Vec3f
DWABASE_blendSubsurfaceNormal(uniform ShadingTLState *uniform tls,
                              const varying State& state,
                              const uniform SubMtlData& subMtl0,
                              const uniform SubMtlData& subMtl1,
                              varying float mask)
{
    const DWABASELAYERABLE_resolveSubsurfaceNormalFunc normalFn0 =
            (DWABASELAYERABLE_resolveSubsurfaceNormalFunc) subMtl0.mResolveSubsurfaceNormalFunc;
    const DWABASELAYERABLE_resolveSubsurfaceNormalFunc normalFn1 =
            (DWABASELAYERABLE_resolveSubsurfaceNormalFunc) subMtl1.mResolveSubsurfaceNormalFunc;
    const uniform Material* uniform material0 =
            (const uniform Material * uniform) subMtl0.mDwaBaseLayerable;
    const uniform Material* uniform material1 =
            (const uniform Material * uniform) subMtl1.mDwaBaseLayerable;

    // Verify that any submaterials that are connected are
    // DwaBaseLayerable, and verify the mask is not binary 0|1,
    // otherwise exit early
    if (!material0) {
        return normalFn1(material1, tls, state);
    }
    else if (!material1) {
        return normalFn0(material0, tls, state);
    }

    const varying Vec3f normal0 = normalFn0(material0, tls, state);
    const varying Vec3f normal1 = normalFn1(material1, tls, state);

    const varying Vec3f normal = normalize(lerp(normal0, normal1, mask));
}

