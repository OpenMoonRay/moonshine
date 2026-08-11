// Copyright 2023-2024 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

///
/// @file DwaBase.cc
/// $Id$
///

#include "DwaBase.h"

#include <scene_rdl2/scene/rdl2/rdl2.h>
#include <moonray/rendering/shading/EvalAttribute.h>
#include <scene_rdl2/common/math/MathUtil.h>

#include <memory>

namespace moonshine {

// This is a construct to access the ISPC Data Struct from Inside the Scalar Material object.
// Note we are not using MATERIAL_GET_ISPC_CPTR to retrieve the ispc structure from the Material
// class as in other materials since that macro assumes a certain data layout we do not adhere to in
// DwaBase materials.
extern "C" {
const void*
getDwaBaseMaterialStruct(const void* ptr)
{
    const dwabase::DwaBase* classPtr = static_cast<const dwabase::DwaBase*>(ptr);
    return (static_cast<const void*>(classPtr->getISPCBaseMaterialStruct()));
}

}

namespace {
void
initializeNormalMap(const dwabase::DwaBase* dwaBase,
                    const scene_rdl2::rdl2::AttributeKey<scene_rdl2::rdl2::SceneObject *>& normalAttr,
                    const ispc::NormalMap** ispcNormalMapPtr,
                    int64_t& ispcSampleNormalFunc)
{
    *ispcNormalMapPtr = nullptr;
    ispcSampleNormalFunc = 0;
    if (normalAttr.isValid()) {
        scene_rdl2::rdl2::NormalMap* normalMapPtr;
        normalMapPtr = dwaBase->get(normalAttr) ?
                       dwaBase->get(normalAttr)->asA<scene_rdl2::rdl2::NormalMap>() :
                       nullptr;

        *ispcNormalMapPtr = (ispc::NormalMap*)normalMapPtr;
        if (normalMapPtr != nullptr) {
            ispcSampleNormalFunc = (intptr_t)normalMapPtr->mSampleNormalFuncv;
        }
    }
}
} // end anonymous namespace

namespace dwabase {

using namespace moonray;
using namespace scene_rdl2::math;
using namespace moonray::shading;

#define VALIDATE_TOON_SPEC_ATTR_KEYS(name)                                    \
    MNRY_ASSERT(mAttrKeys.name.mIntensity.isValid());                         \
    MNRY_ASSERT(mAttrKeys.name.mFresnelBlend.isValid());                      \
    MNRY_ASSERT(mAttrKeys.name.mRoughness.isValid());                         \
    MNRY_ASSERT(mAttrKeys.name.mTint.isValid());                              \
    MNRY_ASSERT(mAttrKeys.name.mRampInputScale.isValid());                    \
    MNRY_ASSERT(mAttrKeys.name.mRampPositions.isValid());                     \
    MNRY_ASSERT(mAttrKeys.name.mRampValues.isValid());                        \
    MNRY_ASSERT(mAttrKeys.name.mRampInterpolators.isValid());                 \
    MNRY_ASSERT(mAttrKeys.name.mEnableInputNormal.isValid());                 \
    MNRY_ASSERT(mAttrKeys.name.mInputNormal.isValid());                       \
    MNRY_ASSERT(mAttrKeys.name.mInputNormalDial.isValid());                   \
    MNRY_ASSERT(mAttrKeys.name.mStretchU.isValid());                          \
    MNRY_ASSERT(mAttrKeys.name.mStretchV.isValid());                          \
    MNRY_ASSERT(mAttrKeys.name.mUseInputVectors.isValid());                   \
    MNRY_ASSERT(mAttrKeys.name.mInputU.isValid());                            \
    MNRY_ASSERT(mAttrKeys.name.mInputV.isValid());                            \
    MNRY_ASSERT(mAttrKeys.name.mEnableIndirectReflections.isValid());         \
    MNRY_ASSERT(mAttrKeys.name.mIndirectReflectionsIntensity.isValid());      \
    MNRY_ASSERT(mAttrKeys.name.mIndirectReflectionsRoughness.isValid());

#define VALIDATE_TOON_SPEC_ATTR_FUNCS(name)                                        \
    MNRY_ASSERT(mIspc.mAttrFuncs.name.mEvalAttrIntensity);                         \
    MNRY_ASSERT(mIspc.mAttrFuncs.name.mEvalAttrFresnelBlend);                      \
    MNRY_ASSERT(mIspc.mAttrFuncs.name.mEvalAttrRoughness);                         \
    MNRY_ASSERT(mIspc.mAttrFuncs.name.mEvalAttrTint);                              \
    MNRY_ASSERT(mIspc.mAttrFuncs.name.mEvalAttrRampInputScale);                    \
    MNRY_ASSERT(mIspc.mAttrFuncs.name.mGetAttrEnableInputNormal);                  \
    MNRY_ASSERT(mIspc.mAttrFuncs.name.mEvalAttrInputNormalDial);                   \
    MNRY_ASSERT(mIspc.mAttrFuncs.name.mEvalAttrStretchU);                          \
    MNRY_ASSERT(mIspc.mAttrFuncs.name.mEvalAttrStretchV);                          \
    MNRY_ASSERT(mIspc.mAttrFuncs.name.mGetAttrUseInputVectors);                    \
    MNRY_ASSERT(mIspc.mAttrFuncs.name.mEvalAttrInputU);                            \
    MNRY_ASSERT(mIspc.mAttrFuncs.name.mEvalAttrInputV);                            \
    MNRY_ASSERT(mIspc.mAttrFuncs.name.mGetAttrEnableIndirectReflections);          \
    MNRY_ASSERT(mIspc.mAttrFuncs.name.mEvalAttrIndirectReflectionsIntensity);      \
    MNRY_ASSERT(mIspc.mAttrFuncs.name.mEvalAttrIndirectReflectionsRoughness);      \
    MNRY_ASSERT(mIspc.mAttrFuncs.name.mEvalAttrRampMultiplier0);                   \
    MNRY_ASSERT(mIspc.mAttrFuncs.name.mEvalAttrRampMultiplier1);                   \
    MNRY_ASSERT(mIspc.mAttrFuncs.name.mEvalAttrRampMultiplier2);                   \
    MNRY_ASSERT(mIspc.mAttrFuncs.name.mEvalAttrRampMultiplier3);                   \
    MNRY_ASSERT(mIspc.mAttrFuncs.name.mEvalAttrRampMultiplier4);                   \
    MNRY_ASSERT(mIspc.mAttrFuncs.name.mEvalAttrRampMultiplier5);                   \
    MNRY_ASSERT(mIspc.mAttrFuncs.name.mEvalAttrRampMultiplier6);                   \
    MNRY_ASSERT(mIspc.mAttrFuncs.name.mEvalAttrRampMultiplier7);                   \
    MNRY_ASSERT(mIspc.mAttrFuncs.name.mEvalAttrRampMultiplier8);                   \
    MNRY_ASSERT(mIspc.mAttrFuncs.name.mEvalAttrRampMultiplier9);                   \
    MNRY_ASSERT(mIspc.mAttrFuncs.name.mEvalAttrRampPositionOffset0);               \
    MNRY_ASSERT(mIspc.mAttrFuncs.name.mEvalAttrRampPositionOffset1);               \
    MNRY_ASSERT(mIspc.mAttrFuncs.name.mEvalAttrRampPositionOffset2);               \
    MNRY_ASSERT(mIspc.mAttrFuncs.name.mEvalAttrRampPositionOffset3);               \
    MNRY_ASSERT(mIspc.mAttrFuncs.name.mEvalAttrRampPositionOffset4);               \
    MNRY_ASSERT(mIspc.mAttrFuncs.name.mEvalAttrRampPositionOffset5);               \
    MNRY_ASSERT(mIspc.mAttrFuncs.name.mEvalAttrRampPositionOffset6);               \
    MNRY_ASSERT(mIspc.mAttrFuncs.name.mEvalAttrRampPositionOffset7);               \
    MNRY_ASSERT(mIspc.mAttrFuncs.name.mEvalAttrRampPositionOffset8);               \
    MNRY_ASSERT(mIspc.mAttrFuncs.name.mEvalAttrRampPositionOffset9);

DwaBase::DwaBase(const scene_rdl2::rdl2::SceneClass& sceneClass,
                 const std::string& name,
                 const DwaBaseAttributeKeys& attrKeys,
                 const ispc::DwaBaseAttributeFuncs& attrFns,
                 const ispc::DwaBaseLabels& labels,
                 const ispc::Model model):
      DwaBaseLayerable(sceneClass, name, labels),
      mAttrKeys(attrKeys)
{
    // verify that for each component/feature we have all required AttributeKeys
    // and accessor functions needed to determine the corresponding parameters

    mIspc.mAttrFuncs = attrFns;
    mIspc.mModel     = model;

    // The DwaLayerMaterial is derived from DwaBase because of glitter
    // but does not have the presence parameter like the other Dwa
    // materials.
    if (mIspc.mModel != ispc::Model::Layer) {
        MNRY_ASSERT(mAttrKeys.mPresence.isValid());
    }

    if (mAttrKeys.mShowGlitter.isValid()) {

        // Uniform glitter attribute keys
        MNRY_ASSERT(mAttrKeys.mGlitterSeed.isValid());
        MNRY_ASSERT(mAttrKeys.mGlitterSpace.isValid());
        MNRY_ASSERT(mAttrKeys.mGlitterRandomness.isValid());
        MNRY_ASSERT(mAttrKeys.mGlitterFlakeTextureA.isValid());
        MNRY_ASSERT(mAttrKeys.mGlitterFlakeTextureB.isValid());

        // Varying glitter attribute keys
        MNRY_ASSERT(mAttrKeys.mGlitter.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrGlitter);

        MNRY_ASSERT(mAttrKeys.mGlitterCompensateDeformation.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mGetAttrGlitterCompensateDeformation);

        MNRY_ASSERT(mAttrKeys.mGlitterFlakeJitter.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrGlitterJitter);

        MNRY_ASSERT(mAttrKeys.mGlitterStyleAFrequency.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrGlitterStyleAFrequency);

        MNRY_ASSERT(mAttrKeys.mGlitterStyleBFrequency.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrGlitterStyleBFrequency);

        MNRY_ASSERT(mAttrKeys.mGlitterFlakeSizeA.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrGlitterSizeA);

        MNRY_ASSERT(mAttrKeys.mGlitterFlakeSizeB.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrGlitterSizeB);

        MNRY_ASSERT(mAttrKeys.mGlitterFlakeDensity.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrGlitterDensity);

        MNRY_ASSERT(mAttrKeys.mGlitterFlakeRoughnessA.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mGetAttrGlitterRoughnessA);

        MNRY_ASSERT(mAttrKeys.mGlitterFlakeRoughnessB.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mGetAttrGlitterRoughnessB);

        MNRY_ASSERT(mAttrKeys.mGlitterFlakeColorA.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrGlitterColorA);

        MNRY_ASSERT(mAttrKeys.mGlitterFlakeColorB.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrGlitterColorB);

        MNRY_ASSERT(mAttrKeys.mGlitterFlakeOrientationRandomness.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mGetAttrGlitterOrientationRandomness);

        MNRY_ASSERT(mAttrKeys.mGlitterFlakeHueVariation.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrGlitterHueVariation);

        MNRY_ASSERT(mAttrKeys.mGlitterFlakeSaturationVariation.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrGlitterSaturationVariation);

        MNRY_ASSERT(mAttrKeys.mGlitterFlakeValueVariation.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrGlitterValueVariation);

        MNRY_ASSERT(mAttrKeys.mGlitterApproximateForSecondaryRays.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mGetAttrGlitterApproximateForSecondaryRays);
    }

    if (mIspc.mModel == ispc::Model::Toon) {
        MNRY_ASSERT(mAttrKeys.mToonDiffuseKeys.mModel.isValid());

        MNRY_ASSERT(mAttrKeys.mToonDiffuseKeys.mTerminatorShift.isValid());
        MNRY_ASSERT(mAttrKeys.mToonDiffuseKeys.mFlatness.isValid());
        MNRY_ASSERT(mAttrKeys.mToonDiffuseKeys.mFlatnessFalloff.isValid());
        MNRY_ASSERT(mAttrKeys.mToonDiffuseKeys.mRampInterpolators.isValid());
        MNRY_ASSERT(mAttrKeys.mToonDiffuseKeys.mRampPositions.isValid());
        MNRY_ASSERT(mAttrKeys.mToonDiffuseKeys.mExtendRamp.isValid());
        MNRY_ASSERT(mAttrKeys.mToonDiffuseKeys.mRampInputScale.isValid());

        MNRY_ASSERT(mIspc.mAttrFuncs.mToonDiffuseFuncs.mEvalAttrTerminatorShift);
        MNRY_ASSERT(mIspc.mAttrFuncs.mToonDiffuseFuncs.mEvalAttrFlatness);
        MNRY_ASSERT(mIspc.mAttrFuncs.mToonDiffuseFuncs.mEvalAttrFlatnessFalloff);

        MNRY_ASSERT(mIspc.mAttrFuncs.mToonDiffuseFuncs.mEvalAttrRampColorMultiplier0);
        MNRY_ASSERT(mIspc.mAttrFuncs.mToonDiffuseFuncs.mEvalAttrRampColorMultiplier1);
        MNRY_ASSERT(mIspc.mAttrFuncs.mToonDiffuseFuncs.mEvalAttrRampColorMultiplier2);
        MNRY_ASSERT(mIspc.mAttrFuncs.mToonDiffuseFuncs.mEvalAttrRampColorMultiplier3);
        MNRY_ASSERT(mIspc.mAttrFuncs.mToonDiffuseFuncs.mEvalAttrRampColorMultiplier4);
        MNRY_ASSERT(mIspc.mAttrFuncs.mToonDiffuseFuncs.mEvalAttrRampColorMultiplier5);
        MNRY_ASSERT(mIspc.mAttrFuncs.mToonDiffuseFuncs.mEvalAttrRampColorMultiplier6);
        MNRY_ASSERT(mIspc.mAttrFuncs.mToonDiffuseFuncs.mEvalAttrRampColorMultiplier7);
        MNRY_ASSERT(mIspc.mAttrFuncs.mToonDiffuseFuncs.mEvalAttrRampColorMultiplier8);
        MNRY_ASSERT(mIspc.mAttrFuncs.mToonDiffuseFuncs.mEvalAttrRampColorMultiplier9);

        MNRY_ASSERT(mIspc.mAttrFuncs.mToonDiffuseFuncs.mEvalAttrRampPositionOffset0);
        MNRY_ASSERT(mIspc.mAttrFuncs.mToonDiffuseFuncs.mEvalAttrRampPositionOffset1);
        MNRY_ASSERT(mIspc.mAttrFuncs.mToonDiffuseFuncs.mEvalAttrRampPositionOffset2);
        MNRY_ASSERT(mIspc.mAttrFuncs.mToonDiffuseFuncs.mEvalAttrRampPositionOffset3);
        MNRY_ASSERT(mIspc.mAttrFuncs.mToonDiffuseFuncs.mEvalAttrRampPositionOffset4);
        MNRY_ASSERT(mIspc.mAttrFuncs.mToonDiffuseFuncs.mEvalAttrRampPositionOffset5);
        MNRY_ASSERT(mIspc.mAttrFuncs.mToonDiffuseFuncs.mEvalAttrRampPositionOffset6);
        MNRY_ASSERT(mIspc.mAttrFuncs.mToonDiffuseFuncs.mEvalAttrRampPositionOffset7);
        MNRY_ASSERT(mIspc.mAttrFuncs.mToonDiffuseFuncs.mEvalAttrRampPositionOffset8);
        MNRY_ASSERT(mIspc.mAttrFuncs.mToonDiffuseFuncs.mEvalAttrRampPositionOffset9);

        VALIDATE_TOON_SPEC_ATTR_KEYS(mToonSpecularKeys);
        VALIDATE_TOON_SPEC_ATTR_FUNCS(mToonSpecularFuncs);
    }

    if (mAttrKeys.mShowHair.isValid()) {
        MNRY_ASSERT(mIspc.mAttrFuncs.mGetAttrShowHair);

        MNRY_ASSERT(mAttrKeys.mHair.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrHair);
    }

    if (mAttrKeys.mShowHair.isValid() || mIspc.mModel == ispc::Model::Hair) {
        MNRY_ASSERT(mAttrKeys.mHairColor.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrHairColor);

        MNRY_ASSERT(mAttrKeys.mRefractiveIndex.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mGetAttrRefractiveIndex);

        MNRY_ASSERT(mAttrKeys.mHairFresnelType.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mGetAttrHairFresnelType);

        MNRY_ASSERT(mAttrKeys.mHairCuticleLayerThickness.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrHairCuticleLayerThickness);

        MNRY_ASSERT(mAttrKeys.mHairShowR.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mGetAttrHairShowR);

        MNRY_ASSERT(mAttrKeys.mHairROffset.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrHairROffset);

        MNRY_ASSERT(mAttrKeys.mHairRRoughness.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrHairRRoughness);

        MNRY_ASSERT(mAttrKeys.mHairRTintColor.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrHairRTintColor);

        MNRY_ASSERT(mAttrKeys.mHairShowTRT.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mGetAttrHairShowTRT);

        MNRY_ASSERT(mAttrKeys.mHairTRTOffset.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrHairTRTOffset);

        MNRY_ASSERT(mAttrKeys.mHairTRTUseRoughness.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mGetAttrHairTRTUseRoughness);

        MNRY_ASSERT(mAttrKeys.mHairTRTRoughness.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrHairTRTRoughness);

        MNRY_ASSERT(mAttrKeys.mHairTRTTintColor.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrHairTRTTintColor);

        if (mAttrKeys.mHairShowGlint.isValid()) {
            MNRY_ASSERT(mIspc.mAttrFuncs.mGetAttrShowHairGlint);

            MNRY_ASSERT(mAttrKeys.mHairGlintRoughness.isValid());
            MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrHairGlintRoughness);

            MNRY_ASSERT(mAttrKeys.mHairGlintMinTwists.isValid());
            MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrHairGlintMinTwists);

            MNRY_ASSERT(mAttrKeys.mHairGlintMaxTwists.isValid());
            MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrHairGlintMaxTwists);

            MNRY_ASSERT(mAttrKeys.mHairGlintEccentricity.isValid());
            MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrHairGlintEccentricity);

            MNRY_ASSERT(mAttrKeys.mHairGlintSaturation.isValid());
            MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrHairGlintSaturation);
        }

        MNRY_ASSERT(mAttrKeys.mHairShowTT.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mGetAttrHairShowTT);

        MNRY_ASSERT(mAttrKeys.mHairTTOffset.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrHairTTOffset);

        MNRY_ASSERT(mAttrKeys.mHairTTUseRoughness.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mGetAttrHairTTUseRoughness);

        MNRY_ASSERT(mAttrKeys.mHairTTRoughness.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrHairTTRoughness);

        MNRY_ASSERT(mAttrKeys.mHairTTAzimuthalRoughness.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrHairTTAzimuthalRoughness);

        MNRY_ASSERT(mAttrKeys.mHairTTTintColor.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrHairTTTintColor);

        MNRY_ASSERT(mAttrKeys.mHairUseOptimizedSampling.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mGetAttrHairUseOptimizedSampling);

        MNRY_ASSERT(mAttrKeys.mHairShowTRRTPlus.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mGetAttrHairShowTRRTPlus);
    }

    if (mAttrKeys.mShowHairDiffuse.isValid()) {
        MNRY_ASSERT(mIspc.mAttrFuncs.mGetAttrShowHairDiffuse);

        MNRY_ASSERT(mAttrKeys.mHairDiffuse.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrHairDiffuse);
    }

    if (mIspc.mModel == ispc::Model::HairToon) {
        VALIDATE_TOON_SPEC_ATTR_KEYS(mHairToonS1Keys);
        VALIDATE_TOON_SPEC_ATTR_FUNCS(mHairToonS1Funcs);
        VALIDATE_TOON_SPEC_ATTR_KEYS(mHairToonS2Keys);
        VALIDATE_TOON_SPEC_ATTR_FUNCS(mHairToonS2Funcs);
        VALIDATE_TOON_SPEC_ATTR_KEYS(mHairToonS3Keys);
        VALIDATE_TOON_SPEC_ATTR_FUNCS(mHairToonS3Funcs);
    }

    if (mAttrKeys.mShowHairDiffuse.isValid() ||
        mIspc.mModel == ispc::Model::HairDiffuse ||
        mIspc.mModel == ispc::Model::HairToon) {

        MNRY_ASSERT(mAttrKeys.mHairiDiffuseUseIndependentFrontAndBackColor.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mGetAttrHairDiffuseUseIndependentFrontAndBackColor);

        MNRY_ASSERT(mAttrKeys.mHairDiffuseFrontColor.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrHairDiffuseFrontColor);

        MNRY_ASSERT(mAttrKeys.mHairDiffuseBackColor.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrHairDiffuseBackColor);
    }

    if (mAttrKeys.mShowOuterSpecular.isValid()) {
        MNRY_ASSERT(mIspc.mAttrFuncs.mGetAttrShowOuterSpecular);

        MNRY_ASSERT(mAttrKeys.mOuterSpecular.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrOuterSpecular);

        MNRY_ASSERT(mAttrKeys.mOuterSpecularModel.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mGetAttrOuterSpecularModel);

        MNRY_ASSERT(mAttrKeys.mOuterSpecularRefractiveIndex.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mGetAttrOuterSpecularRefractiveIndex);

        MNRY_ASSERT(mAttrKeys.mOuterSpecularRoughness.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrOuterSpecularRoughness);

        MNRY_ASSERT(mAttrKeys.mUseOuterSpecularNormal.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mGetAttrUseOuterSpecularNormal);

        MNRY_ASSERT(mAttrKeys.mOuterSpecularNormal.isValid());
        MNRY_ASSERT(mAttrKeys.mOuterSpecularNormalDial.isValid());
    }

    if (mAttrKeys.mShowSpecular.isValid()) {
        MNRY_ASSERT(mIspc.mAttrFuncs.mGetAttrShowSpecular);

        MNRY_ASSERT(mAttrKeys.mSpecular.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrSpecular);

        MNRY_ASSERT(mAttrKeys.mSpecularModel.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mGetAttrSpecularModel);

        MNRY_ASSERT(mAttrKeys.mRoughness.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrRoughness);

        if (mAttrKeys.mMetallic.isValid() || mIspc.mModel == ispc::Model::Metal) {
            MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrMetallic || mIspc.mModel == ispc::Model::Metal);

            MNRY_ASSERT(mAttrKeys.mMetallicColor.isValid());
            MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrMetallicColor);

            MNRY_ASSERT(mAttrKeys.mMetallicEdgeColor.isValid());
            MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrMetallicEdgeColor);
        } else {
            // If the material does not support 'metallic' then
            // refractive index is definitely required
            MNRY_ASSERT(mAttrKeys.mRefractiveIndex.isValid());
            MNRY_ASSERT(mIspc.mAttrFuncs.mGetAttrRefractiveIndex);
        }

        if (mAttrKeys.mAnisotropy.isValid()) {
            MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrAnisotropy);

            MNRY_ASSERT(mAttrKeys.mShadingTangent.isValid());
            MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrShadingTangent);
        }

        if (mAttrKeys.mIridescence.isValid()) {
            MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrIridescence);

            MNRY_ASSERT(mAttrKeys.mIridescenceApplyTo.isValid());
            MNRY_ASSERT(mIspc.mAttrFuncs.mGetAttrIridescenceApplyTo);

            MNRY_ASSERT(mAttrKeys.mIridescenceColorControl.isValid());
            MNRY_ASSERT(mIspc.mAttrFuncs.mGetAttrIridescenceColorControl);

            MNRY_ASSERT(mAttrKeys.mIridescencePrimaryColor.isValid());
            MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrIridescencePrimaryColor);

            MNRY_ASSERT(mAttrKeys.mIridescenceSecondaryColor.isValid());
            MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrIridescenceSecondaryColor);

            MNRY_ASSERT(mAttrKeys.mIridescenceFlipHueDirection.isValid());
            MNRY_ASSERT(mIspc.mAttrFuncs.mGetAttrIridescenceFlipHueDirection);

            MNRY_ASSERT(mAttrKeys.mIridescenceThickness.isValid());
            MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrIridescenceThickness);

            MNRY_ASSERT(mAttrKeys.mIridescenceExponent.isValid());
            MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrIridescenceExponent);

            MNRY_ASSERT(mAttrKeys.mIridescenceAt0.isValid());
            MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrIridescenceAt0);

            MNRY_ASSERT(mAttrKeys.mIridescenceAt90.isValid());
            MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrIridescenceAt90);

            MNRY_ASSERT(mAttrKeys.mIridescenceRampInterpolationMode.isValid());
            MNRY_ASSERT(mIspc.mAttrFuncs.mGetAttrIridescenceRampInterpolationMode);
        }
    }

    if (mAttrKeys.mShowTransmission.isValid()) {
        MNRY_ASSERT(mIspc.mAttrFuncs.mGetAttrShowTransmission);

        MNRY_ASSERT(mAttrKeys.mTransmission.isValid() || mIspc.mModel == ispc::Model::Refractive);
        MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrTransmission || mIspc.mModel == ispc::Model::Refractive);

        MNRY_ASSERT(mAttrKeys.mTransmissionColor.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrTransmissionColor);

        MNRY_ASSERT(mAttrKeys.mDispersionAbbeNumber.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mGetAttrDispersionAbbeNumber);

        MNRY_ASSERT(mAttrKeys.mRefractiveIndex.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mGetAttrRefractiveIndex);

        if (mAttrKeys.mUseIndependentTransmissionRefractiveIndex.isValid()) {
            MNRY_ASSERT(mAttrKeys.mIndependentTransmissionRefractiveIndex.isValid());
            MNRY_ASSERT(mIspc.mAttrFuncs.mGetAttrUseIndependentTransmissionRefractiveIndex);
            MNRY_ASSERT(mIspc.mAttrFuncs.mGetAttrIndependentTransmissionRefractiveIndex);
        }

        if (mAttrKeys.mUseIndependentTransmissionRoughness.isValid()) {
            MNRY_ASSERT(mAttrKeys.mIndependentTransmissionRoughness.isValid());
            MNRY_ASSERT(mIspc.mAttrFuncs.mGetAttrUseIndependentTransmissionRoughness);
            MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrIndependentTransmissionRoughness);
        }
    }

   if (mAttrKeys.mShowFabricSpecular.isValid()) {
       MNRY_ASSERT(mIspc.mAttrFuncs.mGetAttrShowFabricSpecular);

       if (mAttrKeys.mFabricSpecular.isValid() || mIspc.mModel == ispc::Model::Fabric) {
           MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrFabricSpecular  || mIspc.mModel == ispc::Model::Fabric);

           MNRY_ASSERT(mAttrKeys.mWarpColor.isValid());
           MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrWarpColor);

           MNRY_ASSERT(mAttrKeys.mWarpRoughness.isValid());
           MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrWarpRoughness);

           MNRY_ASSERT(mAttrKeys.mUseIndependentWeftAttributes.isValid());
           MNRY_ASSERT(mIspc.mAttrFuncs.mGetAttrUseIndependentWeftAttributes);

           MNRY_ASSERT(mAttrKeys.mWeftRoughness.isValid());
           MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrWeftRoughness);

           MNRY_ASSERT(mAttrKeys.mWeftColor.isValid());
           MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrWeftColor);

           MNRY_ASSERT(mAttrKeys.mWarpThreadDirection.isValid());
           MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrWarpThreadDirection);

           MNRY_ASSERT(mAttrKeys.mWarpThreadCoverage.isValid());
           MNRY_ASSERT(mIspc.mAttrFuncs.mGetAttrWarpThreadCoverage);

           MNRY_ASSERT(mAttrKeys.mWarpThreadElevation.isValid());
           MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrWarpThreadElevation);
       }
   }

    if (mAttrKeys.mShowDiffuse.isValid()) {
        MNRY_ASSERT(mIspc.mAttrFuncs.mGetAttrShowDiffuse);

        MNRY_ASSERT(mAttrKeys.mAlbedo.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrAlbedo);

        MNRY_ASSERT(mAttrKeys.mDiffuseRoughness.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrDiffuseRoughness);

        MNRY_ASSERT(mAttrKeys.mDiffuseTransmission.isValid());
        MNRY_ASSERT(mAttrKeys.mDiffuseTransmissionColor.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrDiffuseTransmission);
        MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrDiffuseTransmissionColor);

        MNRY_ASSERT(mAttrKeys.mDiffuseTransmissionBlendingBehavior.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mGetAttrDiffuseTransmissionBlendingBehavior);

        if (mAttrKeys.mFabricSpecular.isValid() || mIspc.mModel == ispc::Model::Fabric) {
            MNRY_ASSERT(mAttrKeys.mFabricDiffuseScattering.isValid());
            MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrFabricDiffuseScattering);
        }
    }

    if (mAttrKeys.mShowEmission.isValid()) {
        MNRY_ASSERT(mIspc.mAttrFuncs.mGetAttrShowEmission);

        MNRY_ASSERT(mAttrKeys.mEmission.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mEvalAttrEmission);
    }

    if (mAttrKeys.mInputNormal.isValid()) {
        MNRY_ASSERT(mAttrKeys.mInputNormalDial.isValid());
    }

    if (mAttrKeys.mNormalAAStrategy.isValid()) {
        MNRY_ASSERT(mIspc.mAttrFuncs.mGetAttrNormalAAStrategy);

        MNRY_ASSERT(mAttrKeys.mNormalAADial.isValid());
        MNRY_ASSERT(mIspc.mAttrFuncs.mGetAttrNormalAADial);
    }
}

DwaBase::~DwaBase()
{
}

bool
DwaBase::hasGlitter() const
{
    bool result = false;
    if (mAttrKeys.mShowGlitter.isValid()) {
        if (get(mAttrKeys.mShowGlitter)) {
            result = !isZero(get(mAttrKeys.mGlitter));
        }
    }
    return result;
}

// IMPORTANT:  Derived classes that override update() should
// be sure to call DwaBase::update() or the hints will not
// get updated and the behavior will be undefined,
void
DwaBase::update()
{
    // We compute some 'hints' here that will potentially allow us to
    // skip evaluation of certain attributes during resolveParameters().
    // In particular We don't want to evaluate any map bindings unless
    // they are truly needed.
    //
    mIspc.mHints.mGlitterIsOne = false;
    mIspc.mHints.mRequiresGlitterParams = false;

    if (mAttrKeys.mShowGlitter.isValid()) {
        if (get(mAttrKeys.mShowGlitter)) {
            const float val = get(mAttrKeys.mGlitter);
            mIspc.mHints.mRequiresGlitterParams = !isZero(val);
            mIspc.mHints.mGlitterIsOne = (getBinding(mAttrKeys.mGlitter) == nullptr) && isOne(val);
        }
    }
    if (mIspc.mHints.mRequiresGlitterParams) {
        updateGlitter();
    }


    mIspc.mHints.mRequiresToonDiffuseParams = false;
    mIspc.mHints.mRequiresToonSpecularParams = false;
    if (mIspc.mModel == ispc::Model::Toon) {
        mIspc.mHints.mRequiresToonDiffuseParams = true;

        const ToonSpecularKeys& tsKeys = mAttrKeys.mToonSpecularKeys;
        if (tsKeys.mShow.isValid() && get(tsKeys.mShow) &&
           (tsKeys.mModel.isValid() && get(tsKeys.mModel) == ispc::ToonSpecularModel::ToonSpecularSurface ||
            tsKeys.mModel.isValid() && get(tsKeys.mModel) == ispc::ToonSpecularModel::ToonSpecularHair)) {

            mIspc.mHints.mRequiresToonSpecularParams = true;
        }
    }

    mIspc.mHints.mRequiresHairToonS1Params = false;
    mIspc.mHints.mRequiresHairToonS2Params = false;
    mIspc.mHints.mRequiresHairToonS3Params = false;
    if (mIspc.mModel == ispc::Model::HairToon) {
        if (mAttrKeys.mHairToonS1Keys.mShow.isValid() &&
            get(mAttrKeys.mHairToonS1Keys.mShow)) {
            mIspc.mHints.mRequiresHairToonS1Params = true;
        }

        if (mAttrKeys.mHairToonS2Keys.mShow.isValid() &&
            get(mAttrKeys.mHairToonS2Keys.mShow)) {
            mIspc.mHints.mRequiresHairToonS2Params = true;
        }

        if (mAttrKeys.mHairToonS3Keys.mShow.isValid() &&
            get(mAttrKeys.mHairToonS3Keys.mShow)) {
            mIspc.mHints.mRequiresHairToonS3Params = true;
        }
    }

    mIspc.mHints.mHairIsOne = false;
    mIspc.mHints.mRequiresHairParams = false;
    if (mAttrKeys.mShowHair.isValid()) {
        mIspc.mHints.mRequiresHairParams = get(mAttrKeys.mShowHair);
        const float val = get(mAttrKeys.mHair);
        mIspc.mHints.mRequiresHairParams = !isZero(val);
        mIspc.mHints.mHairIsOne = (getBinding(mAttrKeys.mHair) == nullptr) && isOne(val);
    } else if (mIspc.mModel == ispc::Model::Hair) {
        mIspc.mHints.mRequiresHairParams = true;
        mIspc.mHints.mHairIsOne = true;
    }

    if (mAttrKeys.mHairShowGlint.isValid() && get(mAttrKeys.mHairShowGlint)) {
        if (hasChanged(mAttrKeys.mHairGlintMinTwists) ||
            hasChanged(mAttrKeys.mHairGlintMaxTwists)) {

            // don't want to clear mRequiredAttributes in case glitter still requires refPKey
            std::vector<int>::iterator it = std::find(mRequiredAttributes.begin(),
                                                      mRequiredAttributes.end(),
                                                      moonray::shading::StandardAttributes::sScatterTag);
            if (it != mRequiredAttributes.end() &&
                    isEqual(get(mAttrKeys.mHairGlintMinTwists), get(mAttrKeys.mHairGlintMaxTwists))) {
                // key found but no glint variation, so erase key
                mRequiredAttributes.erase(it);
            } else if (it == mRequiredAttributes.end() &&
                    !isEqual(get(mAttrKeys.mHairGlintMinTwists), get(mAttrKeys.mHairGlintMaxTwists))) {
                // key not found and glint variation is present, so add key
                mRequiredAttributes.push_back(moonray::shading::StandardAttributes::sScatterTag);
            }
        }
    }

    mIspc.mHints.mHairDiffuseIsOne = false;
    mIspc.mHints.mRequiresHairDiffuseParams = false;
    if (mAttrKeys.mShowHairDiffuse.isValid()) {
        if (get(mAttrKeys.mShowHairDiffuse)) {
            const float val = get(mAttrKeys.mHairDiffuse);
            mIspc.mHints.mRequiresHairDiffuseParams = !isZero(val);
            mIspc.mHints.mHairDiffuseIsOne = (getBinding(mAttrKeys.mHairDiffuse) == nullptr) && isOne(val);
        }
    } else if (mIspc.mModel == ispc::Model::HairDiffuse) {
        mIspc.mHints.mRequiresHairDiffuseParams = true;
        mIspc.mHints.mHairDiffuseIsOne = true;
    }

    if (mAttrKeys.mShowFuzz.isValid()) {
        mIspc.mHints.mRequiresFuzzParams = get(mAttrKeys.mShowFuzz);
    } else {
        mIspc.mHints.mRequiresFuzzParams = false;
    }

    if (mAttrKeys.mShowOuterSpecular.isValid() && get(mAttrKeys.mShowOuterSpecular)) {
        mIspc.mHints.mRequiresOuterSpecularParams = !isZero(get(mAttrKeys.mOuterSpecular));
        mIspc.mHints.mRequiresOuterSpecularAbsorptionParams = mAttrKeys.mOuterSpecularThickness.isValid() ?
                                                              !isZero(get(mAttrKeys.mOuterSpecularThickness))
                                                              : false;
    } else {
        mIspc.mHints.mRequiresOuterSpecularParams = false;
        mIspc.mHints.mRequiresOuterSpecularAbsorptionParams = false;
    }

    if (mAttrKeys.mShowSpecular.isValid() && get(mAttrKeys.mShowSpecular)) {
        mIspc.mHints.mRequiresSpecularParams =
            mAttrKeys.mSpecular.isValid() && !isZero(get(mAttrKeys.mSpecular));
        if (mIspc.mHints.mRequiresSpecularParams) {
            if (mIspc.mModel == ispc::Model::Metal) {
                mIspc.mHints.mRequiresMetallicParams = true;
                mIspc.mHints.mMetallicIsOne = true;
            } else if (mAttrKeys.mMetallic.isValid()) {
                const float val = get(mAttrKeys.mMetallic);
                mIspc.mHints.mRequiresMetallicParams = !isZero(val);
                mIspc.mHints.mMetallicIsOne =
                    (getBinding(mAttrKeys.mMetallic) == nullptr) && isOne(val);
            } else {
                mIspc.mHints.mRequiresMetallicParams = false;
                mIspc.mHints.mMetallicIsOne = false;
            }
            mIspc.mHints.mRequiresAnisotropyParams =
                mAttrKeys.mAnisotropy.isValid() && !isZero(get(mAttrKeys.mAnisotropy));

        } else {
            mIspc.mHints.mRequiresMetallicParams = false;
            mIspc.mHints.mMetallicIsOne = false;
            mIspc.mHints.mRequiresAnisotropyParams = false;
        }
    } else {
        mIspc.mHints.mRequiresSpecularParams = false;
        mIspc.mHints.mRequiresMetallicParams = false;
        mIspc.mHints.mMetallicIsOne = false;
        mIspc.mHints.mRequiresAnisotropyParams = false;
    }

    if (mAttrKeys.mIridescence.isValid()) {
        mIspc.mHints.mRequiresIridescenceParams = !isZero(get(mAttrKeys.mIridescence));
    } else {
        mIspc.mHints.mRequiresIridescenceParams = false;
    }

    if (mIspc.mHints.mRequiresIridescenceParams) {
        updateIridescence();
    }

    if (mAttrKeys.mShowTransmission.isValid() && get(mAttrKeys.mShowTransmission)) {
        if (mIspc.mModel == ispc::Model::Refractive) {
            mIspc.mHints.mRequiresTransmissionParams = true;
            mIspc.mHints.mTransmissionIsOne = true;
        } else if (mAttrKeys.mTransmission.isValid()) {
            const float val = get(mAttrKeys.mTransmission);
            mIspc.mHints.mRequiresTransmissionParams = !isZero(val);
            mIspc.mHints.mTransmissionIsOne = (getBinding(mAttrKeys.mTransmission) == nullptr) && isOne(val);
        }

        if (mAttrKeys.mUseIndependentTransmissionRefractiveIndex.isValid() && get(mAttrKeys.mUseIndependentTransmissionRefractiveIndex)) {
            mIspc.mHints.mRequiresTransmissionRefractiveIndex = true;
        } else {
            mIspc.mHints.mRequiresTransmissionRefractiveIndex = false;
        }
    } else {
        mIspc.mHints.mRequiresTransmissionRefractiveIndex = false;
        mIspc.mHints.mRequiresTransmissionParams = false;
        mIspc.mHints.mTransmissionIsOne = false;
    }

    if (mAttrKeys.mShowFabricSpecular.isValid() && get(mAttrKeys.mShowFabricSpecular)) {
        if (mIspc.mModel == ispc::Model::Fabric) {
            mIspc.mHints.mRequiresFabricParams = true;
            mIspc.mHints.mFabricIsOne = true;
        } else if (mAttrKeys.mFabricSpecular.isValid()) {
            const float val = get(mAttrKeys.mFabricSpecular);
            mIspc.mHints.mRequiresFabricParams = !isZero(val);
            mIspc.mHints.mFabricIsOne = (getBinding(mAttrKeys.mFabricSpecular) == nullptr) && isOne(val);
        } else {
            mIspc.mHints.mRequiresFabricParams = false;
            mIspc.mHints.mFabricIsOne = false;
        }
    } else {
        mIspc.mHints.mRequiresFabricParams = false;
        mIspc.mHints.mFabricIsOne = false;
    }

    if (mAttrKeys.mShowDiffuse.isValid() && get(mAttrKeys.mShowDiffuse)) {
        mIspc.mHints.mRequiresDiffuseParams =
             !mIspc.mHints.mTransmissionIsOne && !mIspc.mHints.mMetallicIsOne &&
             mAttrKeys.mAlbedo.isValid() && !isBlack(get(mAttrKeys.mAlbedo));

        if (mAttrKeys.mFabricDiffuseScattering.isValid()) {
            mIspc.mHints.mRequiresDiffuseParams = mIspc.mHints.mRequiresDiffuseParams &&
                    !isZero(get(mAttrKeys.mFabricDiffuseScattering));
        }
    } else {
        mIspc.mHints.mRequiresDiffuseParams = false;
    }

    mIspc.mHints.mRequiresSubsurfaceParams =
        (mIspc.mHints.mRequiresDiffuseParams || mIspc.mHints.mRequiresHairDiffuseParams) &&
        mAttrKeys.mScatteringRadius.isValid() && mAttrKeys.mScatteringColor.isValid() &&
        !isZero(get(mAttrKeys.mScatteringRadius)) && !isBlack(get(mAttrKeys.mScatteringColor));

    mIspc.mHints.mRequiresDiffuseTransmissionParams =
        mAttrKeys.mShowDiffuse.isValid() && get(mAttrKeys.mShowDiffuse) &&
        !mIspc.mHints.mTransmissionIsOne && !mIspc.mHints.mMetallicIsOne &&
        mAttrKeys.mDiffuseTransmissionColor.isValid() && !isBlack(get(mAttrKeys.mDiffuseTransmissionColor)) &&
        mAttrKeys.mDiffuseTransmission.isValid() && !isZero(get(mAttrKeys.mDiffuseTransmission));

    mIspc.mHints.mRequiresEmissionParams =
        mAttrKeys.mShowEmission.isValid() && get(mAttrKeys.mShowEmission) &&
        mAttrKeys.mEmission.isValid() && !isBlack(get(mAttrKeys.mEmission));

    mIspc.mHints.mThinGeometry = mAttrKeys.mThinGeometry.isValid() && get(mAttrKeys.mThinGeometry);

    mIspc.mHints.mPreventLightCulling =
        mAttrKeys.mPreventLightCulling.isValid() && get(mAttrKeys.mPreventLightCulling);

    // Global disable for optimized hair sampling
    const scene_rdl2::rdl2::SceneVariables& sceneVariables =
        getSceneClass().getSceneContext()->getSceneVariables();
    mIspc.mHints.mDisableOptimizedHairSampling =
        sceneVariables.get(scene_rdl2::rdl2::SceneVariables::sDisableOptimizedHairSampling);

    // get SubsurfaceTraceSet
    if (mIspc.mHints.mRequiresSubsurfaceParams) {
        mIspc.mSubsurfaceTraceSet = (ispc::TraceSet *)get(mAttrKeys.mSubsurfaceTraceSet);
    }

    mXform = std::make_unique<moonray::shading::Xform>(this, nullptr, nullptr, nullptr);
    mIspc.mXform = mXform->getIspcXform();

    // Initialize pointers for evaluating normal maps in ISPC
    initializeNormalMap(this,
                        mAttrKeys.mInputNormal,
                        &mIspc.mNormalMap,
                        mIspc.mSampleNormalFunc);

    initializeNormalMap(this,
                        mAttrKeys.mOuterSpecularNormal,
                        &mIspc.mOuterSpecularNormalMap,
                        mIspc.mOuterSpecularSampleNormalFunc);

    initializeNormalMap(this,
                        mAttrKeys.mFuzzNormal,
                        &mIspc.mFuzzNormalMap,
                        mIspc.mFuzzSampleNormalFunc);

    if (mIspc.mModel == ispc::Model::Toon) {
        initializeNormalMap(this,
                            mAttrKeys.mToonSpecularKeys.mInputNormal,
                            &mIspc.mToonSpecularData.mNormalMap,
                            mIspc.mToonSpecularData.mSampleNormalFunc);
    }

    if (mIspc.mModel == ispc::Model::HairToon) {
        initializeNormalMap(this,
                            mAttrKeys.mHairToonS1Keys.mInputNormal,
                            &mIspc.mHairToonS1Data.mNormalMap,
                            mIspc.mHairToonS1Data.mSampleNormalFunc);

        initializeNormalMap(this,
                            mAttrKeys.mHairToonS2Keys.mInputNormal,
                            &mIspc.mHairToonS2Data.mNormalMap,
                            mIspc.mHairToonS2Data.mSampleNormalFunc);

        initializeNormalMap(this,
                            mAttrKeys.mHairToonS3Keys.mInputNormal,
                            &mIspc.mHairToonS3Data.mNormalMap,
                            mIspc.mHairToonS3Data.mSampleNormalFunc);
    }

    resolveUniformParameters(mIspc.mUParams);

    mIspc.mDiffuseLightSet = nullptr;
    mIspc.mSpecularLightSet = nullptr;
}

void
DwaBase::updateGlitter()
{
    // Gather uniform attributes
    ispc::GLITTER_UniformParameters uniformParams;
    uniformParams.mSeed = get(mAttrKeys.mGlitterSeed);
    uniformParams.mSpace = static_cast<ispc::SHADING_Space>(get(mAttrKeys.mGlitterSpace));
    uniformParams.mFlakeRandomness = get(mAttrKeys.mGlitterRandomness);
    uniformParams.mDenseGlitterLodQuality = get(mAttrKeys.mGlitterDenseLodQuality);
    uniformParams.mSearchRadiusFactor = 0.25f;
    uniformParams.mLayeringMode = get(mAttrKeys.mGlitterLayeringMode);
    uniformParams.mDebugMode = static_cast<ispc::GLITTER_DebugModes>(get(mAttrKeys.mGlitterDebugMode));

    // we'll need "ref_P" and "ref_N" if using reference space
    int refPKey = moonray::shading::StandardAttributes::sRefP;
    int refNKey = moonray::shading::StandardAttributes::sRefN;
    if (hasChanged(mAttrKeys.mGlitterSpace)) {
        // don't want to clear mRequiredAttributes in case hair requires scatterTagKey
        std::vector<int>::iterator it = std::find(mRequiredAttributes.begin(), mRequiredAttributes.end(), refPKey);
        if (it != mRequiredAttributes.end() && uniformParams.mSpace != ispc::SHADING_SPACE_REFERENCE) {
            // key found but not ref space, so erase key
            mRequiredAttributes.erase(it);
        } else if (it == mRequiredAttributes.end() && uniformParams.mSpace == ispc::SHADING_SPACE_REFERENCE) {
            // key not found and is ref space, so add key
            mRequiredAttributes.push_back(refPKey);
        }

        mOptionalAttributes.clear();
        if (uniformParams.mSpace == ispc::SHADING_SPACE_REFERENCE) {
            mOptionalAttributes.push_back(refNKey);
        }
    }

    // Create a GlitterFlake library object with all of the shader attributes
    std::vector<glitter::GlitterTexture> filenames;
    const std::string filenameA = get(mAttrKeys.mGlitterFlakeTextureA);
    const float frequencyA = get(mAttrKeys.mGlitterStyleAFrequency);
    if (!filenameA.empty() && !scene_rdl2::math::isZero(frequencyA)) {
        filenames.push_back(std::make_pair(filenameA, frequencyA));
    }

    const std::string filenameB = get(mAttrKeys.mGlitterFlakeTextureB);
    const float frequencyB = get(mAttrKeys.mGlitterStyleBFrequency);
    if (!filenameB.empty() && !scene_rdl2::math::isZero(frequencyB)) {
        filenames.push_back(std::make_pair(filenameB, frequencyB));
    }

    try {
        if (!mGlitterPointer) {
            mGlitterPointer = std::make_unique<glitter::Glitter>(this, filenames, sLogEventRegistry, uniformParams);
        }

    } catch (const std::invalid_argument &e) {
        fatal(e.what());
    } catch (...) {
        fatal("Could not instantiate Glitter library");
    }

    mIspc.mGlitterPointer = mGlitterPointer->getIspc();
    mIspc.mGlitterUniformParameters = mGlitterPointer->getIspcUniformParameters();
}

void
DwaBase::updateIridescence()
{
    const std::vector<float>& points  = get(mAttrKeys.mIridescenceRampPositions);
    const std::vector<Color>& colors  = get(mAttrKeys.mIridescenceRampColors);
    const std::vector<int>&   interps = get(mAttrKeys.mIridescenceRampInterpolators);

    mIspc.mIridescenceData.mRampNumPoints = points.size();
    for (size_t i = 0; i < points.size(); ++i) {
        mIspc.mIridescenceData.mRampPositions[i]     = points[i];
        asCpp(mIspc.mIridescenceData.mRampColors[i]) = colors[i];
        mIspc.mIridescenceData.mRampInterpolators[i] = (ispc::RampInterpolatorMode)interps[i];
    }
}

finline void
resolveNormalParams(const DwaBase* me,
                    moonray::shading::TLState *tls,
                    const moonray::shading::State& state,
                    bool castsCaustics,
                    const ispc::DwaBaseParameterHints& hints,
                    const DwaBaseAttributeKeys& keys,
                    ispc::DwaBaseParameters &params)
{
    if (keys.mInputNormal.isValid()) {
        params.mNormalDial = evalFloat(me, keys.mInputNormalDial, tls, state);
        if (keys.mNormalAAStrategy.isValid()) {
            params.mNormalAAStrategy = static_cast<ispc::NormalAAStrategyType>(me->get(keys.mNormalAAStrategy));
            params.mNormalAADial = me->get(keys.mNormalAADial);
        }
        if (params.mNormalAAStrategy == ispc::NORMAL_AA_STRATEGY_TOKSVIG) {
            // For Toksvig we need the length of the normal before
            // it's normalized therefore we pass in the length by
            // reference.
            scene_rdl2::math::Vec3f unadaptedN;
            asCpp(params.mNormal) = evalNormal(me,
                                               keys.mInputNormal,
                                               params.mNormalDial,
                                               tls,
                                               state,
                                               &params.mNormalLength,
                                               &unadaptedN);
            asCpp(params.mDiffuseNormal) = unadaptedN;
        } else {
            scene_rdl2::math::Vec3f unadaptedN;
            asCpp(params.mNormal) = evalNormal(me,
                                               keys.mInputNormal,
                                               params.mNormalDial,
                                               tls,
                                               state,
                                               nullptr,
                                               &unadaptedN);
            asCpp(params.mDiffuseNormal) = unadaptedN;
        }

        const ispc::DwaBase* dwabase = reinterpret_cast<const ispc::DwaBase* >(getDwaBaseMaterialStruct(me));
        params.mEvalSubsurfaceNormalFn = dwabase->mAttrFuncs.mEvalSubsurfaceNormal;
    } else {
        asCpp(params.mNormal) = state.getN();
        asCpp(params.mDiffuseNormal) = state.getN();
    }
}

finline void
resolveToonDiffuseParams(const DwaBase* me,
                         moonray::shading::TLState *tls,
                         const moonray::shading::State& state,
                         const ispc::ToonDiffuseUniformData& uniformData,
                         const ToonDiffuseKeys& keys,
                         ispc::ToonDiffuseParameters &toonDParams)
{
    toonDParams.mToonDiffuse = 1.0f;
    toonDParams.mModel = me->get(keys.mModel);
    if (toonDParams.mModel == ispc::TOON_DIFFUSE_OREN_NAYAR) {
        toonDParams.mTerminatorShift = scene_rdl2::math::clamp(evalFloat(me, keys.mTerminatorShift, tls, state),
                                                      -1.0f, 1.0f);
        toonDParams.mFlatness = clamp(evalFloat(me, keys.mFlatness, tls, state),
                                                0.0f, 1.0f);
        toonDParams.mFlatnessFalloff = clamp(evalFloat(me, keys.mFlatnessFalloff, tls, state),
                                                       0.0f, 1.0f);
    }

    // Eval normal separately as a toon normal so adaptNormal
    // can be disabled for only diffuse lobes
    const float normalDial = evalFloat(me, keys.mInputNormalDial, tls, state);
    scene_rdl2::math::Vec3f unadaptedN;
    evalToonNormal(me,
                   keys.mInputNormal,
                   normalDial,
                   tls,
                   state,
                   nullptr,
                   &unadaptedN);
    asCpp(toonDParams.mNormal) = unadaptedN;

    if (toonDParams.mModel == ispc::TOON_DIFFUSE_RAMP_V1 ||
        toonDParams.mModel == ispc::TOON_DIFFUSE_RAMP_V2) {

        toonDParams.mRampWeight = 1.0f;

        // Get the ramp data points and validate them
        toonDParams.mRampNumPoints = min(ispc::DWABASE_MAX_TOOND_RAMP_POINTS,
                                         uniformData.mRampNumPoints);

        for(int i = 0; i < toonDParams.mRampNumPoints; ++i) {
            const float offset = evalFloat(me,
                                           keys.mRampOffsets[i],
                                           tls, state);
            toonDParams.mRampPositions[i] = uniformData.mRampPositions[i] + offset;
            scene_rdl2::math::asCpp(toonDParams.mRampColors[i]) = asCpp(uniformData.mRampColors[i]) *
                                                      evalColor(me, keys.mRampColors[i], tls, state);
            toonDParams.mRampInterpolators[i] = uniformData.mRampInterpolators[i];
        }
        toonDParams.mExtendRamp = me->get(keys.mExtendRamp);
        toonDParams.mRampInputScale = evalFloat(me, keys.mRampInputScale, tls, state);
    }
}

finline void
resolveToonSpecParams(const DwaBase* me,
                      moonray::shading::TLState *tls,
                      const moonray::shading::State& state,
                      const float specular,
                      const ispc::ToonSpecularUniformData& uniformData,
                      const ToonSpecularKeys& keys,
                      ispc::ToonSpecularParameters &params,
                      ispc::ToonSpecularModel model)
{
    params.mToonSpecular = specular;

    params.mIntensity = evalFloat(me, keys.mIntensity, tls, state);
    params.mFresnelBlend = evalFloat(me, keys.mFresnelBlend, tls, state);
    params.mRoughness = evalFloat(me, keys.mRoughness, tls, state);
    asCpp(params.mTint) = clamp(evalColor(me,keys.mTint, tls, state),
                                          sBlack, sWhite);
    params.mRampInputScale = evalFloat(me, keys.mRampInputScale, tls, state);

    params.mRampNumPoints = min(ispc::DWABASE_MAX_TOOND_RAMP_POINTS,
                                     uniformData.mRampNumPoints);
    for(int i = 0; i < params.mRampNumPoints; ++i) {
        const float offset = evalFloat(me,
                                       keys.mRampOffsets[i],
                                       tls, state);
        params.mRampPositions[i] = uniformData.mRampPositions[i] + offset;
        params.mRampValues[i] = uniformData.mRampValues[i] * evalFloat(me, keys.mRampMultipliers[i], tls, state);
        params.mRampInterpolators[i] = uniformData.mRampInterpolators[i];
    }

    if (me->get(keys.mEnableInputNormal)) {
        const float normalDial = evalFloat(me, keys.mInputNormalDial, tls, state);
        asCpp(params.mNormal) = evalNormal(me, keys.mInputNormal, normalDial, tls, state);
    } else {
        asCpp(params.mNormal) = state.getN();
    }

    params.mStretchU = evalFloat(me, keys.mStretchU, tls, state);
    params.mStretchV = evalFloat(me, keys.mStretchV, tls, state);
    if (me->get(keys.mUseInputVectors)) {
        asCpp(params.mdPds) = normalize(evalVec3f(me, keys.mInputU, tls, state));
        asCpp(params.mdPdt) = normalize(evalVec3f(me, keys.mInputV, tls, state));
    } else {
        asCpp(params.mdPds) = normalize(state.getdPds());
        asCpp(params.mdPdt) = normalize(state.getdPdt());
    }

    params.mEnableIndirectReflections = me->get(keys.mEnableIndirectReflections);
    if (params.mEnableIndirectReflections) {
        params.mIndirectReflectionsIntensity = evalFloat(me, keys.mIndirectReflectionsIntensity, tls, state);
        params.mIndirectReflectionsRoughness = evalFloat(me, keys.mIndirectReflectionsRoughness, tls, state);
    }

    if (model == ispc::ToonSpecularModel::ToonSpecularHair) {
        asCpp(params.mHairDir) = normalize(state.getdPds());
        asCpp(params.mHairUV) = state.getSt();
    }
}

finline void
resolveToonParams(const DwaBase* me,
                  moonray::shading::TLState *tls,
                  const moonray::shading::State& state,
                  const ispc::DwaBaseParameterHints& hints,
                  const DwaBaseAttributeKeys& keys,
                  ispc::DwaBaseParameters &params)
{
    if (hints.mRequiresToonDiffuseParams) {
        resolveToonDiffuseParams(me, tls, state,
                                 me->getISPCBaseMaterialStruct()->mToonDiffuseData,
                                 keys.mToonDiffuseKeys,
                                 params.mToonDiffuseParams);
    }

    if (hints.mRequiresToonSpecularParams) {
        resolveToonSpecParams(me, tls, state,
                              params.mSpecular,
                              me->getISPCBaseMaterialStruct()->mToonSpecularData,
                              keys.mToonSpecularKeys,
                              params.mToonSpecularParams,
                              me->getISPCBaseMaterialStruct()->mUParams.mToonSpecularModel);
        // When the toon specular doesn't have its own input normal enabled, or
        // it is enabled but no specific normal map is connected, fall back to
        // params.mNormal
        if (!me->get(keys.mToonSpecularKeys.mEnableInputNormal) ||
            me->get(keys.mToonSpecularKeys.mInputNormal) == nullptr) {
            asCpp(params.mToonSpecularParams.mNormal) = asCpp(params.mNormal);
        }
    }
}

finline void
resolveHairToonParams(const DwaBase* me,
                      moonray::shading::TLState *tls,
                      const moonray::shading::State& state,
                      const ispc::DwaBaseParameterHints& hints,
                      const DwaBaseAttributeKeys& keys,
                      ispc::DwaBaseParameters &params)
{
    const ispc::DwaBase* dwabase = me->getISPCBaseMaterialStruct();

    if (hints.mRequiresHairToonS1Params) {
        resolveToonSpecParams(me, tls, state,
                              1.0f, // specular
                              dwabase->mHairToonS1Data,
                              keys.mHairToonS1Keys,
                              params.mHairToonS1Params,
                              me->getISPCBaseMaterialStruct()->mUParams.mHairToonS1Model);

    }

    if (hints.mRequiresHairToonS2Params) {
        resolveToonSpecParams(me, tls, state,
                              1.0f, // specular
                              dwabase->mHairToonS2Data,
                              keys.mHairToonS2Keys,
                              params.mHairToonS2Params,
                              me->getISPCBaseMaterialStruct()->mUParams.mHairToonS2Model);
    }

    if (hints.mRequiresHairToonS3Params) {
        resolveToonSpecParams(me, tls, state,
                              1.0f, // specular
                              dwabase->mHairToonS3Data,
                              keys.mHairToonS3Keys,
                              params.mHairToonS3Params,
                              me->getISPCBaseMaterialStruct()->mUParams.mHairToonS3Model);
    }
}

finline void
resolveHairParams(const DwaBase* me,
                  moonray::shading::TLState *tls,
                  const moonray::shading::State& state,
                  bool castsCaustics,
                  const ispc::DwaBase& ispcDwaBase,
                  const ispc::DwaBaseParameterHints& hints,
                  const DwaBaseAttributeKeys& keys,
                  ispc::HairParameters &params)
{
    if (castsCaustics || !state.isCausticPath()) {
        params.mHairCastsCaustics = true;
    }

    // Common Params
    if (hints.mRequiresHairParams || hints.mRequiresHairDiffuseParams) {
        asCpp(params.mHairDir) = normalize(state.getdPds());
    }

    if (hints.mRequiresHairParams && !hints.mHairDiffuseIsOne) {
        asCpp(params.mHairColor) = clamp(evalColor(me, keys.mHairColor, tls, state),
                                         sBlack,
                                         sWhite);
    }

    // HairMaterial_v3 parameters
    if (hints.mRequiresHairParams) {
        if (ispcDwaBase.mModel == ispc::Model::Hair || hints.mHairIsOne) {
            params.mHair = 1.0f;
        } else {
            params.mHair = saturate(evalFloat(me, keys.mHair, tls, state));
        }

        if (!isZero(params.mHair) && params.mHairCastsCaustics) {
            /// @brief An Energy Conserving Hair Material based on the following papers:
            /// [1] An Energy-conserving Hair Reflectance Model - D'eon et al Sig'11
            /// [2] A Practical and Controllable Hair and Fur Model for Production Path Tracing - Chiang et al EGSR'16
            /// [3] Importance Sampling for Physically-Based Hair Fiber Models, D'eon et al Sig'13
            /// [4] Light Scattering from Human Hair Fibers - Marschner et al

            // Hair Need Azimuthal Roughness To Compute Absorption Coefficients
            params.mHairTTAzimRoughness =
                clamp(evalFloat(me, keys.mHairTTAzimuthalRoughness, tls, state),
                                0.01f,
                                1.0f - sEpsilon);

            asCpp(params.mHairUV) = state.getSt();

            params.mHairIOR = me->get(keys.mRefractiveIndex);

            // Evaluate input attributes we need
            params.mHairRLongRoughness = evalFloat(me, keys.mHairRRoughness, tls, state);
            params.mHairShowR = me->get(keys.mHairShowR);

            if (params.mHairShowR) {
                // No Specular Offset beyond 10 degrees max makes sense for hair
                // Marschner suggests this should be [2,5] degrees
                const float specularOffset = clamp(evalFloat(me, keys.mHairROffset, tls, state), -10.0f, 10.0f);
                params.mHairRShift = deg2rad(specularOffset);

                // Tint
                asCpp(params.mHairRTint) = evalColor(me, keys.mHairRTintColor, tls, state);
            }

            // Transmission (TT)
            params.mHairShowTT = me->get(keys.mHairShowTT);
            // Roughness according to Table 1 in [4]
            params.mHairTTLongRoughness  = 0.5f * params.mHairRLongRoughness;
            if (params.mHairShowTT) {
                if (me->get(keys.mHairTTUseRoughness)) {
                    params.mHairTTLongRoughness = evalFloat(me, keys.mHairTTRoughness, tls, state);
                }
                const float ttOffset = clamp(evalFloat(me, keys.mHairTTOffset, tls, state), -10.0f, 10.0f);
                params.mHairTTShift = deg2rad(ttOffset);

                // Tint
                asCpp(params.mHairTTTint) = evalColor(me, keys.mHairTTTintColor, tls, state);
                // MOONSHINE-1238: only for HiFi hits
                if (state.isHifi()) {
                    params.mHairTTSaturation = evalFloat(me, keys.mHairTTSaturation, tls, state);
                } else {
                    params.mHairTTSaturation = 1.0f;
                }
            }

            // Directional Diffuse (TRT)
            params.mHairShowTRT = me->get(keys.mHairShowTRT);
            // Roughness according to Table 1 in [4]
            params.mHairTRTLongRoughness = 2.0f * params.mHairRLongRoughness;
            if (params.mHairShowTRT) {
                if (me->get(keys.mHairTRTUseRoughness) == true) {
                    params.mHairTRTLongRoughness = evalFloat(me, keys.mHairTRTRoughness, tls, state);
                }
                const float trtOffset = clamp(evalFloat(me, keys.mHairTRTOffset, tls, state), -10.0f, 10.0f);
                params.mHairTRTShift = deg2rad(trtOffset);

                // Tint
                asCpp(params.mHairTRTTint) = evalColor(me, keys.mHairTRTTintColor, tls, state);

                params.mHairShowGlint = me->get(keys.mHairShowGlint);
                if (params.mHairShowGlint) {
                    params.mHairGlintRoughness = evalFloat(me, keys.mHairGlintRoughness, tls, state);
                    params.mHairGlintMinTwists = evalFloat(me, keys.mHairGlintMinTwists, tls, state);
                    params.mHairGlintMaxTwists = evalFloat(me, keys.mHairGlintMaxTwists, tls, state);
                    params.mHairGlintEccentricity = evalFloat(me, keys.mHairGlintEccentricity, tls, state);
                    params.mHairGlintSaturation = evalFloat(me, keys.mHairGlintSaturation, tls, state);
                }
            }

            params.mHairShowTRRT = me->get(keys.mHairShowTRRTPlus);
            // Section 3.4 in [2]
            // Using a wide Longitudinal Lobe (4*hairParams.mRLongRoughness)
            // Internally it uses an Isotropic Azimuthal term
            params.mHairTRRTLongRoughness = 4.0f * params.mHairRLongRoughness;

            params.mHairFresnelType = (ispc::HairFresnelType)me->get(keys.mHairFresnelType);

            params.mHairCuticleLayerThickness =
                clamp(evalFloat(me, keys.mHairCuticleLayerThickness, tls, state), 0.0f, 1.0f);

            params.mHairUseOptimizedSampling = hints.mDisableOptimizedHairSampling ? false : me->get(keys.mHairUseOptimizedSampling);
        }
    }

    // HairDiffuseMaterial parameters
    if (hints.mRequiresHairDiffuseParams) {
        if (ispcDwaBase.mModel == ispc::Model::HairDiffuse || hints.mHairDiffuseIsOne) {
            params.mHairDiffuse = 1.0f;
        } else {
            params.mHairDiffuse = saturate(evalFloat(me, keys.mHairDiffuse, tls, state));
        }

        if (!isZero(params.mHairDiffuse)) {
            params.mHairDiffuseUseIndependentFrontAndBackColor =
                me->get(keys.mHairiDiffuseUseIndependentFrontAndBackColor);

            params.mHairSubsurfaceBlend = saturate(evalFloat(me, keys.mHairSubsurfaceBlend, tls, state));

            if (params.mHairDiffuseUseIndependentFrontAndBackColor) {
                asCpp(params.mHairDiffuseFrontColor) =
                    clamp(evalColor(me, keys.mHairDiffuseFrontColor, tls, state),
                                    sBlack,
                                    sWhite);

                asCpp(params.mHairDiffuseBackColor) =
                    clamp(evalColor(me, keys.mHairDiffuseBackColor, tls, state),
                                    sBlack,
                                    sWhite);
            } else {
                asCpp(params.mHairDiffuseFrontColor) =
                    clamp(evalColor(me, keys.mHairColor, tls, state),
                                    sBlack,
                                    sWhite);
                params.mHairDiffuseBackColor = params.mHairDiffuseFrontColor;
            }
        }
    }
}

finline void
resolveFuzzParams(const DwaBase* me,
                  moonray::shading::TLState *tls,
                  const moonray::shading::State& state,
                  bool castsCaustics,
                  const ispc::DwaBaseParameterHints& hints,
                  const DwaBaseAttributeKeys& keys,
                  ispc::DwaBaseParameters &params)
{
    if (hints.mRequiresFuzzParams &&
        (castsCaustics || !state.isCausticPath())) {

        params.mFuzz                 = saturate(evalFloat(me, keys.mFuzz, tls, state));
        asCpp(params.mFuzzAlbedo)    = clamp(evalColor(me, keys.mFuzzAlbedo, tls, state));
        params.mFuzzRoughness        = saturate(evalFloat(me, keys.mFuzzRoughness, tls, state));

        params.mFuzzNormalDial = evalFloat(me, keys.mFuzzNormalDial, tls, state);
        asCpp(params.mFuzzNormal) = evalNormal(me,
                                               keys.mFuzzNormal,
                                               params.mFuzzNormalDial,
                                               tls,
                                               state);

        params.mFuzzUseAbsorbingFibers = me->get(keys.mFuzzUseAbsorbingFibers);
    }
}

finline void
resolveOuterSpecularParams(const DwaBase* me,
                           moonray::shading::TLState *tls,
                           const moonray::shading::State& state,
                           bool castsCaustics,
                           const ispc::DwaBase& ispcDwaBase,
                           const ispc::DwaBaseParameterHints& hints,
                           const DwaBaseAttributeKeys& keys,
                           ispc::DwaBaseParameters &params)
{
    asCpp(params.mOuterSpecularNormal) = state.getN();
    if (hints.mRequiresOuterSpecularParams &&
        (castsCaustics || !state.isCausticPath())) {
        params.mOuterSpecular = saturate(evalFloat(me, keys.mOuterSpecular, tls, state));
        params.mOuterSpecularRefractiveIndex = max(sEpsilon, me->get(keys.mOuterSpecularRefractiveIndex));
        params.mOuterSpecularRoughness = saturate(evalFloat(me, keys.mOuterSpecularRoughness, tls, state));

        params.mOuterSpecularNormalDial = evalFloat(me, keys.mOuterSpecularNormalDial, tls, state);
        if (me->get(keys.mUseOuterSpecularNormal)) {
            asCpp(params.mOuterSpecularNormal) = evalNormal(me,
                                                            keys.mOuterSpecularNormal,
                                                            params.mOuterSpecularNormalDial,
                                                            tls,
                                                            state,
                                                            &params.mOuterSpecularNormalLength);
            params.mOuterSpecularNormalDial = me->get(keys.mOuterSpecularNormalDial);
        }

        if (hints.mRequiresOuterSpecularAbsorptionParams) {
            params.mOuterSpecularThickness =
                max(0.0f, evalFloat(me, keys.mOuterSpecularThickness, tls, state));

            asCpp(params.mOuterSpecularAttenuationColor) =
                clamp(evalColor(me, keys.mOuterSpecularAttenuationColor, tls, state));
        }
    }
}

finline void
resolveIridescenceParams(const DwaBase* me,
                         moonray::shading::TLState *tls,
                         const moonray::shading::State& state,
                         bool castsCaustics,
                         const ispc::DwaBase& ispcDwaBase,
                         const ispc::DwaBaseParameterHints& hints,
                         const DwaBaseAttributeKeys& keys,
                         ispc::IridescenceParameters &params)
{
    if (hints.mRequiresIridescenceParams &&
        (castsCaustics || !state.isCausticPath())) {
        params.mIridescence = saturate(evalFloat(me, keys.mIridescence, tls, state));
        params.mIridescenceApplyTo = (ispc::IridescenceLobe)me->get(keys.mIridescenceApplyTo);
        params.mIridescenceColorControl = (ispc::SHADING_IridescenceColorMode)me->get(keys.mIridescenceColorControl);
        asCpp(params.mIridescencePrimaryColor) = clamp(evalColor(me, keys.mIridescencePrimaryColor, tls, state));
        asCpp(params.mIridescenceSecondaryColor) = clamp(evalColor(me, keys.mIridescenceSecondaryColor, tls, state));
        params.mIridescenceFlipHueDirection = me->get(keys.mIridescenceFlipHueDirection);
        params.mIridescenceThickness = max(0.0f, evalFloat(me, keys.mIridescenceThickness, tls, state));
        params.mIridescenceExponent = max(0.0f, evalFloat(me, keys.mIridescenceExponent, tls, state));
        params.mIridescenceAt0 = clamp(evalFloat(me, keys.mIridescenceAt0, tls, state));
        params.mIridescenceAt90 = clamp(evalFloat(me, keys.mIridescenceAt90, tls, state));

        params.mIridescenceRampInterpolationMode = (ispc::ColorRampControlSpace)me->get(keys.mIridescenceRampInterpolationMode);
        const ispc::DwaBase* dwabase = me->getISPCBaseMaterialStruct();
        // Get the ramp data points and validate them
        params.mIridescenceRampNumPoints = min(ispc::DWABASE_MAX_IRIDESCENCE_RAMP_POINTS,
                                    dwabase->mIridescenceData.mRampNumPoints);

        // Copy ramp point data gathered in update()
        for(int i = 0; i < params.mIridescenceRampNumPoints; ++i) {
            params.mIridescenceRampPositions[i] = dwabase->mIridescenceData.mRampPositions[i];
            scene_rdl2::math::asCpp(params.mIridescenceRampColors[i]) = asCpp(dwabase->mIridescenceData.mRampColors[i]);
            params.mIridescenceRampInterpolators[i] = dwabase->mIridescenceData.mRampInterpolators[i];
        }
    }
}

finline void
resolveSpecularParams(const DwaBase* me,
                      moonray::shading::TLState *tls,
                      const moonray::shading::State& state,
                      bool castsCaustics,
                      const ispc::DwaBase& ispcDwaBase,
                      const ispc::DwaBaseParameterHints& hints,
                      const DwaBaseAttributeKeys& keys,
                      ispc::DwaBaseParameters &params)
{
    if (hints.mRequiresSpecularParams &&
        (castsCaustics || !state.isCausticPath())) {
        params.mSpecular = saturate(evalFloat(me, keys.mSpecular, tls, state));
        params.mRoughness = saturate(evalFloat(me, keys.mRoughness, tls, state));

        if (hints.mRequiresAnisotropyParams) {
            params.mAnisotropy = evalFloat(me, keys.mAnisotropy, tls, state);
            params.mAnisotropy = clamp(params.mAnisotropy, -1.0f, 1.0f);
            if (!isZero(params.mAnisotropy)) {
                asCpp(params.mShadingTangent) = evalVec2f(me, keys.mShadingTangent, tls, state);
            }
        }
    }

    if (hints.mRequiresMetallicParams &&
        (castsCaustics || !state.isCausticPath())) {
        if (ispcDwaBase.mModel == ispc::Model::Metal || hints.mMetallicIsOne) {
            params.mMetallic = 1.0f;
        } else {
            params.mMetallic = saturate(evalFloat(me, keys.mMetallic, tls, state));
        }
        asCpp(params.mMetallicColor) = clamp(evalColor(me, keys.mMetallicColor, tls, state));
        asCpp(params.mMetallicEdgeColor) = clamp(evalColor(me, keys.mMetallicEdgeColor, tls, state));
    }
}

finline void
resolveTransmissionParams(const DwaBase* me,
                          moonray::shading::TLState *tls,
                          const moonray::shading::State& state,
                          bool castsCaustics,
                          const ispc::DwaBase& ispcDwaBase,
                          const ispc::DwaBaseParameterHints& hints,
                          const DwaBaseAttributeKeys& keys,
                          ispc::DwaBaseParameters &params)
{
    if (!hints.mMetallicIsOne && hints.mRequiresTransmissionParams &&
        (castsCaustics || !state.isCausticPath())) {
        // Evaluate roughness here if specular params are not required
        if (!hints.mRequiresSpecularParams) {
            params.mRoughness = saturate(evalFloat(me, keys.mRoughness, tls, state));
        }
        if (ispcDwaBase.mModel == ispc::Model::Refractive) {
            params.mTransmission = 1.0f;
        } else {
            params.mTransmission = saturate(evalFloat(me, keys.mTransmission, tls, state));
        }
        asCpp(params.mTransmissionColor) = clamp(evalColor(me, keys.mTransmissionColor, tls, state));

        if (hints.mRequiresTransmissionRefractiveIndex) {
            params.mUseIndependentTransmissionRefractiveIndex = true;
            params.mIndependentTransmissionRefractiveIndex =
                max(sEpsilon, keys.mIndependentTransmissionRefractiveIndex.isValid() ?
                    me->get(keys.mIndependentTransmissionRefractiveIndex) : 1.5f);
        }

        params.mUseIndependentTransmissionRoughness = me->get(keys.mUseIndependentTransmissionRoughness);
        if (params.mUseIndependentTransmissionRoughness) {
            params.mIndependentTransmissionRoughness =
                    saturate(evalFloat(me, keys.mIndependentTransmissionRoughness, tls, state));
        } else {
            params.mIndependentTransmissionRoughness = params.mRoughness;
        }

        if (me->get(keys.mUseDispersion))
            params.mDispersionAbbeNumber = max(0.0f, me->get(keys.mDispersionAbbeNumber));
        else {
            // No Dispersion
            params.mDispersionAbbeNumber = 0.0f;
        }
    }
}

finline void
resolveFabricSpecularParams(const DwaBase* me,
                            moonray::shading::TLState *tls,
                            const moonray::shading::State& state,
                            bool castsCaustics,
                            const ispc::DwaBase& ispcDwaBase,
                            const ispc::DwaBaseParameterHints& hints,
                            const DwaBaseAttributeKeys& keys,
                            ispc::DwaBaseParameters &params)
{
    if (!hints.mMetallicIsOne && !hints.mTransmissionIsOne &&
        hints.mRequiresFabricParams) {

        params.mFabricSpecular = (ispcDwaBase.mModel == ispc::Model::Fabric) ? 1.0f : 0.0f;

        // For fabric, the specular lobes are effectively scaled
        // down as "fabric diffuse scattering" is increased.  This is somewhat
        // opposite how the other shading models work where the diffuse energy
        // is determined by a OneMinusFresnel.  Here, we handle me energy
        // distribution now as we resolve the parameters. At lobe creation time
        // the diffuse is then scaled by 1-fabricSpecular.
        const float fabricDiffuseScattering = saturate(evalFloat(me, keys.mFabricDiffuseScattering, tls, state));
        params.mFabricSpecular *= (1.0f - fabricDiffuseScattering);

        if (!castsCaustics && state.isCausticPath()) {
            // If this is a caustic path and we aren't casting caustics, we can
            // compute fabric attenuation for lobes underneath and exit.
            params.mFabricAttenuation = (1.0f - params.mFabricSpecular);
            return;
        }

        // Common parameters between Fabric and Velvet.
        asCpp(params.mWarpColor) = clamp(evalColor(me, keys.mWarpColor, tls, state));
        params.mWarpRoughness = saturate(evalFloat(me, keys.mWarpRoughness, tls, state));

        if (hints.mRequiresFabricParams) { // Not velvet

            // Clamp the thread elevation to slightly less than 90 degrees
            params.mWarpThreadElevation = clamp(evalFloat(me, keys.mWarpThreadElevation, tls, state), -89.0f, 89.0f);
            params.mUseIndependentWeftAttributes = me->get(keys.mUseIndependentWeftAttributes);

            if (params.mUseIndependentWeftAttributes) {
                asCpp(params.mWeftColor) = clamp(evalColor(me, keys.mWeftColor, tls, state));
                params.mWeftRoughness = saturate(evalFloat(me, keys.mWeftRoughness, tls, state));
            } else {
                params.mWeftColor = params.mWarpColor;
                params.mWeftRoughness = params.mWarpRoughness;
            }

            if (me->get(keys.mUseUVsForThreadDirection)) {
                // Use dPds as the tangent for Fabric ReferenceFrame
                asCpp(params.mFabricTangent) = normalize(state.getdPds());
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
                asCpp(params.mFabricTangent) = scene_rdl2::math::Vec3f(1.0f, 0.0f, 0.0f);
            }
            asCpp(params.mWarpThreadDirection) = normalize(evalVec3f(me, keys.mWarpThreadDirection, tls, state));
            params.mWarpThreadCoverage = saturate(me->get(keys.mWarpThreadCoverage));
        }
    }
}

finline void
resolveAccentParams(const DwaBase* me,
                    moonray::shading::TLState *tls,
                    const moonray::shading::State& state,
                    bool castsCaustics,
                    const ispc::DwaBase& ispcDwaBase,
                    const ispc::DwaBaseParameterHints& hints,
                    const DwaBaseAttributeKeys& keys,
                    ispc::DwaBaseParameters &params)
{
    params.mAccentParams.mSubsurface = 1.0f;
    asCpp(params.mAccentParams.mSubsurfaceNormal) = asCpp(params.mDiffuseNormal);
    params.mAccentParams.mSubsurfaceNormalDial = params.mNormalDial;
    asCpp(params.mAccentParams.mSubsurfaceColor) = asCpp(params.mAlbedo);
}

int
DwaBase::resolveSubsurfaceType(const State& state) const
{
    return mIspc.mUParams.mSubsurface;
}

void
DwaBase::resolveUniformParameters(ispc::DwaBaseUniformParameters &uParams) const
{
    // init uniform params
    DwaBaseLayerable::initUniformParameters(uParams);

    uParams.mThinGeometry = mIspc.mHints.mThinGeometry;
    uParams.mPreventLightCulling = mIspc.mHints.mPreventLightCulling;

    if (mIspc.mHints.mRequiresToonSpecularParams) {
        uParams.mToonSpecularModel = static_cast<ispc::ToonSpecularModel>(get(mAttrKeys.mToonSpecularKeys.mModel));
    }
    if (mIspc.mHints.mRequiresHairToonS1Params) {
        uParams.mHairToonS1Model = static_cast<ispc::ToonSpecularModel>(get(mAttrKeys.mHairToonS1Keys.mModel));
    }
    if (mIspc.mHints.mRequiresHairToonS2Params) {
        uParams.mHairToonS2Model = static_cast<ispc::ToonSpecularModel>(get(mAttrKeys.mHairToonS2Keys.mModel));
    }
    if (mIspc.mHints.mRequiresHairToonS3Params) {
        uParams.mHairToonS3Model = static_cast<ispc::ToonSpecularModel>(get(mAttrKeys.mHairToonS3Keys.mModel));
    }
    if (mIspc.mHints.mRequiresSpecularParams) {
        uParams.mSpecularModel = get(mAttrKeys.mSpecularModel);
    }
    if (mIspc.mHints.mRequiresOuterSpecularParams) {
        uParams.mOuterSpecularModel = get(mAttrKeys.mOuterSpecularModel);
        uParams.mOuterSpecularUseBending = mAttrKeys.mOuterSpecularUseBending.isValid() ?
                                           get(mAttrKeys.mOuterSpecularUseBending) : false;
    }

    if (mIspc.mHints.mRequiresSubsurfaceParams) {
        uParams.mSubsurface = static_cast<ispc::SubsurfaceType>(get(mAttrKeys.mSubsurface));
    }
}

bool
DwaBase::resolveParameters(moonray::shading::TLState *tls,
                           const State& state,
                           const bool castsCaustics,
                           ispc::DwaBaseParameters &params) const
{
    // The following uniform params are set in update
    // with resolveUniformParameters:
    // mOuterSpecularModel
    // mOuterSpecularUseBending
    // mSpecularModel
    // mSubsurface
    // mThinGeometry
    // mPreventLightCulling

    // make sure all fields are initialized
    DwaBaseLayerable::initParameters(params);

    // We want to avoid evaluating any attributes which are not ultimately needed,
    // especially in the context of indirect or caustic light paths.  We use the
    // 'hints' that were computed in update() to efficiently skip evaluation of
    // unneeded attributes.

    resolveNormalParams(this, tls, state, castsCaustics, mIspc.mHints, mAttrKeys, params);

    params.mRefractiveIndex = max(sEpsilon,
            mAttrKeys.mRefractiveIndex.isValid() ?  get(mAttrKeys.mRefractiveIndex) : 1.5f);

    resolveSpecularParams(this, tls, state, castsCaustics, mIspc, mIspc.mHints, mAttrKeys, params);
    resolveHairParams(this, tls, state, castsCaustics, mIspc, mIspc.mHints, mAttrKeys, params.mHairParameters);
    resolveToonParams(this, tls, state, mIspc.mHints, mAttrKeys, params);
    resolveHairToonParams(this, tls, state, mIspc.mHints, mAttrKeys, params);
    resolveGlitterParams(this, tls, state, castsCaustics, mIspc, mIspc.mHints, mAttrKeys, params);
    resolveFuzzParams(this, tls, state, castsCaustics, mIspc.mHints, mAttrKeys, params);
    resolveIridescenceParams(this, tls, state, castsCaustics, mIspc, mIspc.mHints, mAttrKeys, params.mIridescenceParameters);
    resolveOuterSpecularParams(this, tls, state, castsCaustics, mIspc, mIspc.mHints, mAttrKeys, params);
    resolveTransmissionParams(this, tls, state, castsCaustics, mIspc, mIspc.mHints, mAttrKeys, params);
    resolveFabricSpecularParams(this, tls, state, castsCaustics, mIspc, mIspc.mHints, mAttrKeys, params);

    if (mIspc.mHints.mRequiresDiffuseParams) {
        asCpp(params.mAlbedo) = clamp(evalColor(this, mAttrKeys.mAlbedo, tls, state));
    }

    if (mIspc.mHints.mRequiresDiffuseParams || mIspc.mHints.mRequiresToonDiffuseParams) {
        params.mDiffuseRoughness = saturate(evalFloat(this, mAttrKeys.mDiffuseRoughness, tls, state));
    }

    if (mIspc.mHints.mRequiresSubsurfaceParams && state.isSubsurfaceAllowed()) {
        asCpp(params.mScatteringRadius) =
            clamp(evalColor(this, mAttrKeys.mScatteringColor, tls, state)) *
            max(0.0f, evalFloat(this, mAttrKeys.mScatteringRadius, tls, state));
        params.mCreaseAttenuation = max(0.0f, mAttrKeys.mCreaseAttenuation.isValid() ?
                get(mAttrKeys.mCreaseAttenuation) : 0.0f);
        params.mSubsurfaceTraceSet = (ispc::TraceSet *)get(mAttrKeys.mSubsurfaceTraceSet);
        params.mSSSResolveSelfIntersections = mAttrKeys.mSSSResolveSelfIntersections.isValid() ?
                get(mAttrKeys.mSSSResolveSelfIntersections) : true;
    }

    if (mIspc.mHints.mRequiresDiffuseTransmissionParams) {
        const float diffuseTransmission = clamp(evalFloat(this, mAttrKeys.mDiffuseTransmission, tls, state),
                                                0.0f, 1.0f);
        asCpp(params.mDiffuseTransmission) = diffuseTransmission *
                                             clamp(evalColor(this, mAttrKeys.mDiffuseTransmissionColor, tls, state));
        params.mDiffuseTransmissionBlendingBehavior = get(mAttrKeys.mDiffuseTransmissionBlendingBehavior);
    }

    if (mIspc.mHints.mRequiresEmissionParams) {
        asCpp(params.mEmission) = max(sBlack, evalColor(this, mAttrKeys.mEmission, tls, state));
    }

    params.mDiffuseLightSet = mIspc.mDiffuseLightSet;
    params.mSpecularLightSet = mIspc.mSpecularLightSet;

    resolveAccentParams(this, tls, state, castsCaustics, mIspc, mIspc.mHints, mAttrKeys, params);

    applyColorCorrectParameters(tls, state, params);

    // ---------------
    // for debug_pixel
    // ---------------
    //std::cout << params << std::endl;

    return true;
}

scene_rdl2::math::Vec3f
DwaBase::resolveSubsurfaceNormal(moonray::shading::TLState *tls,
                                 const moonray::shading::State& state) const
{
    return evalNormal(this,
                      mAttrKeys.mInputNormal,
                      evalFloat(this, mAttrKeys.mInputNormalDial, tls, state),
                      tls,
                      state);
}

float
DwaBase::resolvePresence(moonray::shading::TLState *tls,
                         const State& state) const
{
    if (mAttrKeys.mPresence.isValid()) {
        return saturate(evalFloat(this, mAttrKeys.mPresence, tls, state));
    }
    return 1.f;
}

float
DwaBase::resolveRefractiveIndex(moonray::shading::TLState * /*tls*/,
                                const State& /*state*/) const
{
    if (mAttrKeys.mUseIndependentTransmissionRefractiveIndex.isValid()) {
        if (get(mAttrKeys.mUseIndependentTransmissionRefractiveIndex)) {
            return get(mAttrKeys.mIndependentTransmissionRefractiveIndex);
        }
    }
    if (mAttrKeys.mRefractiveIndex.isValid()) {
        return get(mAttrKeys.mRefractiveIndex);
    }
    return 1.f;
}

bool
DwaBase::resolvePreventLightCulling(const State& state) const
{
    return mIspc.mHints.mPreventLightCulling;
}

} // namespace dwabase
} // namespace moonshine

