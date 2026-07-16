// Copyright 2023-2024 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

///
/// @file DwaBaseLayerable.h
/// $Id$
///

#pragma once

#include "DwaBaseLayerable_ispc_stubs.h"

#include <moonray/common/mcrt_macros/moonray_static_check.h>
#include <moonray/rendering/shading/MaterialApi.h>
#include <moonshine/material/glitter/Glitter.h>
#include <scene_rdl2/scene/rdl2/rdl2.h>
#include <scene_rdl2/common/math/Color.h>
#include <scene_rdl2/common/platform/HybridUniformData.h>
#include <moonray/rendering/shading/AttributeKey.h>

#include <iostream>

namespace moonshine {
namespace dwabase {

finline std::ostream& operator<<(std::ostream& os,
                                 const ispc::GLITTER_UniformParameters &p)
{
    return os
        << "mSpace: " << p.mSpace << "\n"
        << "mFlakeRandomness: " << p.mFlakeRandomness << "\n"
        << "mDenseGlitterLodQuality: " << p.mDenseGlitterLodQuality << "\n"
        << "mSearchRadiusFactor: " << p.mSearchRadiusFactor << "\n"
        << "mLayeringMode: " << p.mLayeringMode << "\n";
}

finline std::ostream& operator<<(std::ostream& os,
                                 const ispc::GLITTER_VaryingParameters& p)
{
    return os
        << "mFlakeStyleFrequency[0]: " << p.mFlakeStyleFrequency[0] << "\n"
        << "mFlakeColor[0]: "
            << scene_rdl2::math::asCpp(p.mFlakeColor[0]).r << " "
            << scene_rdl2::math::asCpp(p.mFlakeColor[0]).g << " "
            << scene_rdl2::math::asCpp(p.mFlakeColor[0]).b << " " << "\n"
        << "mFlakeSize[0]: " << p.mFlakeSize[0] << "\n"
        << "mFlakeRoughness[0]: " << p.mFlakeRoughness[0] << "\n"
        << "mFlakeStyleFrequency[1]: " << p.mFlakeStyleFrequency[1] << "\n"
        << "mFlakeColor[1]: "
            << scene_rdl2::math::asCpp(p.mFlakeColor[1]).r << " "
            << scene_rdl2::math::asCpp(p.mFlakeColor[1]).g << " "
            << scene_rdl2::math::asCpp(p.mFlakeColor[1]).b << " " << "\n"
        << "mFlakeSize[1]: " << p.mFlakeSize[1] << "\n"
        << "mFlakeRoughness[1]: " << p.mFlakeRoughness[1] << "\n"
        << "mFlakeHSVColorVariation: "
            << scene_rdl2::math::asCpp(p.mFlakeHSVColorVariation).x << " "
            << scene_rdl2::math::asCpp(p.mFlakeHSVColorVariation).y << " "
            << scene_rdl2::math::asCpp(p.mFlakeHSVColorVariation).z << " " << "\n"
        << "mFlakeDensity: " << p.mFlakeDensity << "\n"
        << "mFlakeJitter: " << p.mFlakeJitter << "\n"
        << "mFlakeOrientationRandomness: " << p.mFlakeOrientationRandomness << "\n"
        << "mCompensateDeformation: " << p.mCompensateDeformation << "\n"
        << "mApproximateForSecRays: " << p.mApproximateForSecRays << "\n"
        << "mGlitterMask: " << p.mGlitterMask << "\n";
}

finline std::ostream& operator<<(std::ostream& os,
                                 const ispc::HairParameters& p)
{
    return os
        // Common Hair Params
        << "mHairDir: "
            << scene_rdl2::math::asCpp(p.mHairDir).x << " "
            << scene_rdl2::math::asCpp(p.mHairDir).y << " "
            << scene_rdl2::math::asCpp(p.mHairDir).z << " " << "\n"
        << "mHairColor: "
            << scene_rdl2::math::asCpp(p.mHairColor).r << " "
            << scene_rdl2::math::asCpp(p.mHairColor).g << " "
            << scene_rdl2::math::asCpp(p.mHairColor).b << " " << "\n"
        << "mHair: " << p.mHair << "\n"
        << "mHairDiffuse: " << p.mHairDiffuse << "\n"

        // Hair params
        << "mHairCastsCaustics: " << p.mHairCastsCaustics << "\n"
        << "mHairHairUV: "
            << scene_rdl2::math::asCpp(p.mHairUV).x << " "
            << scene_rdl2::math::asCpp(p.mHairUV).y << " " << "\n"
        << "mHairIOR: " << p.mHairIOR << "\n"
        << "mHairShowR: " << p.mHairShowR << "\n"
        << "mHairRShift: " << p.mHairRShift << "\n"
        << "mHairRLongRoughness: " << p.mHairRLongRoughness << "\n"
        << "mHairRTint: "
            << scene_rdl2::math::asCpp(p.mHairRTint).r << " "
            << scene_rdl2::math::asCpp(p.mHairRTint).g << " "
            << scene_rdl2::math::asCpp(p.mHairRTint).b << " " << "\n"
        << "mHairShowTT: " << p.mHairShowTT << "\n"
        << "mHairTTShift: " << p.mHairTTShift << "\n"
        << "mHairTTLongRoughness: " << p.mHairTTLongRoughness << "\n"
        << "mHairTTAzimRoughness: " << p.mHairTTAzimRoughness << "\n"
        << "mHairTTTint: "
            << scene_rdl2::math::asCpp(p.mHairTTTint).r << " "
            << scene_rdl2::math::asCpp(p.mHairTTTint).g << " "
            << scene_rdl2::math::asCpp(p.mHairTTTint).b << " " << "\n"
        << "mHairTTSaturation: " << p.mHairTTSaturation << "\n"
        << "mHairShowTRT: " << p.mHairShowTRT << "\n"
        << "mHairTRTShift: " << p.mHairTRTShift << "\n"
        << "mHairTRTLongRoughness: " << p.mHairTRTLongRoughness << "\n"
        << "mHairTRTTint: "
            << scene_rdl2::math::asCpp(p.mHairTRTTint).r << " "
            << scene_rdl2::math::asCpp(p.mHairTRTTint).g << " "
            << scene_rdl2::math::asCpp(p.mHairTRTTint).b << " " << "\n"
        << "mHairShowGlint: " << p.mHairShowGlint << "\n"
        << "mHairGlintRoughness: " << p.mHairGlintRoughness << "\n"
        << "mHairGlintMinTwists: " << p.mHairGlintMinTwists << "\n"
        << "mHairGlintMaxTwists: " << p.mHairGlintMaxTwists << "\n"
        << "mHairGlintEccentricity: " << p.mHairGlintEccentricity << "\n"
        << "mHairGlintSaturation: " << p.mHairGlintSaturation << "\n"
        << "mHairShowTRRT: " << p.mHairShowTRRT << "\n"
        << "mHairTRRTLongRoughness: " << p.mHairTRRTLongRoughness << "\n"
        << "mHairFresnelType: " << p.mHairFresnelType << "\n"
        << "mHairCuticleLayerThickness: " << p.mHairCuticleLayerThickness << "\n"
        << "mHairUseOptimizedSampling: " << p.mHairUseOptimizedSampling << "\n"

        // HairDiffuse params
        << "mHairDiffuseFrontColor: "
            << scene_rdl2::math::asCpp(p.mHairDiffuseFrontColor).r << " "
            << scene_rdl2::math::asCpp(p.mHairDiffuseFrontColor).g << " "
            << scene_rdl2::math::asCpp(p.mHairDiffuseFrontColor).b << " " << "\n"
        << "mHairDiffuseBackColor: "
            << scene_rdl2::math::asCpp(p.mHairDiffuseBackColor).r << " "
            << scene_rdl2::math::asCpp(p.mHairDiffuseBackColor).g << " "
            << scene_rdl2::math::asCpp(p.mHairDiffuseBackColor).b << " " << "\n"
        << "mHairDiffuseUseIndependentFrontAndBackColor: "
            << p.mHairDiffuseUseIndependentFrontAndBackColor << "\n";
}

finline std::ostream& operator<<(std::ostream& os,
                                 const ispc::ToonSpecularParameters& p)
{
    return os
        << "\tmToonSpecular: " << p.mToonSpecular << "\n"
        << "\tmIntensity: " << p.mIntensity << "\n"
        << "\tmFresnelBlend: " << p.mFresnelBlend << "\n"
        << "\tmRoughness: " << p.mRoughness << "\n"
        << "\tmTint: "
            << scene_rdl2::math::asCpp(p.mTint).r << " "
            << scene_rdl2::math::asCpp(p.mTint).g << " "
            << scene_rdl2::math::asCpp(p.mTint).b << " " << "\n"
        << "\tmRampNumPoints: " << p.mRampNumPoints << "\n"
        << "\tmNormal: "
            << scene_rdl2::math::asCpp(p.mNormal).x << " "
            << scene_rdl2::math::asCpp(p.mNormal).y << " "
            << scene_rdl2::math::asCpp(p.mNormal).z << " " << "\n"
        << "\tmStretchU: " << p.mStretchU << "\n"
        << "\tmStretchV: " << p.mStretchV << "\n"
        << "\tmdPds: "
            << scene_rdl2::math::asCpp(p.mdPds).x << " "
            << scene_rdl2::math::asCpp(p.mdPds).y << " "
            << scene_rdl2::math::asCpp(p.mdPds).z << " " << "\n"
        << "\tmdPdt: "
            << scene_rdl2::math::asCpp(p.mdPdt).x << " "
            << scene_rdl2::math::asCpp(p.mdPdt).y << " "
            << scene_rdl2::math::asCpp(p.mdPdt).z << " " << "\n"
        << "\tmEnableIndirectReflections: " << p.mEnableIndirectReflections << "\n"
        << "\tmIndirectReflectionsIntensity: " << p.mIndirectReflectionsIntensity << "\n"
        << "\tmIndirectReflectionsRoughness: " << p.mIndirectReflectionsRoughness << "\n";
}

finline std::ostream& operator<<(std::ostream& os,
                                 const ispc::ToonDiffuseParameters& p)
{
    return os
        << "mToonDiffuse: " << p.mToonDiffuse << "\n"
        << "mModel: " << p.mModel << "\n"
        << "mTerminatorShift: " << p.mTerminatorShift << "\n"
        << "mFlatness: " << p.mFlatness << "\n"
        << "mFlatnessFalloff: " << p.mFlatnessFalloff << "\n"
        << "mRampWeight: " << p.mRampWeight << "\n"
        << "mRampNumPoints: " << p.mRampNumPoints << "\n"
        << "mExtendRamp: " << p.mExtendRamp << "\n"
        << "mRampInputScale: " << p.mRampInputScale << "\n"
        << "mNormal: "
            << scene_rdl2::math::asCpp(p.mNormal).x << " "
            << scene_rdl2::math::asCpp(p.mNormal).y << " "
            << scene_rdl2::math::asCpp(p.mNormal).z << " " << "\n";
}

finline std::ostream& operator<<(std::ostream& os,
                                 const ispc::AccentParameters& p)
{
    return os
        << "mSubsurface: " << p.mSubsurface << "\n"
        << "mSubsurfaceNormal: "
        << scene_rdl2::math::asCpp(p.mSubsurfaceNormal).x << " "
        << scene_rdl2::math::asCpp(p.mSubsurfaceNormal).y << " "
        << scene_rdl2::math::asCpp(p.mSubsurfaceNormal).z << " " << "\n"
        << "mSubsurfaceNormalDial: " << p.mSubsurfaceNormalDial << "\n"
        << "mSubsurfaceColor: "
        << scene_rdl2::math::asCpp(p.mSubsurfaceColor).r << " "
        << scene_rdl2::math::asCpp(p.mSubsurfaceColor).g << " "
        << scene_rdl2::math::asCpp(p.mSubsurfaceColor).b << " " << "\n";
}

finline std::ostream& operator<<(std::ostream& os,
                                 const ispc::DwaBaseUniformParameters& p)
{
    return os
        << "mOuterSpecularModel: " << p.mOuterSpecularModel << "\n"
        << "mOuterSpecularBending: " << p.mOuterSpecularUseBending << "\n"
        << "mSpecularModel: " << p.mSpecularModel << "\n"
        << "mSubsurface: " << p.mSubsurface << "\n"
        << "mThinGeometry: " << p.mThinGeometry << "\n"
        << "mPreventLightCulling: " << p.mPreventLightCulling << "\n";
}

finline std::ostream& operator<<(std::ostream& os,
                                 const ispc::DwaBaseParameters& p)
{

    // Glitter params

    /* the following line is commented out because it can crash scenes without glitter
     * since it's just a debugging line, it's left in if you need it
     */
    // std::cout << *(p.mGlitterUniformParameters) << "\n";
    std::cout << p.mGlitterVaryingParameters << "\n";

    // Hair params
    std::cout << p.mHairParameters << "\n";

    // Hair toon params
    std::cout << "mHairToonS1:" << "\n" << p.mHairToonS1Params << "\n";
    std::cout << "mHairToonS2:" << "\n" << p.mHairToonS2Params << "\n";
    std::cout << "mHairToonS3:" << "\n" << p.mHairToonS3Params << "\n";

    // Toon params
    std::cout << "mToonDiffuseParams" << "\n" << p.mToonDiffuseParams << "\n";
    std::cout << "mToonSpecularParams" << "\n" << p.mToonSpecularParams << "\n";

    return os
        // Fuzz params
        << "mFuzz: " << p.mFuzz << "\n"
        << "mFuzzRoughness: " << p.mFuzzRoughness << "\n"
        << "mFuzzAlbedo: "
            << scene_rdl2::math::asCpp(p.mFuzzAlbedo).r << " "
            << scene_rdl2::math::asCpp(p.mFuzzAlbedo).g << " "
            << scene_rdl2::math::asCpp(p.mFuzzAlbedo).b << " " << "\n"
        << "mFuzzUseAbsorbingFibers: " << p.mFuzzUseAbsorbingFibers << "\n"
        << "mFuzzNormal: "
            << scene_rdl2::math::asCpp(p.mFuzzNormal).x << " "
            << scene_rdl2::math::asCpp(p.mFuzzNormal).y << " "
            << scene_rdl2::math::asCpp(p.mFuzzNormal).z << " " << "\n"
        // OuterSpecular params
        << "mOuterSpecular: " << p.mOuterSpecular << "\n"
        << "mOuterSpecularRefractiveIndex: " << p.mOuterSpecularRefractiveIndex << "\n"
        << "mOuterSpecularRoughness: " << p.mOuterSpecularRoughness << "\n"
        << "mOuterSpecularThickness: " << p.mOuterSpecularThickness << "\n"
        << "mOuterSpecularAttenuationColor: "
            << scene_rdl2::math::asCpp(p.mOuterSpecularAttenuationColor).r << " "
            << scene_rdl2::math::asCpp(p.mOuterSpecularAttenuationColor).g << " "
            << scene_rdl2::math::asCpp(p.mOuterSpecularAttenuationColor).b << " " << "\n"
        << "mOuterSpecularNormal: "
            << scene_rdl2::math::asCpp(p.mOuterSpecularNormal).x << " "
            << scene_rdl2::math::asCpp(p.mOuterSpecularNormal).y << " "
            << scene_rdl2::math::asCpp(p.mOuterSpecularNormal).z << " " << "\n"
        << "mOuterSpecularNormalDial: " << p.mOuterSpecularNormalDial << "\n"
        // metallic params
        << "mMetallic: " << p.mMetallic << "\n"
        << "mMetallicColor: "
            << scene_rdl2::math::asCpp(p.mMetallicColor).r << " "
            << scene_rdl2::math::asCpp(p.mMetallicColor).g << " "
            << scene_rdl2::math::asCpp(p.mMetallicColor).b << " " << "\n"
        << "mMetallicEdgeColor: "
            << scene_rdl2::math::asCpp(p.mMetallicEdgeColor).r << " "
            << scene_rdl2::math::asCpp(p.mMetallicEdgeColor).g << " "
            << scene_rdl2::math::asCpp(p.mMetallicEdgeColor).b << " " << "\n"
        // specular params
        << "mSpecular: " << p.mSpecular << "\n"
        << "mRefractiveIndex: " << p.mRefractiveIndex << "\n"
        << "mRoughness: " << p.mRoughness << "\n"
        << "mAnisotropy: " << p.mAnisotropy << "\n"
        << "mShadingTangent: "
            << scene_rdl2::math::asCpp(p.mShadingTangent).x << " "
            << scene_rdl2::math::asCpp(p.mShadingTangent).y << " " << "\n"
        // fabric params
        << "mFabricSpecular: " << p.mFabricSpecular << "\n"
        << "mWarpColor: "
        << scene_rdl2::math::asCpp(p.mWarpColor).r << " "
            << scene_rdl2::math::asCpp(p.mWarpColor).g << " "
            << scene_rdl2::math::asCpp(p.mWarpColor).b << " " << "\n"
        << "mWarpRoughness: " << p.mWarpRoughness << "\n"
        << "mUseIndependentWeftAttributes: " << p.mUseIndependentWeftAttributes << "\n"
        << "mWeftRoughness: " << p.mWeftRoughness << "\n"
        << "mWeftColor: "
            << scene_rdl2::math::asCpp(p.mWeftColor).r << " "
            << scene_rdl2::math::asCpp(p.mWeftColor).g << " "
            << scene_rdl2::math::asCpp(p.mWeftColor).b << " " << "\n"
        << "mWarpThreadDirection: "
            << scene_rdl2::math::asCpp(p.mWarpThreadDirection).x << " "
            << scene_rdl2::math::asCpp(p.mWarpThreadDirection).y << " "
            << scene_rdl2::math::asCpp(p.mWarpThreadDirection).z << " " << "\n"
        << "mWarpThreadCoverage: " << p.mWarpThreadCoverage << "\n"
        << "mWarpThreadElevation: " << p.mWarpThreadElevation << "\n"
        << "mFabricAttenuation: " << p.mFabricAttenuation << "\n"
        << "mFabricTangent: "
            << scene_rdl2::math::asCpp(p.mFabricTangent).x << " "
            << scene_rdl2::math::asCpp(p.mFabricTangent).y << " "
            << scene_rdl2::math::asCpp(p.mFabricTangent).z << " " << "\n"
        // iridescence params
        << "mIridescence: " << p.mIridescenceParameters.mIridescence << "\n"
        << "mIridescenceApplyTo: " << p.mIridescenceParameters.mIridescenceApplyTo << "\n"
        << "mIridescenceColorControl: " << p.mIridescenceParameters.mIridescenceColorControl << "\n"
        << "mIridescencePrimaryColor: "
            << scene_rdl2::math::asCpp(p.mIridescenceParameters.mIridescencePrimaryColor).r << " "
            << scene_rdl2::math::asCpp(p.mIridescenceParameters.mIridescencePrimaryColor).g << " "
            << scene_rdl2::math::asCpp(p.mIridescenceParameters.mIridescencePrimaryColor).b << " " << "\n"
        << "mIridescenceSecondaryColor: "
            << scene_rdl2::math::asCpp(p.mIridescenceParameters.mIridescenceSecondaryColor).r << " "
            << scene_rdl2::math::asCpp(p.mIridescenceParameters.mIridescenceSecondaryColor).g << " "
            << scene_rdl2::math::asCpp(p.mIridescenceParameters.mIridescenceSecondaryColor).b << " " << "\n"
        << "mIridescenceFlipHueDirection: " << p.mIridescenceParameters.mIridescenceFlipHueDirection << "\n"
        << "mIridescenceThickness: " << p.mIridescenceParameters.mIridescenceThickness << "\n"
        << "mIridescenceExponent: " << p.mIridescenceParameters.mIridescenceExponent << "\n"
        << "mIridescenceAt0: " << p.mIridescenceParameters.mIridescenceAt0 << "\n"
        << "mIridescenceAt90: " << p.mIridescenceParameters.mIridescenceAt90 << "\n"
        << "mIridescenceRampInterpolationMode: " << p.mIridescenceParameters.mIridescenceRampInterpolationMode << "\n"
        << "mIridescenceRampPoints: " << p.mIridescenceParameters.mIridescenceRampNumPoints << "\n"
        // transmission params
        << "mTransmission: " << p.mTransmission << "\n"
        << "mTransmissionColor: "
            << scene_rdl2::math::asCpp(p.mTransmissionColor).r << " "
            << scene_rdl2::math::asCpp(p.mTransmissionColor).g << " "
            << scene_rdl2::math::asCpp(p.mTransmissionColor).b << " " << "\n"
        << "mUseIndependentTransmissionRefractiveIndex: " << p.mUseIndependentTransmissionRefractiveIndex << "\n"
        << "mIndependentTransmissionRefractiveIndex: " << p.mIndependentTransmissionRefractiveIndex << "\n"
        << "mUseIndependentTransmissionRoughness: " << p.mUseIndependentTransmissionRoughness << "\n"
        << "mIndependentTransmissionRoughness: " << p.mIndependentTransmissionRoughness << "\n"
        << "mDispersionAbbeNumber: " << p.mDispersionAbbeNumber << "\n"
        // diffuse params
        << "mAlbedo: "
            << scene_rdl2::math::asCpp(p.mAlbedo).r << " "
            << scene_rdl2::math::asCpp(p.mAlbedo).g << " "
            << scene_rdl2::math::asCpp(p.mAlbedo).b << " " << "\n"
        << "mDiffuseRoughness: " << p.mDiffuseRoughness << "\n"
        << "mScatteringRadius: "
        << scene_rdl2::math::asCpp(p.mScatteringRadius).r << " "
        << scene_rdl2::math::asCpp(p.mScatteringRadius).g << " "
        << scene_rdl2::math::asCpp(p.mScatteringRadius).b << " " << "\n"
        << "mCreaseAttenuation: "   << p.mCreaseAttenuation << "\n"
        << "mDiffuseTransmission: "
            << scene_rdl2::math::asCpp(p.mDiffuseTransmission).r << " "
            << scene_rdl2::math::asCpp(p.mDiffuseTransmission).g << " "
            << scene_rdl2::math::asCpp(p.mDiffuseTransmission).b << " " << "\n"
        << "mDiffuseTransmissionBlendingBehavior: "
            << p.mDiffuseTransmissionBlendingBehavior << " " << "\n"
        << "mSubsurfaceTraceSet: " << p.mSubsurfaceTraceSet << "\n"
        << "mSSSResolveSelfIntersections: " << p.mSSSResolveSelfIntersections << "\n"
        // other params
        << "mNormal: "
            << scene_rdl2::math::asCpp(p.mNormal).x << " "
            << scene_rdl2::math::asCpp(p.mNormal).y << " "
            << scene_rdl2::math::asCpp(p.mNormal).z << " " << "\n"
        << "mNormalDial: " << p.mNormalDial << "\n"
        << "mNormalAAStrategy: " << p.mNormalAAStrategy << "\n"
        << "mNormalAADial: " << p.mNormalAADial << "\n"
        << "mEmission: "
            << scene_rdl2::math::asCpp(p.mEmission).r << " "
            << scene_rdl2::math::asCpp(p.mEmission).g << " "
            << scene_rdl2::math::asCpp(p.mEmission).b << " " << "\n"
        << "mEvalSubsurfaceNormalFn: " << p.mEvalSubsurfaceNormalFn << "\n"

        << p.mAccentParams << "\n";
}

finline float
computeMicrofacetRoughness(const float roughness,
                           ispc::NormalAAStrategyType normalAAStrategy,
                           float normalLength,
                           float normalAADial,
                           float normalDial)
{
    if (normalAAStrategy != ispc::NORMAL_AA_STRATEGY_TOKSVIG) {
        // no special normal mapping AA strategy, exit early
        return roughness;
    }

    // Assuming ispc::NORMAL_AA_STRATEGY_TOKSVIG...

    // Exit early if normalLength is exactly 1.0. This suggests no input normal map is bound
    // or use independent clearcoat normal is false so the state N is being used,
    // or the evaluated normal map is exactly unit length. Also check if the dials are zero,
    // in any of these cases we can exit early.
    if (scene_rdl2::math::isOne(normalLength) || scene_rdl2::math::isZero(normalAADial) || scene_rdl2::math::isZero(normalDial)) {
        return roughness;
    }

    // Account for normalDial by lerping the normal map normal length with the
    // unit length (1.f) state normal.
    normalLength = scene_rdl2::math::lerp(1.f, normalLength, normalDial);
    const float varN = scene_rdl2::math::clamp(normalLength, 0.01f, 1.0f);
    const float toksvigFactor = scene_rdl2::math::max((1.0f - varN) / varN, 0.f);

    // Convolution of two gaussian distributions -> variance = sqrt(v1^2 + v2^2).
    const float roughnessAA = scene_rdl2::math::sqrt(toksvigFactor + roughness * roughness);

    return scene_rdl2::math::lerp(roughness, roughnessAA, normalAADial);
}

finline scene_rdl2::math::Vec2f
computeAnisoRoughness(const float roughness,
                      const float minRoughness,
                      const float anisotropy)
{
    scene_rdl2::math::Vec2f result;
    result.x = anisotropy > 0.0f ?
               roughness * (1.0f - anisotropy) : roughness;
    result.y = anisotropy < 0.0f ?
               roughness * (1.0f - scene_rdl2::math::abs(anisotropy)) : roughness;

    result.x = scene_rdl2::math::max(result.x, minRoughness);
    result.y = scene_rdl2::math::max(result.y, minRoughness);

    return result;
}

finline bool
isWhite(const scene_rdl2::math::Color& c)
{
    return (scene_rdl2::math::isOne(c.r) && scene_rdl2::math::isOne(c.g) && scene_rdl2::math::isOne(c.b));
}

// interface and helper class for layering DwaBase
class DwaBaseLayerable : public scene_rdl2::rdl2::Material
{

public:
    DwaBaseLayerable(const scene_rdl2::rdl2::SceneClass& sceneClass,
                     const std::string& name,
                     const ispc::DwaBaseLabels& labels) :
        scene_rdl2::rdl2::Material(sceneClass, name),
        mLabels(labels)
    {
        // Set Common Function Pointers
        mPresenceFunc            = DwaBaseLayerable::presence;
        mIorFunc                 = DwaBaseLayerable::ior;
        mPreventLightCullingFunc = DwaBaseLayerable::preventLightCulling;

        registerShadeTimeEventMessages();
        mIspc.mScatterTagKey = moonray::shading::StandardAttributes::sScatterTag;
    }

    virtual ~DwaBaseLayerable() { }

    // Registers messages for error reporting during shade
    void registerShadeTimeEventMessages();

    // This function allows Layering materials to query each of their submaterials
    // to find out if any casts caustics before resolving any of the submaterials
    // parameters.  If any of the submaterials cast caustics, they all should.
    virtual bool getCastsCaustics() const = 0;

    // This function is used by DwaLayerMaterial and DwaMixMaterial to peek and see
    // if the submaterials have glitter without having to resolve their parameters
    // to find out. This information is necessary because if there are multiple submaterials
    // with glitter then the glitter uniform params need to use the parent material's values
    // because the values cannot be blended after the Glitter object is created during update().
    // Otherwise, if only 1 submaterial has glitter then its values can be used.
    virtual bool hasGlitter() const { return false; }

    friend std::ostream& operator<<(std::ostream& os, const ispc::DwaBaseParameters& p);

    finline static void
    addGlitterLobes(const scene_rdl2::rdl2::Material *material,
                    moonray::shading::BsdfBuilder &builder,
                    moonray::shading::TLState *tls,
                    const moonray::shading::State &state,
                    const ispc::DwaBaseParameters &params,
                    const ispc::DwaBaseLabels &labels,
                    bool& exitEarly,
                    const ispc::DwaBaseEventMessages &eventMessages,
                    const scene_rdl2::rdl2::LightSet* lightSet)
    {
        exitEarly = false;

        glitter::Glitter* glitterPointer = (glitter::Glitter*)(params.mGlitterPointerScalar);
        if (glitterPointer) {
            ispc::GLITTER_ResultCode resultCode = ispc::GLITTER_RESULTCODE_SUCCESS;
            glitterPointer->createLobes(tls,
                                        state,
                                        builder,
                                        params.mGlitterVaryingParameters,
                                        labels.mGlitter,
                                        resultCode,
                                        lightSet);

            if (resultCode != ispc::GLITTER_RESULTCODE_SUCCESS) {
                switch (resultCode)
                {
                case ispc::GLITTER_RESULTCODE_NO_REFN:
                    moonray::shading::logEvent(material, eventMessages.sErrorNoRefN);
                    break;
                case ispc::GLITTER_RESULTCODE_NO_REFP_PARTIALS:
                    moonray::shading::logEvent(material, eventMessages.sWarnNoRefPpartials);
                    break;
                default:
                    break;
                }
            }
        }
    }

    finline static float
    createHairRandomPerStrand(const DwaBaseLayerable * const dwaBaseLayerable,
                              const moonray::shading::TLState *tls,
                              const moonray::shading::State &state,
                              const ispc::HairParameters& hairParams,
                              const ispc::DwaBaseEventMessages &eventMessages)
    {
        float glintRandom = 0.0f;
        if (!scene_rdl2::math::isEqual(hairParams.mHairGlintMinTwists, hairParams.mHairGlintMaxTwists)) {

            // scatter_tag is a random number [0, 1) assigned to each hair
            float scatterTag = state.getAttribute(moonray::shading::StandardAttributes::sScatterTag);
            if (scene_rdl2::math::isEqual(scatterTag, -1.f)) {
                moonray::shading::logEvent(dwaBaseLayerable, eventMessages.sErrorScatterTagMissing);
            } else {
                glintRandom = scatterTag;
            }
        }
        return glintRandom;
    }

    finline static void
    addHairLobes(const DwaBaseLayerable * const dwaBaseLayerable,
                 moonray::shading::BsdfBuilder &builder,
                 const ispc::DwaBaseParameters &params,
                 const ispc::DwaBaseUniformParameters &uParams,
                 const ispc::DwaBaseLabels &labels,
                 const moonray::shading::TLState *tls,
                 const moonray::shading::State &state,
                 const ispc::DwaBaseEventMessages &eventMessages,
                 const scene_rdl2::rdl2::LightSet* lightSet)
    {
        const ispc::HairParameters hairParams = params.mHairParameters;

        // Hair Diffuse
        if (!scene_rdl2::math::isZero(hairParams.mHairDiffuse)) {

            // hair curves are always thin
            builder.setThinGeo();

            const bool hasSSS = !scene_rdl2::math::isBlack(scene_rdl2::math::asCpp(params.mScatteringRadius));
            const float sssWeight = hasSSS ? hairParams.mHairSubsurfaceBlend : 0.0f;

            moonray::shading::HairDiffuseBSDF hairDiffuseBsdf(
                scene_rdl2::math::asCpp(hairParams.mHairDir),
                scene_rdl2::math::asCpp(hairParams.mHairDiffuseFrontColor),
                scene_rdl2::math::asCpp(hairParams.mHairDiffuseBackColor));

            builder.addHairDiffuseBSDF(hairDiffuseBsdf,
                                       hairParams.mHairDiffuse * (1.0f - sssWeight),
                                       ispc::BSDFBUILDER_PHYSICAL,
                                       labels.mHairLabels.mHairDiffuse,
                                       lightSet);

            if (hasSSS) {
                // diffuse front color substitutes for albedo in these lobes
                // random walk is not supported
                scene_rdl2::math::Vec3f N = scene_rdl2::math::asCpp(params.mNormal);

                const ispc::SubsurfaceType bssrdfType = static_cast<ispc::SubsurfaceType>(uParams.mSubsurface);
                if (bssrdfType == ispc::SUBSURFACE_NORMALIZED_DIFFUSION) {
                    const moonray::shading::NormalizedDiffusion diffuseRefl(
                            N,
                            scene_rdl2::math::asCpp(hairParams.mHairDiffuseFrontColor),
                            scene_rdl2::math::asCpp(params.mScatteringRadius),
                            dwaBaseLayerable,
                            reinterpret_cast<const scene_rdl2::rdl2::TraceSet *>(params.mSubsurfaceTraceSet),
                            reinterpret_cast<scene_rdl2::rdl2::EvalNormalFunc>(params.mEvalSubsurfaceNormalFn));

                    builder.addNormalizedDiffusion(diffuseRefl,
                                                   hairParams.mHairDiffuse * sssWeight,
                                                   ispc::BSDFBUILDER_PHYSICAL,
                                                   labels.mHairLabels.mHair,
                                                   lightSet);

                } else if (bssrdfType == ispc::SUBSURFACE_DIPOLE_DIFFUSION) {
                    const moonray::shading::DipoleDiffusion diffuseRefl(
                            N,
                            scene_rdl2::math::asCpp(hairParams.mHairDiffuseFrontColor),
                            scene_rdl2::math::asCpp(params.mScatteringRadius),
                            dwaBaseLayerable,
                            reinterpret_cast<const scene_rdl2::rdl2::TraceSet *>(params.mSubsurfaceTraceSet),
                            reinterpret_cast<scene_rdl2::rdl2::EvalNormalFunc>(params.mEvalSubsurfaceNormalFn));

                    builder.addDipoleDiffusion(diffuseRefl,
                                               hairParams.mHairDiffuse * sssWeight,
                                               ispc::BSDFBUILDER_PHYSICAL,
                                               labels.mHairLabels.mHair,
                                               lightSet);

                }
            }
        }

        // Hair
        if (!scene_rdl2::math::isZero(hairParams.mHair)) {

            // hair curves are always thin
            builder.setThinGeo();

            if (!hairParams.mHairCastsCaustics) {
                // Add a Diffuse Approximation for Caustic Paths to reduce noise
                moonray::shading::HairDiffuseBSDF hairDiffuseBsdf(
                    scene_rdl2::math::asCpp(hairParams.mHairDir),
                    scene_rdl2::math::asCpp(hairParams.mHairColor),
                    scene_rdl2::math::asCpp(hairParams.mHairColor));

                builder.addHairDiffuseBSDF(hairDiffuseBsdf,
                                           1.0f,
                                           ispc::BSDFBUILDER_PHYSICAL,
                                           labels.mHairLabels.mHair,
                                           lightSet);
            } else {
                scene_rdl2::math::Vec3f N = scene_rdl2::math::Vec3f(0.f);
                float hairRotation = 0.0f;
                float glintRoughness = 0.0f;

                if (hairParams.mHairShowGlint) {
                    if (!(scene_rdl2::math::isZero(hairParams.mHairGlintMinTwists) &&
                          scene_rdl2::math::isZero(hairParams.mHairGlintMaxTwists))) {
                        const scene_rdl2::math::Vec3f T = scene_rdl2::math::normalize(state.getdPds());
                        const scene_rdl2::math::Vec3f B = scene_rdl2::math::normalize(state.getdPdt());
                        N = scene_rdl2::math::cross(B, T);

                        // Random float per hair between [0, 1)
                        const float glintRandom = createHairRandomPerStrand(dwaBaseLayerable,
                                                                            tls,
                                                                            state,
                                                                            hairParams,
                                                                            eventMessages);

                        hairRotation = scene_rdl2::math::sTwoPi * scene_rdl2::math::lerp(hairParams.mHairGlintMinTwists,
                                                                               hairParams.mHairGlintMaxTwists,
                                                                               glintRandom);
                    }

                    // Somewhat arbitrary remapping based on empirical results
                    glintRoughness = 0.125f * hairParams.mHairGlintRoughness * hairParams.mHairGlintRoughness;
                }

                if (hairParams.mHairUseOptimizedSampling) {
                    moonray::shading::HairBSDF hairBsdf(
                        scene_rdl2::math::asCpp(hairParams.mHairDir),
                        scene_rdl2::math::asCpp(hairParams.mHairUV),
                        hairParams.mHairIOR,
                        hairParams.mHairFresnelType,
                        hairParams.mHairCuticleLayerThickness,
                        hairParams.mHairShowR,
                        hairParams.mHairRShift,
                        hairParams.mHairRLongRoughness,
                        scene_rdl2::math::asCpp(hairParams.mHairRTint),
                        hairParams.mHairShowTT,
                        hairParams.mHairTTShift,
                        hairParams.mHairTTLongRoughness,
                        hairParams.mHairTTAzimRoughness,
                        scene_rdl2::math::asCpp(hairParams.mHairTTTint),
                        hairParams.mHairTTSaturation,
                        hairParams.mHairShowTRT,
                        hairParams.mHairTRTShift,
                        hairParams.mHairTRTLongRoughness,
                        scene_rdl2::math::asCpp(hairParams.mHairTRTTint),
                        hairParams.mHairShowGlint,
                        glintRoughness,
                        scene_rdl2::math::clamp(hairParams.mHairGlintEccentricity, 0.85f, 1.0f),
                        hairParams.mHairGlintSaturation,
                        hairRotation,
                        N,
                        hairParams.mHairShowTRRT,
                        scene_rdl2::math::asCpp(hairParams.mHairColor));
                    builder.addHairBSDF(hairBsdf,
                                        1.0f,
                                        ispc::BSDFBUILDER_PHYSICAL,
                                        labels.mHairLabels.mHair,
                                        lightSet);
                } else {
                    if (hairParams.mHairShowR) {
                        moonray::shading::HairRBRDF hairRBRDF(
                            scene_rdl2::math::asCpp(hairParams.mHairDir),
                            scene_rdl2::math::asCpp(hairParams.mHairUV),
                            hairParams.mHairIOR,
                            hairParams.mHairFresnelType,
                            hairParams.mHairCuticleLayerThickness,
                            hairParams.mHairRShift,
                            hairParams.mHairRLongRoughness,
                            scene_rdl2::math::sWhite);
                        builder.addHairRBRDF(hairRBRDF,
                                             1.0f,
                                             ispc::BSDFBUILDER_PHYSICAL,
                                             labels.mHairLabels.mHairR,
                                             lightSet);
                    }

                    // these 3 lobes need to be added as adjacent lobes because while
                    // they are 'under' the R lobe, they do not effect each other's
                    // energy distribution
                    builder.startAdjacentComponents();

                    if (hairParams.mHairShowTT) {
                        moonray::shading::HairTTBTDF hairTTBTDF(
                            scene_rdl2::math::asCpp(hairParams.mHairDir),
                            scene_rdl2::math::asCpp(hairParams.mHairUV),
                            hairParams.mHairIOR,
                            hairParams.mHairFresnelType,
                            hairParams.mHairCuticleLayerThickness,
                            hairParams.mHairTTShift,
                            hairParams.mHairTTLongRoughness,
                            hairParams.mHairTTAzimRoughness,
                            scene_rdl2::math::asCpp(hairParams.mHairColor),
                            scene_rdl2::math::asCpp(hairParams.mHairTTTint),
                            hairParams.mHairTTSaturation);
                       builder.addHairTTBTDF(hairTTBTDF,
                                             1.0f,
                                             ispc::BSDFBUILDER_PHYSICAL,
                                             labels.mHairLabels.mHairTT,
                                             lightSet);
                    }

                    if (hairParams.mHairShowTRT) {
                        moonray::shading::HairTRTBRDF hairTRTBRDF(
                            scene_rdl2::math::asCpp(hairParams.mHairDir),
                            scene_rdl2::math::asCpp(hairParams.mHairUV),
                            hairParams.mHairIOR,
                            hairParams.mHairFresnelType,
                            hairParams.mHairCuticleLayerThickness,
                            hairParams.mHairTRTShift,
                            hairParams.mHairTRTLongRoughness,
                            hairParams.mHairTTAzimRoughness,
                            scene_rdl2::math::asCpp(hairParams.mHairColor),
                            scene_rdl2::math::asCpp(hairParams.mHairTRTTint),
                            hairParams.mHairShowGlint,
                            glintRoughness,
                            scene_rdl2::math::clamp(hairParams.mHairGlintEccentricity, 0.85f, 1.0f),
                            hairParams.mHairGlintSaturation,
                            hairRotation,
                            N);
                       builder.addHairTRTBRDF(hairTRTBRDF,
                                              1.0f,
                                              ispc::BSDFBUILDER_PHYSICAL,
                                              labels.mHairLabels.mHairTRT,
                                              lightSet);
                    }

                    if (hairParams.mHairShowTRRT) {
                        moonray::shading::HairTRRTBRDF hairTRRTBRDF(
                            scene_rdl2::math::asCpp(hairParams.mHairDir),
                            scene_rdl2::math::asCpp(hairParams.mHairUV),
                            hairParams.mHairIOR,
                            hairParams.mHairFresnelType,
                            hairParams.mHairCuticleLayerThickness,
                            hairParams.mHairTRRTLongRoughness,
                            hairParams.mHairTTAzimRoughness,
                            scene_rdl2::math::asCpp(hairParams.mHairColor),
                            scene_rdl2::math::sWhite);
                       builder.addHairTRRTBRDF(hairTRRTBRDF,
                                               1.0f,
                                               ispc::BSDFBUILDER_PHYSICAL,
                                               labels.mHairLabels.mHairTRRT,
                                               lightSet);
                    }

                    builder.endAdjacentComponents();
                }
            }
        }
    }

    finline static scene_rdl2::math::Color
    getDiffuseTransmissionAttenuation(const ispc::DwaBaseParameters &params)
    {
        scene_rdl2::math::Color diffuseTransmissionAttenuation;
        if (params.mDiffuseTransmissionBlendingBehavior == ispc::DIFFUSE_TRANSMISSION_BLENDING_RGB) {
            diffuseTransmissionAttenuation = scene_rdl2::math::sWhite - scene_rdl2::math::asCpp(params.mDiffuseTransmission);
        } else {
            // Use the max of the transmission color to avoid hue shifting
            diffuseTransmissionAttenuation = scene_rdl2::math::sWhite - scene_rdl2::math::Color(
                    scene_rdl2::math::max(scene_rdl2::math::max(scene_rdl2::math::asCpp(params.mDiffuseTransmission).r,
                                                      scene_rdl2::math::asCpp(params.mDiffuseTransmission).g),
                                                      scene_rdl2::math::asCpp(params.mDiffuseTransmission).b));
        }
        return diffuseTransmissionAttenuation;
    }

    finline static void
    addToonSpecularLobes(moonray::shading::BsdfBuilder &builder,
                         const ispc::ToonSpecularParameters &params,
                         const int label,
                         const scene_rdl2::rdl2::LightSet* lightSet,
                         const moonray::shading::Iridescence * iridescence = nullptr)
    {
        const moonray::shading::ToonSpecularBRDF toonSpecularBRDF(
                scene_rdl2::math::asCpp(params.mNormal),
                params.mIntensity,
                params.mFresnelBlend,
                scene_rdl2::math::asCpp(params.mTint),
                params.mRampInputScale,
                params.mRampNumPoints,
                params.mRampPositions,
                params.mRampInterpolators,
                params.mRampValues,
                params.mStretchU,
                params.mStretchV,
                scene_rdl2::math::asCpp(params.mdPds),
                scene_rdl2::math::asCpp(params.mdPdt),
                params.mEnableIndirectReflections,
                params.mIndirectReflectionsIntensity,
                params.mIndirectReflectionsRoughness,
                iridescence);

        builder.addToonSpecularBRDF(
                toonSpecularBRDF,
                params.mToonSpecular,
                ispc::BSDFBUILDER_PHYSICAL,
                label,
                lightSet);
    }

    finline static void
    addHairToonSpecularLobes(moonray::shading::BsdfBuilder &builder,
                         const ispc::ToonSpecularParameters &params,
                         const ispc::DwaBaseLabels &labels,
                         const scene_rdl2::rdl2::LightSet* lightSet,
                         const moonray::shading::Iridescence * iridescence = nullptr)
    {
        const moonray::shading::HairToonSpecularBRDF hairToonSpecularBRDF(
                scene_rdl2::math::asCpp(params.mNormal),
                scene_rdl2::math::asCpp(params.mHairDir),
                scene_rdl2::math::asCpp(params.mHairUV),
                params.mHairIOR,
                params.mHairFresnelType,
                params.mHairCuticleLayerThickness,
                params.mHairRShift,
                params.mRoughness,
                scene_rdl2::math::asCpp(params.mTint),
                params.mIntensity,
                params.mRampNumPoints,
                params.mRampPositions,
                params.mRampInterpolators,
                params.mRampValues,
                params.mEnableIndirectReflections,
                params.mIndirectReflectionsIntensity,
                params.mIndirectReflectionsRoughness,
                iridescence);

        builder.addHairToonSpecularBRDF(
                hairToonSpecularBRDF,
                params.mToonSpecular,
                ispc::BSDFBUILDER_PHYSICAL,
                labels.mHairLabels.mHair,
                lightSet);
    }

    finline static void
    addOuterSpecularLobes(moonray::shading::BsdfBuilder &builder,
                          const ispc::DwaBaseParameters &params,
                          const ispc::DwaBaseUniformParameters &uParams,
                          const ispc::DwaBaseLabels &labels,
                          float minRoughness,
                          const moonray::shading::Iridescence * iridescence,
                          const scene_rdl2::rdl2::LightSet* lightSet)
    {
        const bool refracts =
            (uParams.mOuterSpecularUseBending && (!uParams.mThinGeometry));

        const scene_rdl2::math::Color& attenuationColor = scene_rdl2::math::asCpp(params.mOuterSpecularAttenuationColor);
        const bool absorbs = !scene_rdl2::math::isZero(params.mOuterSpecularThickness) &&
                             !isWhite(attenuationColor);

        // Modulate outer specular roughness based on normal map mip-map anti-aliasing strategy.
        float roughness = computeMicrofacetRoughness(
                params.mOuterSpecularRoughness,
                static_cast<ispc::NormalAAStrategyType>(params.mNormalAAStrategy),
                params.mOuterSpecularNormalLength,
                params.mNormalAADial,
                params.mOuterSpecularNormalDial);

        roughness = scene_rdl2::math::max(roughness, minRoughness);

        if (refracts || absorbs) {
            // only isotropic clearcoat supported in dwabase
            if (scene_rdl2::math::isZero(roughness)) { // mirror
                const moonray::shading::MirrorClearcoat clearcoat(
                        scene_rdl2::math::asCpp(params.mOuterSpecularNormal),
                        params.mOuterSpecularRefractiveIndex,
                        params.mOuterSpecularThickness,
                        attenuationColor,
                        refracts,
                        iridescence);

                builder.addMirrorClearcoat(
                        clearcoat,
                        params.mOuterSpecular,
                        ispc::BSDFBUILDER_PHYSICAL,
                        labels.mOuterSpecular,
                        lightSet);
            } else { // microfacet
                const moonray::shading::MicrofacetIsotropicClearcoat clearcoat(
                        scene_rdl2::math::asCpp(params.mOuterSpecularNormal),
                        params.mOuterSpecularRefractiveIndex,
                        roughness,
                        params.mOuterSpecularThickness,
                        attenuationColor,
                        refracts,
                        static_cast<ispc::MicrofacetDistribution>(uParams.mOuterSpecularModel),
                        ispc::MICROFACET_GEOMETRIC_SMITH,
                        iridescence);

                builder.addMicrofacetIsotropicClearcoat(
                        clearcoat,
                        params.mOuterSpecular,
                        ispc::BSDFBUILDER_PHYSICAL,
                        labels.mOuterSpecular,
                        lightSet);
            }
        } else { // not clearcoat, just a reflection lobe
            if (scene_rdl2::math::isZero(roughness)) { // mirror
                const moonray::shading::MirrorBRDF reflection(
                        scene_rdl2::math::asCpp(params.mOuterSpecularNormal),
                        params.mOuterSpecularRefractiveIndex,
                        iridescence);

                builder.addMirrorBRDF(
                        reflection,
                        params.mOuterSpecular,
                        ispc::BSDFBUILDER_PHYSICAL,
                        labels.mOuterSpecular,
                        lightSet);
            } else { // microfacet
                const moonray::shading::MicrofacetIsotropicBRDF reflection(
                        scene_rdl2::math::asCpp(params.mOuterSpecularNormal),
                        params.mOuterSpecularRefractiveIndex,
                        roughness,
                        static_cast<ispc::MicrofacetDistribution>(uParams.mOuterSpecularModel),
                        ispc::MICROFACET_GEOMETRIC_SMITH,
                        iridescence);

                builder.addMicrofacetIsotropicBRDF(reflection,
                                                   params.mOuterSpecular,
                                                   ispc::BSDFBUILDER_PHYSICAL,
                                                   labels.mOuterSpecular,
                                                   lightSet);
            }
        }
    }

    // Called from Leaf-level materials.
    // material must derive from DwaBaseLayerable
    virtual void
    createLobes(const DwaBaseLayerable * const dwaBaseLayerable,
                moonray::shading::TLState *tls,
                const moonray::shading::State &state,
                moonray::shading::BsdfBuilder &builder,
                const ispc::DwaBaseParameters &params,
                const ispc::DwaBaseUniformParameters &uParams,
                const ispc::DwaBaseLabels &labels) const;

    finline static void
    initGlitterVaryingParameters(ispc::GLITTER_VaryingParameters &varyingParams)
    {
        varyingParams.mFlakeStyleFrequency[0] = 1.0f;
        scene_rdl2::math::asCpp(varyingParams.mFlakeColor[0]) = scene_rdl2::math::sWhite;
        varyingParams.mFlakeSize[0] = 1.0f;
        varyingParams.mFlakeRoughness[0] = 0.14f;
        varyingParams.mFlakeStyleFrequency[1] = 0.0f;
        scene_rdl2::math::asCpp(varyingParams.mFlakeColor[1]) = scene_rdl2::math::sWhite;
        varyingParams.mFlakeSize[1] = 1.0f;
        varyingParams.mFlakeRoughness[1] = 0.14f;
        scene_rdl2::math::asCpp(varyingParams.mFlakeHSVColorVariation) = scene_rdl2::math::Vec3f(0.0f, 0.0f, 0.0f);
        varyingParams.mFlakeDensity = 1.0f;;
        varyingParams.mFlakeJitter = 1.0f;
        varyingParams.mGlitterMask = 0.0f;
        varyingParams.mFlakeOrientationRandomness = 0.15f;
        varyingParams.mCompensateDeformation = true;
        varyingParams.mApproximateForSecRays = true;
    }

    finline static void
    initToonDiffuseParameters(ispc::ToonDiffuseParameters &params)
    {
        params.mToonDiffuse = 0.0f;
        params.mModel = ispc::TOON_DIFFUSE_OREN_NAYAR;
        params.mTerminatorShift = 0.0f;
        params.mFlatness = 0.0f;
        params.mFlatnessFalloff = 0.0f;
        params.mRampWeight = 0.0f;
        params.mRampNumPoints = 1;
        params.mRampPositions[0] = 0.0f;
        scene_rdl2::math::asCpp(params.mRampColors[0]) =
                scene_rdl2::math::Color(1.0f, 1.0f, 1.0f);
        params.mRampInterpolators[0] = ispc::RAMP_INTERPOLATOR_MODE_NONE;
        params.mExtendRamp = false;
        params.mRampInputScale = 1.0f;
        scene_rdl2::math::asCpp(params.mNormal) = scene_rdl2::math::Vec3f(0.0f, 0.0f, 0.0f);
    }

    finline static void
    initToonSpecularParameters(ispc::ToonSpecularParameters &params)
    {
        params.mToonSpecular = 0.0f;
        params.mIntensity = 1.0f;
        params.mFresnelBlend = 1.0f;
        params.mRoughness = 0.9f;
        scene_rdl2::math::asCpp(params.mTint) = scene_rdl2::math::Color(1.0f, 1.0f, 1.0f);
        params.mRampInputScale = 1.0f;
        params.mRampNumPoints = 1;
        params.mRampPositions[0] = 0.0f;
        params.mRampValues[0] = 1.0f;
        params.mRampInterpolators[0] = ispc::RAMP_INTERPOLATOR_MODE_NONE;
        scene_rdl2::math::asCpp(params.mNormal) = scene_rdl2::math::Vec3f(0.0f, 0.0f, 0.0f);
        params.mStretchU = 1.0f;
        params.mStretchV = 1.0f;
        scene_rdl2::math::asCpp(params.mdPds) = scene_rdl2::math::Vec3f(0.0f, 0.0f, 0.0f);
        scene_rdl2::math::asCpp(params.mdPdt) = scene_rdl2::math::Vec3f(0.0f, 0.0f, 0.0f);
        params.mEnableIndirectReflections = false;
        params.mIndirectReflectionsIntensity = 0.0f;
        params.mIndirectReflectionsRoughness = 0.5f;

        scene_rdl2::math::asCpp(params.mHairDir) = scene_rdl2::math::Vec3f(0.0f, 0.0f, 0.0f);
        scene_rdl2::math::asCpp(params.mHairUV) = scene_rdl2::math::Vec2f(0.0f, 0.0f);
        params.mHairIOR = 1.45f;
        params.mHairFresnelType = ispc::HairFresnelType::HAIR_FRESNEL_DIELECTRIC_CYLINDER;
        params.mHairCuticleLayerThickness = 0.1f;
        params.mHairRShift = 0.0f;
    }

    finline static void
    initIridescenceParameters(ispc::IridescenceParameters &params)
    {
        params.mIridescence = 0.0f;
        params.mIridescenceApplyTo = ispc::IRIDESCENCE_PRIMARY_SPECULAR;
        params.mIridescenceColorControl = ispc::SHADING_IRIDESCENCE_COLOR_USE_HUE_INTERPOLATION;
        scene_rdl2::math::asCpp(params.mIridescencePrimaryColor) = scene_rdl2::math::sWhite;
        scene_rdl2::math::asCpp(params.mIridescenceSecondaryColor) = scene_rdl2::math::sWhite;
        params.mIridescenceFlipHueDirection = false;
        params.mIridescenceThickness = 0.0f;
        params.mIridescenceExponent = 1.0f;
        params.mIridescenceAt0 = 0.0f;
        params.mIridescenceAt90 = 0.0f;
        params.mIridescenceRampInterpolationMode = ispc::COLOR_RAMP_CONTROL_SPACE_RGB;
        params.mIridescenceRampNumPoints = 1;
        params.mIridescenceRampPositions[0] = 0.0f;
        scene_rdl2::math::asCpp(params.mIridescenceRampColors[0]) =
                scene_rdl2::math::sWhite;
        params.mIridescenceRampInterpolators[0] = ispc::RAMP_INTERPOLATOR_MODE_NONE;
    }

    finline static void
    initHairParameters(ispc::HairParameters &params)
    {
        // Common Hair params
        scene_rdl2::math::asCpp(params.mHairDir) = scene_rdl2::math::Vec3f(0.0f, 0.0f, 0.0f);
        scene_rdl2::math::asCpp(params.mHairColor) = scene_rdl2::math::Color(1.0f, 1.0f, 1.0f);

        // HairMaterial_v3 params
        params.mHair = 0.0f;
        params.mHairCastsCaustics = false;
        scene_rdl2::math::asCpp(params.mHairUV) = scene_rdl2::math::Vec2f(0.0f, 0.0f);
        params.mHairIOR = 1.45f;
        params.mHairShowR = true;
        params.mHairRShift = 0.0f;
        params.mHairRLongRoughness = 0.5f;
        scene_rdl2::math::asCpp(params.mHairRTint) = scene_rdl2::math::Color(1.0f, 1.0f, 1.0f);
        params.mHairShowTT = true;
        params.mHairTTShift = 0.0f;
        params.mHairTTLongRoughness = 0.1f;
        params.mHairTTAzimRoughness = 1.0f;
        scene_rdl2::math::asCpp(params.mHairTTTint) = scene_rdl2::math::Color(1.0f, 1.0f, 1.0f);
        params.mHairTTSaturation = 1.0f;
        params.mHairShowTRT = true;
        params.mHairTRTShift = 0.0f;
        params.mHairTRTLongRoughness = 0.4f;
        scene_rdl2::math::asCpp(params.mHairTRTTint) = scene_rdl2::math::Color(1.0f, 1.0f, 1.0f);
        params.mHairShowGlint = false;
        params.mHairGlintRoughness = 0.5f;
        params.mHairGlintMinTwists = 1.5f;
        params.mHairGlintMaxTwists = 1.5f;
        params.mHairGlintEccentricity = 0.85f;
        params.mHairGlintSaturation = 0.5f;
        params.mHairShowTRRT = true;
        params.mHairTRRTLongRoughness = 4.0f * params.mHairRLongRoughness;
        params.mHairFresnelType = ispc::HairFresnelType::HAIR_FRESNEL_DIELECTRIC_CYLINDER;
        params.mHairCuticleLayerThickness = 0.1f;
        params.mHairUseOptimizedSampling = true;

        // HairDiffuseMaterial params
        params.mHairDiffuse = 0.0f;
        params.mHairDiffuseUseIndependentFrontAndBackColor = false;
        scene_rdl2::math::asCpp(params.mHairDiffuseFrontColor) = scene_rdl2::math::Color(1.0f, 1.0f, 1.0f);
        scene_rdl2::math::asCpp(params.mHairDiffuseBackColor) = scene_rdl2::math::Color(1.0f, 1.0f, 1.0f);
        params.mHairSubsurfaceBlend = 1.0f;
    }

    finline static void
    initAccentParameters(ispc::AccentParameters &params)
    {
        params.mSubsurface = 1.0f;
        scene_rdl2::math::asCpp(params.mSubsurfaceNormal) = scene_rdl2::math::Vec3f(0.0f, 0.0f, 0.0f);
        params.mSubsurfaceNormalDial = 1.0f;
        scene_rdl2::math::asCpp(params.mSubsurfaceColor) = scene_rdl2::math::sWhite;
    }

    finline static void
    initParameters(ispc::DwaBaseParameters &params)
    {
        // The following uniform params are set in update:
        // mOuterSpecularModel
        // mOuterSpecularUseBending
        // mSpecularModel
        // mSubsurface
        // mThinGeometry
        // mPreventLightCulling

        // Glitter params
        params.mGlitterUniformParameters = nullptr;
        initGlitterVaryingParameters(params.mGlitterVaryingParameters);

        // Hair params
        initHairParameters(params.mHairParameters);

        // Hair Toon params
        initToonSpecularParameters(params.mHairToonS1Params);
        initToonSpecularParameters(params.mHairToonS2Params);
        initToonSpecularParameters(params.mHairToonS3Params);

        // Toon params
        initToonDiffuseParameters(params.mToonDiffuseParams);
        initToonSpecularParameters(params.mToonSpecularParams);

        // Fuzz params
        params.mFuzz = 0.0f;
        params.mFuzzRoughness = 0.0f;
        scene_rdl2::math::asCpp(params.mFuzzAlbedo) = scene_rdl2::math::sWhite;
        params.mFuzzUseAbsorbingFibers = false;
        scene_rdl2::math::asCpp(params.mFuzzNormal) = scene_rdl2::math::Vec3f(0.0f, 0.0f, 0.0f);

        // OuterSpecular params
        params.mOuterSpecular = 0.0f;
        params.mOuterSpecularRefractiveIndex = 1.5f;
        params.mOuterSpecularRoughness = 0.0f;
        params.mOuterSpecularThickness = 0.0f;
        scene_rdl2::math::asCpp(params.mOuterSpecularAttenuationColor) = scene_rdl2::math::sWhite;
        scene_rdl2::math::asCpp(params.mOuterSpecularNormal) = scene_rdl2::math::Vec3f(0.0f, 0.0f, 0.0f);
        params.mOuterSpecularNormalLength = 1.0f;
        params.mOuterSpecularNormalDial = 1.0f;

        // metallic params
        params.mMetallic = 0.0f;
        scene_rdl2::math::asCpp(params.mMetallicColor) = scene_rdl2::math::sWhite;
        scene_rdl2::math::asCpp(params.mMetallicEdgeColor) = scene_rdl2::math::sWhite;

        // specular params
        params.mSpecular = 0.0f;
        params.mRefractiveIndex = 1.5f;
        params.mRoughness = 0.0f;
        params.mAnisotropy = 0.0f;
        scene_rdl2::math::asCpp(params.mShadingTangent) = scene_rdl2::math::Vec2f(0.0f, 0.0f);

        // fabric params
        scene_rdl2::math::asCpp(params.mFabricTangent) = scene_rdl2::math::Vec3f(1.0f, 0.0f, 0.0f);
        params.mFabricSpecular = 0.0f;
        scene_rdl2::math::asCpp(params.mWarpColor) = scene_rdl2::math::sBlack;
        params.mWarpRoughness = 0.0f;
        params.mUseIndependentWeftAttributes = false;
        params.mWeftRoughness = 0.0f;
        scene_rdl2::math::asCpp(params.mWeftColor) = scene_rdl2::math::sBlack;
        scene_rdl2::math::asCpp(params.mWarpThreadDirection) = scene_rdl2::math::Vec3f(0.0f, 0.0f, 0.0f);
        params.mWarpThreadCoverage = 0.0f;
        params.mWarpThreadElevation = 0.0f;
        params.mFabricAttenuation = 1.0f;

        // iridescence params
        initIridescenceParameters(params.mIridescenceParameters);

        // transmission params
        params.mTransmission = 0.0f;
        scene_rdl2::math::asCpp(params.mTransmissionColor) = scene_rdl2::math::sBlack;
        params.mUseIndependentTransmissionRefractiveIndex = false;
        params.mIndependentTransmissionRefractiveIndex = 1.5f;
        params.mUseIndependentTransmissionRoughness = false;
        params.mIndependentTransmissionRoughness = 0.0f;
        params.mDispersionAbbeNumber = 0.0f;

        // diffuse params
        scene_rdl2::math::asCpp(params.mAlbedo) = scene_rdl2::math::sBlack;
        params.mDiffuseRoughness = 0.0f;
        scene_rdl2::math::asCpp(params.mScatteringRadius) = scene_rdl2::math::sBlack;
        params.mCreaseAttenuation = 0.0f;

        scene_rdl2::math::asCpp(params.mDiffuseTransmission) = scene_rdl2::math::sBlack;
        params.mDiffuseTransmissionBlendingBehavior = ispc::DIFFUSE_TRANSMISSION_BLENDING_MONOCHROMATIC;
        params.mSubsurfaceTraceSet = 0;
        params.mSSSResolveSelfIntersections = true;

        // other params
        scene_rdl2::math::asCpp(params.mNormal) = scene_rdl2::math::Vec3f(0.0f, 0.0f, 0.0f);
        scene_rdl2::math::asCpp(params.mDiffuseNormal) = scene_rdl2::math::Vec3f(0.0f, 0.0f, 0.0f);
        params.mNormalDial = 1.0f;
        params.mNormalLength = 1.0f;
        params.mNormalAAStrategy = ispc::NormalAAStrategyType::NORMAL_AA_STRATEGY_NONE;
        params.mNormalAADial = 1.0f;
        scene_rdl2::math::asCpp(params.mEmission) = scene_rdl2::math::sBlack;
        params.mEvalSubsurfaceNormalFn = 0;

        params.mDiffuseLightSet = nullptr;
        params.mSpecularLightSet = nullptr;

        initAccentParameters(params.mAccentParams);
    }

    finline static void
    initColorCorrectParameters(ispc::DwaBaseParameters &params)
    {
        params.mNumColorCorrections = 0;
        for (size_t i = 0; i < ispc::DWABASE_MAX_COLOR_CORRECTIONS; ++i) {
            params.mColorCorrectParams[i].mOn = false;
            params.mColorCorrectParams[i].mMix = 1.0f;
            params.mColorCorrectParams[i].mHueShift = 0.0f;
            params.mColorCorrectParams[i].mSaturation = 1.0f;
            params.mColorCorrectParams[i].mGain = 1.0f;
            params.mColorCorrectParams[i].mTmiEnabled = false;
            scene_rdl2::math::asCpp(params.mColorCorrectParams[i].mTmi) = scene_rdl2::math::Color(0.0f, 0.0f, 0.0f);
        }
    }

    finline static void
    copyColorCorrectParameters(const ispc::DwaBaseParameters &src,
                               ispc::DwaBaseParameters &dst)
    {
        dst.mNumColorCorrections = src.mNumColorCorrections;
        for (size_t i = 0; i < dst.mNumColorCorrections; ++i) {
            dst.mColorCorrectParams[i].mOn = src.mColorCorrectParams[i].mOn;
            dst.mColorCorrectParams[i].mMix = src.mColorCorrectParams[i].mMix;
            dst.mColorCorrectParams[i].mHueShift = src.mColorCorrectParams[i].mHueShift;
            dst.mColorCorrectParams[i].mSaturation = src.mColorCorrectParams[i].mSaturation;
            dst.mColorCorrectParams[i].mGain = src.mColorCorrectParams[i].mGain;
            dst.mColorCorrectParams[i].mTmiEnabled = src.mColorCorrectParams[i].mTmiEnabled;
            scene_rdl2::math::asCpp(dst.mColorCorrectParams[i].mTmi) =
                scene_rdl2::math::asCpp(src.mColorCorrectParams[i].mTmi);
        }
    }

    // This function's job is to resolve the BSSRDF type
    // Keeping this separate to mimic the vectorized code which needs to evaluate
    // this uniform value separately.
    virtual int resolveSubsurfaceType(const moonray::shading::State& state) const = 0;

    finline static void
    initUniformParameters(ispc::DwaBaseUniformParameters &uParams)
    {
        // init params
        uParams.mSpecularModel = 1;
        uParams.mToonSpecularModel = ispc::ToonSpecularModel::ToonSpecularGGX;
        uParams.mHairToonS1Model = ispc::ToonSpecularModel::ToonSpecularSurface;
        uParams.mHairToonS2Model = ispc::ToonSpecularModel::ToonSpecularSurface;
        uParams.mHairToonS3Model = ispc::ToonSpecularModel::ToonSpecularSurface;
        uParams.mOuterSpecularModel = 1;
        uParams.mOuterSpecularUseBending = false;
        uParams.mSubsurface = ispc::SubsurfaceType::SUBSURFACE_NONE;
        uParams.mThinGeometry = false;
        uParams.mPreventLightCulling = false;
    }

    virtual void resolveUniformParameters(ispc::DwaBaseUniformParameters &uParams) const = 0;

    // This function's job is to initialize the ispc::DwaBaseParameters
    // instance. The values used to initialize the struct will come from
    // evaluating the material attributes directly or by evaluating
    // the sub-materials' attributes and blending the results. The
    // 'causticsEnabled' arg should override the material's local
    // 'casts caustics' attr and specular-related attrs should be
    // resolved.
    // Returns true if parameters can be resolved, otherwise false.
    virtual bool resolveParameters(moonray::shading::TLState *tls,
                                   const moonray::shading::State& state,
                                   bool castsCaustics,
                                   ispc::DwaBaseParameters &params) const = 0;

    virtual scene_rdl2::math::Vec3f resolveSubsurfaceNormal(
            moonray::shading::TLState *tls,
            const moonray::shading::State& state) const = 0;

    virtual float resolvePresence(moonray::shading::TLState *tls,
                                  const moonray::shading::State& state) const = 0;

    virtual float resolveRefractiveIndex(moonray::shading::TLState *tls,
                                         const moonray::shading::State& state) const = 0;

    // Common Presence Handler for Dwa Materials
    static float presence(const scene_rdl2::rdl2::Material* self,
                          moonray::shading::TLState *tls,
                          const moonray::shading::State& state) {
        const DwaBaseLayerable* me = static_cast<const DwaBaseLayerable*>(self);
        return me->resolvePresence(tls, state);
    }

    // Common IOR Handler for Dwa Materials
    static float ior(const scene_rdl2::rdl2::Material* self,
                     moonray::shading::TLState *tls,
                     const moonray::shading::State& state) {
        const DwaBaseLayerable* me = static_cast<const DwaBaseLayerable*>(self);
        return me->resolveRefractiveIndex(tls, state);
    }

    // Common Early Spherical Check Handler for Dwa Materials
    static bool preventLightCulling(const scene_rdl2::rdl2::Material* self,
                                    const moonray::shading::State& state) {
        const DwaBaseLayerable* me = static_cast<const DwaBaseLayerable*>(self);
        return me->resolvePreventLightCulling(state);
    }

    virtual bool resolvePreventLightCulling(const moonray::shading::State& state) const
    { return false; }

    // Return pointers to the required ISPC functions. These should be
    // retrieved by DwaBaseLayerMaterial in update() when a sub-material
    // is bound/unbound and stored for use during shading in ISPC.
    virtual void * getCastsCausticsISPCFunc() const = 0;
    virtual void * getResolveSubsurfaceTypeISPCFunc() const = 0;
    virtual void * getResolveParametersISPCFunc() const = 0;
    virtual void * getResolvePresenceISPCFunc() const = 0;
    virtual void * getResolvePreventLightCullingISPCFunc() const { return nullptr; }
    virtual void * getResolveSubsurfaceNormalISPCFunc() const = 0;
    virtual void * getCreateLobesISPCFunc() const { return ispc::DwaBase_getCreateLobesFunc(); }


    // Return pointer to evalSubsurfaceNormal function.
    virtual scene_rdl2::rdl2::EvalNormalFunc getEvalSubsurfaceNormalFunc() const = 0;

    const ispc::DwaBaseLayerable* getISPCDwaBaseLayerableStruct() const { return &mIspc; }
    const ispc::DwaBaseLabels* getLabelsDataStruct() const { return &mLabels; }

    void applyColorCorrectParameters(moonray::shading::TLState *tls,
                                     const moonray::shading::State &state,
                                     ispc::DwaBaseParameters &params) const;

private:
    ispc::DwaBaseLayerable mIspc;
    ispc::DwaBaseLabels mLabels;
};

} // namespace dwabase
} // namespace moonshine

