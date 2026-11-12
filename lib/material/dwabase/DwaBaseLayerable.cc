// Copyright 2023-2024 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

///
/// @file DwaBaseLayerable.cc
/// $Id$
///

#include "DwaBaseLayerable.h"

#include <moonray/common/mcrt_macros/moonray_static_check.h>
#include <moonray/common/mcrt_util/Atomic.h>
#include <moonray/rendering/shading/ColorCorrect.h>
#include <moonray/rendering/shading/MaterialApi.h>
#include <scene_rdl2/common/math/ReferenceFrame.h>
#include <scene_rdl2/render/logging/logging.h>

namespace {

// This is a construct to access the ISPC Data Struct from Inside the Scalar Material object.
// Note we are not using MATERIAL_GET_ISPC_CPTR to retrieve the ispc structure from the Material
// class as in other materials since that macro assumes a certain data layout we do not adhere to in
// DwaBase materials.
extern "C" {
const void*
getLabelsDataPtr(const void* ptr)
{
    const moonshine::dwabase::DwaBaseLayerable* classPtr = static_cast<const moonshine::dwabase::DwaBaseLayerable*>(ptr);
    return (const void*)classPtr->getLabelsDataStruct();
}

const void*
getISPCDwaBaseLayerablePtr(const void* ptr)
{
    const moonshine::dwabase::DwaBaseLayerable* classPtr = static_cast<const moonshine::dwabase::DwaBaseLayerable*>(ptr);
    return (const void*)classPtr->getISPCDwaBaseLayerableStruct();
}
}

} // anonymous namespace

namespace moonshine {
namespace dwabase {

using namespace scene_rdl2::math;
using namespace moonray::shading;

static ispc::DwaBaseEventMessages sEventMessages;

void
DwaBaseLayerable::registerShadeTimeEventMessages()
{
    mIspc.mEventMessagesPtr = (ispc::DwaBaseEventMessages*)&sEventMessages;

    const auto errorNoRefN = sLogEventRegistry.createEvent(scene_rdl2::logging::ERROR_LEVEL,
                             "Unable to acquire refN which is required for glitter. Glitter cannot be applied");

    const auto warnNoRefPpartials = sLogEventRegistry.createEvent(scene_rdl2::logging::WARN_LEVEL,
                                    "No partial derivatives associated with refP. "
                                    "Unable to compute deformation for 'deformation compensation' feature. "
                                    "Glitter may stretch");

    const auto errorScatterTagMissing = sLogEventRegistry.createEvent(scene_rdl2::logging::ERROR_LEVEL,
                                        "scatter_tag not found in geometry. "
                                        "Hair glint variation cannot be applied. "
                                        "Please ensure that either scatter_tag is being output from the geometry, "
                                        "or remove hair glint variation by setting min twists and max twists to the "
                                        "same number");

    using namespace moonray::util;

    MOONRAY_START_THREADSAFE_STATIC_WRITE
    atomicStore(&sEventMessages.sErrorNoRefN, errorNoRefN);
    atomicStore(&sEventMessages.sWarnNoRefPpartials, warnNoRefPpartials);
    atomicStore(&sEventMessages.sErrorScatterTagMissing, errorScatterTagMissing);
   MOONRAY_FINISH_THREADSAFE_STATIC_WRITE
}

// Called from Leaf-level materials.
// material must derive from DwaBaseLayerable
void
DwaBaseLayerable::createLobes(const DwaBaseLayerable * const dwaBaseLayerable,
                              moonray::shading::TLState *tls,
                              const moonray::shading::State &state,
                              moonray::shading::BsdfBuilder &builder,
                              const ispc::DwaBaseParameters &params,
                              const ispc::DwaBaseUniformParameters &uParams,
                              const ispc::DwaBaseLabels &labels) const
{
    // print out the parameters for debugging
    // std::cout << uParams << params << "\n";

    const ispc::DwaBaseEventMessages& eventMessages = *(dwaBaseLayerable->mIspc.mEventMessagesPtr);

    // Emission
    builder.addEmission(scene_rdl2::math::asCpp(params.mEmission));

    scene_rdl2::alloc::Arena *arena = getArena(tls);

    const scene_rdl2::math::Vec3f& N = scene_rdl2::math::asCpp(params.mNormal);

    if (uParams.mThinGeometry) {
        builder.setThinGeo();
    }

    if (uParams.mPreventLightCulling) {
        builder.setPreventLightCulling(true);
    }

    const scene_rdl2::rdl2::LightSet* diffuseLightSet = reinterpret_cast<const scene_rdl2::rdl2::LightSet*>(params.mDiffuseLightSet);
    const scene_rdl2::rdl2::LightSet* specularLightSet = reinterpret_cast<const scene_rdl2::rdl2::LightSet*>(params.mSpecularLightSet);

    // Apply roughness clamping
    // FIXME: We should handle roughness clamping automatically
    // in the BsdfBuilder, rather than relying on the shader
    // writer to handle it explicitly
    // Get minimum roughness used to apply roughness clamping.
    const scene_rdl2::math::Vec2f& tmp = state.getMinRoughness();
    const float minRoughness = scene_rdl2::math::min(tmp.x, tmp.y);

    // Fuzz
    if (!scene_rdl2::math::isZero(params.mFuzz) && state.isEntering() == true &&
            !scene_rdl2::math::isZero(params.mFuzzRoughness)) {

        // This mapping for roughness is for backwards compatibility
        // can be removed soon
        const moonray::shading::VelvetBRDF fuzz(
                scene_rdl2::math::asCpp(params.mFuzzNormal),
                scene_rdl2::math::clamp(0.05f + params.mFuzzRoughness, 0.0f, 1.0f),
                scene_rdl2::math::asCpp(params.mFuzzAlbedo),
                params.mFuzzUseAbsorbingFibers);

        builder.addVelvetBRDF(fuzz,
                              params.mFuzz,
                              ispc::BSDFBUILDER_PHYSICAL,
                              labels.mFuzz,
                              diffuseLightSet);
    }

    // setup iridescence if needed
    const moonray::shading::Iridescence * iridescence = nullptr;
    const ispc::IridescenceParameters &iridescenceParams = params.mIridescenceParameters;
    if (!scene_rdl2::math::isZero(iridescenceParams.mIridescence)) {
        // Cast to CPP Type
        if (iridescenceParams.mIridescenceColorControl == ispc::SHADING_IRIDESCENCE_COLOR_USE_HUE_INTERPOLATION) {
            iridescence = arena->allocWithArgs<moonray::shading::Iridescence>(
                    N,
                    iridescenceParams.mIridescence,
                    scene_rdl2::math::asCpp(iridescenceParams.mIridescencePrimaryColor),
                    scene_rdl2::math::asCpp(iridescenceParams.mIridescenceSecondaryColor),
                    iridescenceParams.mIridescenceFlipHueDirection,
                    iridescenceParams.mIridescenceThickness,
                    iridescenceParams.mIridescenceExponent,
                    iridescenceParams.mIridescenceAt0,
                    iridescenceParams.mIridescenceAt90);
        } else { // ispc::SHADING_IRIDESCENCE_COLOR_USE_RAMP
            const scene_rdl2::math::Color* rampColors =
                reinterpret_cast<const scene_rdl2::math::Color*>(iridescenceParams.mIridescenceRampColors);

            iridescence = arena->allocWithArgs<moonray::shading::Iridescence>(
                    N,
                    iridescenceParams.mIridescence,
                    iridescenceParams.mIridescenceRampInterpolationMode,
                    iridescenceParams.mIridescenceRampNumPoints,
                    iridescenceParams.mIridescenceRampPositions,
                    iridescenceParams.mIridescenceRampInterpolators,
                    rampColors,
                    iridescenceParams.mIridescenceThickness,
                    iridescenceParams.mIridescenceExponent,
                    iridescenceParams.mIridescenceAt0,
                    iridescenceParams.mIridescenceAt90);
        }
    }

    const moonray::shading::Iridescence * outerIridescence = iridescenceParams.mIridescenceApplyTo ==
                                                           ispc::IRIDESCENCE_OUTER_SPECULAR ?
                                                           iridescence : nullptr;
    const moonray::shading::Iridescence * primaryIridescence = iridescenceParams.mIridescenceApplyTo ==
                                                             ispc::IRIDESCENCE_PRIMARY_SPECULAR ?
                                                             iridescence : nullptr;

    // Outer specular reflection/clearcoat (ignored when exiting)
    if (!scene_rdl2::math::isZero(params.mOuterSpecular) && state.isEntering() == true) {
        addOuterSpecularLobes(builder, params, uParams, labels, minRoughness, outerIridescence, specularLightSet);
    }

    // Glitter lobes
    if (!scene_rdl2::math::isZero(params.mGlitterVaryingParameters.mGlitterMask)) {
        bool exitEarly = false;
        addGlitterLobes(dwaBaseLayerable, builder, tls, state, params, labels, exitEarly, eventMessages, specularLightSet);
        if (exitEarly) return;
    }

    // Fabric and velvet lobes
    {
        builder.startAdjacentComponents();

        const float warpRoughness = scene_rdl2::math::max(params.mWarpRoughness, minRoughness);
        const float weftRoughness = scene_rdl2::math::max(params.mWeftRoughness, minRoughness);

        const scene_rdl2::math::Color& warpColor = scene_rdl2::math::asCpp(params.mWarpColor);

        if (!scene_rdl2::math::isBlack(warpColor)) {
            const moonray::shading::FabricBRDF warp(N,
                                                  scene_rdl2::math::asCpp(params.mFabricTangent),
                                                  scene_rdl2::math::asCpp(params.mWarpThreadDirection),
                                                  params.mWarpThreadElevation,
                                                  warpRoughness,
                                                  warpColor);

            builder.addFabricBRDF(warp,
                                  params.mWarpThreadCoverage * params.mFabricSpecular,
                                  ispc::BSDFBUILDER_PHYSICAL,
                                  labels.mSpecular,
                                  specularLightSet);
        }

        const scene_rdl2::math::Color& weftColor = scene_rdl2::math::asCpp(params.mWeftColor);
        if (!scene_rdl2::math::isBlack(weftColor)) {
            // Rotate the warp thread by 90 in (X,Y)
            const scene_rdl2::math::Vec3f weftThreadDirection = {-scene_rdl2::math::asCpp(params.mWarpThreadDirection).y,
                                                             scene_rdl2::math::asCpp(params.mWarpThreadDirection).x,
                                                             scene_rdl2::math::asCpp(params.mWarpThreadDirection).z};

            const moonray::shading::FabricBRDF weft(N,
                                                  scene_rdl2::math::asCpp(params.mFabricTangent),
                                                  weftThreadDirection,
                                                  0.f, // no elevation on the second (weft) thread
                                                  weftRoughness,
                                                  weftColor);

            builder.addFabricBRDF(weft,
                                  (1.f - params.mWarpThreadCoverage) * params.mFabricSpecular,
                                  ispc::BSDFBUILDER_PHYSICAL,
                                  labels.mSpecular,
                                  specularLightSet);
        }

        builder.endAdjacentComponents();
    }

    // For internal reflections from a refracted ray, if the
    // "use independent transmission refractive index" parameter
    // is true, we use the "independent transmission refractive index".
    // Not doing this results in incorrect TIR artifacts.
    float reflIor = params.mRefractiveIndex;
    if (params.mUseIndependentTransmissionRefractiveIndex && !state.isEntering()) {
        reflIor = params.mIndependentTransmissionRefractiveIndex;
    }

    // Hair toon specular 1
    if (!scene_rdl2::math::isZero(params.mHairToonS1Params.mToonSpecular)) {
        if (uParams.mHairToonS1Model == ispc::ToonSpecularModel::ToonSpecularSurface) {
            addToonSpecularLobes(builder,
                                 params.mHairToonS1Params,
                                 labels.mHairLabels.mHair,
                                 specularLightSet,
                                 primaryIridescence);
        } else if (uParams.mHairToonS1Model == ispc::ToonSpecularModel::ToonSpecularHair) {
            addHairToonSpecularLobes(builder,
                                     params.mHairToonS1Params,
                                     labels,
                                     specularLightSet,
                                     primaryIridescence);
        }
    }

    // Hair toon specular 2
    if (!scene_rdl2::math::isZero(params.mHairToonS2Params.mToonSpecular)) {
        if (uParams.mHairToonS2Model == ispc::ToonSpecularModel::ToonSpecularSurface) {
            addToonSpecularLobes(builder,
                                 params.mHairToonS2Params,
                                 labels.mHairLabels.mHair,
                                 specularLightSet,
                                 primaryIridescence);
        } else if (uParams.mHairToonS2Model == ispc::ToonSpecularModel::ToonSpecularHair) {
            addHairToonSpecularLobes(builder,
                                     params.mHairToonS2Params,
                                     labels,
                                     specularLightSet,
                                     primaryIridescence);
        }
    }

    // Hair toon specular 3
    if (!scene_rdl2::math::isZero(params.mHairToonS3Params.mToonSpecular)) {
        if (uParams.mHairToonS3Model == ispc::ToonSpecularModel::ToonSpecularSurface) {
            addToonSpecularLobes(builder,
                                 params.mHairToonS3Params,
                                 labels.mHairLabels.mHair,
                                 specularLightSet,
                                 primaryIridescence);
        } else if (uParams.mHairToonS3Model == ispc::ToonSpecularModel::ToonSpecularHair) {
            addHairToonSpecularLobes(builder,
                                     params.mHairToonS3Params,
                                     labels,
                                     specularLightSet,
                                     primaryIridescence);
        }
    }

    if (!scene_rdl2::math::isZero(params.mSpecular) || !scene_rdl2::math::isZero(params.mTransmission)) {
        // Specular reflection and transmission

        // Modulate roughness based on normal map mip-map anti-aliasing strategy.
        const float roughness = computeMicrofacetRoughness(
                                    params.mRoughness,
                                    static_cast<ispc::NormalAAStrategyType>(params.mNormalAAStrategy),
                                    params.mNormalLength,
                                    params.mNormalAADial,
                                    params.mNormalDial);
        const float roughnessClamped = scene_rdl2::math::max(roughness, minRoughness);

        // setup shading tangent if needed
        scene_rdl2::math::Vec3f shadingTangent(0.f);
        if (!scene_rdl2::math::isZero(params.mAnisotropy)) {
            const scene_rdl2::math::Vec3f T = scene_rdl2::math::normalize(state.getdPds());
            const scene_rdl2::math::ReferenceFrame frame(N, T);
            const scene_rdl2::math::Vec2f& shadingTangentLocal = scene_rdl2::math::asCpp(params.mShadingTangent);
            shadingTangent = frame.localToGlobal(
                    scene_rdl2::math::normalize(scene_rdl2::math::Vec3f(shadingTangentLocal.x,
                                                              shadingTangentLocal.y,
                                                              0.f)));
        }

        // Metallic and dielectric specular reflection lobes

        // Metallic specular reflection lobe
        const float metallicWeight = params.mSpecular * params.mMetallic;

        if (!scene_rdl2::math::isZero(metallicWeight)) {
            if (roughnessClamped < scene_rdl2::math::sEpsilon) {
                const moonray::shading::MirrorBRDF conductorBRDF(
                        N,
                        scene_rdl2::math::asCpp(params.mMetallicColor),
                        scene_rdl2::math::asCpp(params.mMetallicEdgeColor),
                        primaryIridescence);

                builder.addMirrorBRDF(conductorBRDF,
                                      metallicWeight,
                                      ispc::BSDFBUILDER_PHYSICAL,
                                      labels.mSpecular,
                                      specularLightSet);

            } else {
                if (scene_rdl2::math::isZero(params.mAnisotropy)) {
                    const moonray::shading::MicrofacetIsotropicBRDF conductorBRDF(
                            N,
                            scene_rdl2::math::asCpp(params.mMetallicColor),
                            scene_rdl2::math::asCpp(params.mMetallicEdgeColor),
                            roughnessClamped,
                            static_cast<ispc::MicrofacetDistribution>(uParams.mSpecularModel),
                            ispc::MICROFACET_GEOMETRIC_SMITH,
                            primaryIridescence);

                    builder.addMicrofacetIsotropicBRDF(conductorBRDF,
                                                       metallicWeight,
                                                       ispc::BSDFBUILDER_PHYSICAL,
                                                       labels.mSpecular,
                                                       specularLightSet);
                } else {
                    const scene_rdl2::math::Vec2f rgh = computeAnisoRoughness(roughness,
                                                                         minRoughness,
                                                                         params.mAnisotropy);

                    const moonray::shading::MicrofacetAnisotropicBRDF conductorBRDF(
                            N,
                            scene_rdl2::math::asCpp(params.mMetallicColor),
                            scene_rdl2::math::asCpp(params.mMetallicEdgeColor),
                            rgh.x, rgh.y,
                            shadingTangent,
                            static_cast<ispc::MicrofacetDistribution>(uParams.mSpecularModel),
                            ispc::MICROFACET_GEOMETRIC_SMITH,
                            primaryIridescence);

                    builder.addMicrofacetAnisotropicBRDF(conductorBRDF,
                                                         metallicWeight,
                                                         ispc::BSDFBUILDER_PHYSICAL,
                                                         labels.mSpecular,
                                                         specularLightSet);
                }
            }
        }

        // if this material is fully metallic we can exit early
        if (scene_rdl2::math::isOne(metallicWeight)) {
            return;
        }

        // Coupled dielectric specular reflection and transmission lobes
        float refrIor = reflIor;

        if (params.mUseIndependentTransmissionRefractiveIndex) {
            refrIor = params.mIndependentTransmissionRefractiveIndex;
        }

        // Dispersion
        // Only allow for primary ray intersections to safeguard against splitting a ray twice.
        // Ideally, we tag the ray with a wavelength it carries or whether it has been split already.
        const float abbeNumber = (!state.isDirect()) ? 0.0f : params.mDispersionAbbeNumber;

        builder.startAdjacentComponents();

        // Toon Specular
        if (!scene_rdl2::math::isZero(params.mToonSpecularParams.mToonSpecular)) {
            const ispc::ToonSpecularParameters& toonParams = params.mToonSpecularParams;

            addToonSpecularLobes(builder,
                                 toonParams,
                                 labels.mSpecular,
                                 specularLightSet,
                                 primaryIridescence);

            // Toon Specular Transmission
            if (!scene_rdl2::math::isZero(params.mTransmission)) { 
                if (roughnessClamped < scene_rdl2::math::sEpsilon) {
                    const moonray::shading::MirrorBTDF mirrorBTDF(
                            N,
                            refrIor,
                            scene_rdl2::math::asCpp(params.mTransmissionColor),
                            abbeNumber);
                    builder.addMirrorBTDF(mirrorBTDF,
                                          params.mTransmission,
                                          ispc::BSDFBUILDER_PHYSICAL,
                                          labels.mSpecularTransmission,
                                          specularLightSet);
                } else {
                    const moonray::shading::MicrofacetIsotropicBTDF dielectricBTDF(
                            N,
                            refrIor,
                            roughnessClamped,
                            static_cast<ispc::MicrofacetDistribution>(uParams.mSpecularModel),
                            ispc::MICROFACET_GEOMETRIC_SMITH,
                            scene_rdl2::math::asCpp(params.mTransmissionColor),
                            abbeNumber);
                    builder.addMicrofacetIsotropicBTDF(dielectricBTDF,
                                          params.mTransmission,
                                          ispc::BSDFBUILDER_PHYSICAL,
                                          labels.mSpecularTransmission,
                                          specularLightSet);
                }
            }
        }

        const float dielectricWeight = params.mSpecular * (1.0f - params.mToonSpecularParams.mToonSpecular);
        const float transmissionWeight = params.mTransmission * (1.0f - params.mToonSpecularParams.mToonSpecular);
        float transmissionRoughness = roughness;
        float transmissionRoughnessClamped = roughnessClamped;
        if (params.mUseIndependentTransmissionRoughness) {
            transmissionRoughness = computeMicrofacetRoughness(
                            params.mIndependentTransmissionRoughness,
                            static_cast<ispc::NormalAAStrategyType>(params.mNormalAAStrategy),
                            params.mNormalLength,
                            params.mNormalAADial,
                            params.mNormalDial);
            transmissionRoughnessClamped = scene_rdl2::math::max(transmissionRoughness, minRoughness);
        }

        if (roughnessClamped < scene_rdl2::math::sEpsilon) {
            const moonray::shading::MirrorBSDF dielectricBSDF(
                    N,
                    reflIor,
                    scene_rdl2::math::asCpp(params.mTransmissionColor),
                    abbeNumber,
                    refrIor,
                    dielectricWeight,
                    transmissionWeight,
                    primaryIridescence);

            builder.addMirrorBSDF(dielectricBSDF,
                                  1.f,
                                  ispc::BSDFBUILDER_PHYSICAL,
                                  labels.mSpecular,
                                  labels.mSpecularTransmission,
                                  specularLightSet);
        } else {
            if (scene_rdl2::math::isZero(params.mAnisotropy)) {
                const moonray::shading::MicrofacetIsotropicBSDF dielectricBSDF(
                        N,
                        reflIor,
                        roughnessClamped,
                        transmissionRoughnessClamped,
                        params.mUseIndependentTransmissionRoughness,
                        static_cast<ispc::MicrofacetDistribution>(uParams.mSpecularModel),
                        ispc::MICROFACET_GEOMETRIC_SMITH,
                        scene_rdl2::math::asCpp(params.mTransmissionColor),
                        abbeNumber,
                        refrIor,
                        dielectricWeight,
                        transmissionWeight,
                        primaryIridescence);

                builder.addMicrofacetIsotropicBSDF(dielectricBSDF,
                                                   1.f,
                                                   ispc::BSDFBUILDER_PHYSICAL,
                                                   labels.mSpecular,
                                                   labels.mSpecularTransmission,
                                                   specularLightSet);
            } else {
                const scene_rdl2::math::Vec2f rgh =
                    computeAnisoRoughness(roughness, minRoughness, params.mAnisotropy);

                scene_rdl2::math::Vec2f transRgh = rgh;
                if (params.mUseIndependentTransmissionRoughness) {
                    transRgh = computeAnisoRoughness(transmissionRoughness, minRoughness, params.mAnisotropy);
                }

                const moonray::shading::MicrofacetAnisotropicBSDF dielectricBSDF(
                        N,
                        reflIor,
                        rgh.x, rgh.y,
                        transRgh.x, transRgh.y,
                        params.mUseIndependentTransmissionRoughness,
                        shadingTangent,
                        static_cast<ispc::MicrofacetDistribution>(uParams.mSpecularModel),
                        ispc::MICROFACET_GEOMETRIC_SMITH,
                        scene_rdl2::math::asCpp(params.mTransmissionColor),
                        abbeNumber,
                        refrIor,
                        dielectricWeight,
                        transmissionWeight,
                        primaryIridescence);

                builder.addMicrofacetAnisotropicBSDF(dielectricBSDF,
                                                     1.f,
                                                     ispc::BSDFBUILDER_PHYSICAL,
                                                     labels.mSpecular,
                                                     labels.mSpecularTransmission,
                                                     specularLightSet);
            }
        }

        builder.endAdjacentComponents();
    }

    // Diffuse Lobes
    const bool zeroScatterRadius = scene_rdl2::math::isBlack(scene_rdl2::math::asCpp(params.mScatteringRadius));
    const scene_rdl2::math::Color diffuseTransmissionAttenuation = getDiffuseTransmissionAttenuation(params);
    const scene_rdl2::math::Color reflectionAlbedo = scene_rdl2::math::asCpp(params.mAlbedo) * diffuseTransmissionAttenuation;
    const bool blackAlbedo = scene_rdl2::math::isBlack(reflectionAlbedo);

    // Toon Ramp Diffuse
    if (!blackAlbedo) {
        // If there is any SSS, toon diffuse is totally ignored
        // otherwise, toon ramp is applied before other diffuse models
        const int toonModel = params.mToonDiffuseParams.mModel;
        const bool isToonRamp = (toonModel == ispc::TOON_DIFFUSE_RAMP_V1 ||
                                 toonModel == ispc::TOON_DIFFUSE_RAMP_V2);
        if (zeroScatterRadius && !scene_rdl2::math::isZero(params.mToonDiffuseParams.mToonDiffuse)
            && isToonRamp) {
            const ispc::ToonDiffuseParameters &toonDParams = params.mToonDiffuseParams;
            const scene_rdl2::math::Vec3f& toonN = scene_rdl2::math::asCpp(toonDParams.mNormal);

            // Cast to CPP Type
            const scene_rdl2::math::Color* rampColors = reinterpret_cast<const scene_rdl2::math::Color*>(toonDParams.mRampColors);
            // "ramp" (v1) preserves legacy 1/(2*pi) normalization; "ramp_v2" (v2) uses corrected 1/pi.
            const int version = (toonModel == ispc::TOON_DIFFUSE_RAMP_V1) ? 1 : 2;
            const moonray::shading::ToonBRDF toon(toonN,
                                                  reflectionAlbedo,
                                                  toonDParams.mRampInputScale,
                                                  toonDParams.mRampNumPoints,
                                                  toonDParams.mRampPositions,
                                                  toonDParams.mRampInterpolators,
                                                  rampColors,
                                                  toonDParams.mExtendRamp,
                                                  version);

            builder.addToonBRDF(toon,
                                params.mFabricAttenuation * toonDParams.mToonDiffuse * toonDParams.mRampWeight,
                                ispc::BSDFBUILDER_PHYSICAL,
                                labels.mDiffuse,
                                diffuseLightSet);
        }
    }

    // Oren-Nayar and Subsurface Scattering Diffuse
    builder.startAdjacentComponents();
    if (!blackAlbedo) {
        if (zeroScatterRadius) {
            // shading normal in most cases will be N,
            // but when blending toon materials it may be intentionally out of surface hemisphere
            // in this case light culling will be off, so we use that for all surface diffuse
            const scene_rdl2::math::Vec3f& shadingN = uParams.mPreventLightCulling ?
                                                 scene_rdl2::math::asCpp(params.mToonDiffuseParams.mNormal) :
                                                 scene_rdl2::math::asCpp(params.mDiffuseNormal);

            // this param is blended only between toons so we need to weight it by what % of the layered material is toon
            const float flatness = params.mToonDiffuseParams.mFlatness * params.mToonDiffuseParams.mToonDiffuse;

            // flat diffuse is oren-nayar with an optional NPR parameter (flatness)
            const moonray::shading::FlatDiffuseBRDF diffuseRefl(shadingN,
                                                                reflectionAlbedo,
                                                                params.mDiffuseRoughness,
                                                                params.mToonDiffuseParams.mTerminatorShift,
                                                                flatness,
                                                                params.mToonDiffuseParams.mFlatnessFalloff);

            builder.addFlatDiffuseBRDF(diffuseRefl,
                                       params.mFabricAttenuation,
                                       ispc::BSDFBUILDER_PHYSICAL,
                                       labels.mDiffuse,
                                       diffuseLightSet);
        } else {
            const ispc::SubsurfaceType bssrdfType = static_cast<ispc::SubsurfaceType>(uParams.mSubsurface);
            if (bssrdfType == ispc::SUBSURFACE_NORMALIZED_DIFFUSION) {
                const moonray::shading::NormalizedDiffusion diffuseRefl(
                        scene_rdl2::math::asCpp(params.mDiffuseNormal),
                        reflectionAlbedo,
                        scene_rdl2::math::asCpp(params.mScatteringRadius),
                        dwaBaseLayerable,
                        reinterpret_cast<const scene_rdl2::rdl2::TraceSet *>(params.mSubsurfaceTraceSet),
                        reinterpret_cast<scene_rdl2::rdl2::EvalNormalFunc>(params.mEvalSubsurfaceNormalFn));

                builder.addNormalizedDiffusion(diffuseRefl,
                                               params.mFabricAttenuation,
                                               ispc::BSDFBUILDER_PHYSICAL,
                                               labels.mDiffuse,
                                               diffuseLightSet);

            } else if (bssrdfType == ispc::SUBSURFACE_DIPOLE_DIFFUSION) {
                const moonray::shading::DipoleDiffusion diffuseRefl(
                        scene_rdl2::math::asCpp(params.mDiffuseNormal),
                        reflectionAlbedo,
                        scene_rdl2::math::asCpp(params.mScatteringRadius),
                        dwaBaseLayerable,
                        reinterpret_cast<const scene_rdl2::rdl2::TraceSet *>(params.mSubsurfaceTraceSet),
                        reinterpret_cast<scene_rdl2::rdl2::EvalNormalFunc>(params.mEvalSubsurfaceNormalFn));

                builder.addDipoleDiffusion(diffuseRefl,
                                           params.mFabricAttenuation,
                                           ispc::BSDFBUILDER_PHYSICAL,
                                           labels.mDiffuse,
                                           diffuseLightSet);

            } else if (bssrdfType == ispc::SUBSURFACE_RANDOM_WALK) {
                const moonray::shading::RandomWalkSubsurface rwSubsurface(
                        scene_rdl2::math::asCpp(params.mDiffuseNormal),
                        reflectionAlbedo,
                        scene_rdl2::math::asCpp(params.mScatteringRadius),
                        params.mCreaseAttenuation,
                        params.mSSSResolveSelfIntersections,
                        dwaBaseLayerable,
                        reinterpret_cast<const scene_rdl2::rdl2::TraceSet *>(params.mSubsurfaceTraceSet),
                        reinterpret_cast<scene_rdl2::rdl2::EvalNormalFunc>(params.mEvalSubsurfaceNormalFn)
                        );

                builder.addRandomWalkSubsurface(rwSubsurface,
                                                params.mFabricAttenuation,
                                                ispc::BSDFBUILDER_PHYSICAL,
                                                labels.mDiffuse,
                                                diffuseLightSet);
            }
        }
    }

    // Diffuse Transmission lobe
    if (!scene_rdl2::math::isBlack(scene_rdl2::math::asCpp(params.mDiffuseTransmission))) {
        const moonray::shading::LambertianBTDF diffuseTrans(
                -scene_rdl2::math::asCpp(params.mDiffuseNormal),
                scene_rdl2::math::asCpp(params.mDiffuseTransmission));

        builder.addLambertianBTDF(diffuseTrans,
                                  params.mFabricAttenuation,
                                  ispc::BSDFBUILDER_PHYSICAL,
                                  labels.mDiffuseTransmission,
                                  diffuseLightSet);
    }
    builder.endAdjacentComponents();


    // Add Hair Lobes
    addHairLobes(dwaBaseLayerable, builder, params, uParams, labels, tls, state, eventMessages, diffuseLightSet);
}

static void
clampTo0(Color& c)
{
    c.r = max(0.f, c.r);
    c.g = max(0.f, c.g);
    c.b = max(0.f, c.b);
}

static void
clampTo1(Color& c)
{
        c.r = min(1.f, c.r);
        c.g = min(1.f, c.g);
        c.b = min(1.f, c.b);
}

static void
applyColorCorrections(const float hueShift,
                      const float saturation,
                      const float gainValue,
                      const bool tmiEnabled,
                      const Color& tmi,
                      Color &c)
{
    if (!isZero(hueShift)) {
        moonray::shading::applyHueShift(hueShift, c);
    }

    if (!isOne(saturation)) {
        moonray::shading::applySaturation(saturation, c);
    }

    c = c * gainValue;

    if (tmiEnabled) {
        moonray::shading::applyTMI(tmi, c);
    }
}

void
DwaBaseLayerable::applyColorCorrectParameters(moonray::shading::TLState *tls,
                                              const moonray::shading::State &state,
                                              ispc::DwaBaseParameters &params) const
{
    // We'll apply color correction to these particular params.
    // Use pointer array to allow for processing them in a loop.
    // We'll also apply color correction to emission later....

    #define NUM_COLOR_ARRAY_ELEMS 15
    Color * colors[NUM_COLOR_ARRAY_ELEMS] = {
        asCpp(&params.mFuzzAlbedo),
        asCpp(&params.mMetallicColor),
        asCpp(&params.mMetallicEdgeColor),
        asCpp(&params.mWarpColor),
        asCpp(&params.mWeftColor),
        asCpp(&params.mTransmissionColor),
        asCpp(&params.mAlbedo),
        asCpp(&params.mDiffuseTransmission),
        asCpp(&params.mHairParameters.mHairDiffuseFrontColor),
        asCpp(&params.mHairParameters.mHairDiffuseBackColor),
        asCpp(&params.mToonSpecularParams.mTint),
        asCpp(&params.mAccentParams.mSubsurfaceColor),
        asCpp(&params.mScatteringRadius),
        asCpp(&params.mIridescenceParameters.mIridescencePrimaryColor),
        asCpp(&params.mIridescenceParameters.mIridescenceSecondaryColor)
    };

    for (size_t cc = 0; cc < params.mNumColorCorrections; ++cc) {
        if (!params.mColorCorrectParams[cc].mOn) { continue; }

        if (isZero(params.mColorCorrectParams[cc].mMix)) { continue; }


        // process refl/trans values
        for (uint8_t i = 0; i < NUM_COLOR_ARRAY_ELEMS; ++i) {
            Color& c = *colors[i];
            const Color original = c;
            applyColorCorrections(params.mColorCorrectParams[cc].mHueShift,
                                  params.mColorCorrectParams[cc].mSaturation,
                                  params.mColorCorrectParams[cc].mGain,
                                  params.mColorCorrectParams[cc].mTmiEnabled,
                                  asCpp(params.mColorCorrectParams[cc].mTmi),
                                  c);
            c = lerpOpt(original, c, params.mColorCorrectParams[cc].mMix);

            // clamp refl/trans vals to [0,1]
            clampTo0(c);
            clampTo1(c);
        }

        // handle toon ramp, if present
        if (!isZero(params.mToonDiffuseParams.mToonDiffuse) && 
            (params.mToonDiffuseParams.mModel == ispc::TOON_DIFFUSE_RAMP_V1 ||
             params.mToonDiffuseParams.mModel == ispc::TOON_DIFFUSE_RAMP_V2)) {
            const int rampPts = params.mToonDiffuseParams.mRampNumPoints;
            for (int i = 0; i < rampPts; ++i) {
                Color c = asCpp(params.mToonDiffuseParams.mRampColors[i]);
                const Color original = c;
                applyColorCorrections(params.mColorCorrectParams[cc].mHueShift,
                                      params.mColorCorrectParams[cc].mSaturation,
                                      params.mColorCorrectParams[cc].mGain,
                                      params.mColorCorrectParams[cc].mTmiEnabled,
                                      asCpp(params.mColorCorrectParams[cc].mTmi),
                                      c);
                c = lerpOpt(original, c, params.mColorCorrectParams[cc].mMix);

                // clamp refl/trans vals to [0,1]
                clampTo0(c);
                clampTo1(c);
                asCpp(params.mToonDiffuseParams.mRampColors[i]) = c;
            }
        }

        // handle iridescence ramp, if present
        if (!isZero(params.mIridescenceParameters.mIridescence) &&
            params.mIridescenceParameters.mIridescenceColorControl == ispc::SHADING_IRIDESCENCE_COLOR_USE_RAMP) {
            const int rampPts = params.mIridescenceParameters.mIridescenceRampNumPoints;
            for (uint8_t i = 0; i < rampPts; ++i) {
                Color c = asCpp(params.mIridescenceParameters.mIridescenceRampColors[i]);
                const Color original = c;
                applyColorCorrections(params.mColorCorrectParams[cc].mHueShift,
                                      params.mColorCorrectParams[cc].mSaturation,
                                      params.mColorCorrectParams[cc].mGain,
                                      params.mColorCorrectParams[cc].mTmiEnabled,
                                      asCpp(params.mColorCorrectParams[cc].mTmi),
                                      c);
                c = lerpOpt(original, c, params.mColorCorrectParams[cc].mMix);

                // clamp refl/trans vals to [0,1]
                clampTo0(c);
                clampTo1(c);
                asCpp(params.mIridescenceParameters.mIridescenceRampColors[i]) = c;
            }
        }

        // handle emission separately, don't clamp at 1.0
        {
            Color& c = asCpp(params.mEmission);
            const Color original = c;
            applyColorCorrections(params.mColorCorrectParams[cc].mHueShift,
                                  params.mColorCorrectParams[cc].mSaturation,
                                  params.mColorCorrectParams[cc].mGain,
                                  params.mColorCorrectParams[cc].mTmiEnabled,
                                  asCpp(params.mColorCorrectParams[cc].mTmi),
                                  c);
            c = lerpOpt(original, c, params.mColorCorrectParams[cc].mMix);

            // clamp emission to [0,inf]
            clampTo0(c);
        }
    }
}


} // namespace dwabase
} // namespace moonshine



