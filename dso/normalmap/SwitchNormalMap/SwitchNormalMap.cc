// Copyright 2023-2024 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0


#include "attributes.cc"
#include "SwitchNormalMap_ispc_stubs.h"

#include <moonray/rendering/shading/MapApi.h>

#include <cmath>

using namespace scene_rdl2::math;
using namespace moonray::shading;

//---------------------------------------------------------------------------

RDL2_DSO_CLASS_BEGIN(SwitchNormalMap, NormalMap)

public:
    SwitchNormalMap(const SceneClass& sceneClass, const std::string& name);
    ~SwitchNormalMap();
    virtual void update();

private:
    static void sampleNormal(const NormalMap* self, moonray::shading::TLState *tls,
                             const moonray::shading::State& state, Vec3f* sample);

    ispc::SwitchNormalMap mIspc;
    NormalMap* mNormalMaps[ispc::SN_MAX_INPUTS];

RDL2_DSO_CLASS_END(SwitchNormalMap)

//---------------------------------------------------------------------------

SwitchNormalMap::SwitchNormalMap(const SceneClass& sceneClass, 
                                 const std::string& name) :
                                 Parent(sceneClass, name)
{
    mSampleNormalFunc = SwitchNormalMap::sampleNormal;
    mSampleNormalFuncv = (SampleNormalFuncv) ispc::SwitchNormalMap_getSampleFunc();
    for (size_t i = 0; i < ispc::SN_MAX_INPUTS; ++i) {
        mIspc.mNormalMap[i] = 0;
        mNormalMaps[i] = nullptr;
    }
}

SwitchNormalMap::~SwitchNormalMap()
{
}

#define POPULATE_NORMALMAP_INPUT(index)                                             \
        auto nm_##index = get(attrInput##index);                                    \
        if (nm_##index != nullptr) {                                                \
            mNormalMaps[index] = nm_##index->asA<NormalMap>();                \
        }                                                                           \
        mIspc.mNormalMap[index] = (intptr_t)nm_##index;

void
SwitchNormalMap::update()
{
    POPULATE_NORMALMAP_INPUT(0)
    POPULATE_NORMALMAP_INPUT(1)
    POPULATE_NORMALMAP_INPUT(2)
    POPULATE_NORMALMAP_INPUT(3)
    POPULATE_NORMALMAP_INPUT(4)
    POPULATE_NORMALMAP_INPUT(5)
    POPULATE_NORMALMAP_INPUT(6)
    POPULATE_NORMALMAP_INPUT(7)
    POPULATE_NORMALMAP_INPUT(8)
    POPULATE_NORMALMAP_INPUT(9)
    POPULATE_NORMALMAP_INPUT(10)
    POPULATE_NORMALMAP_INPUT(11)
    POPULATE_NORMALMAP_INPUT(12)
    POPULATE_NORMALMAP_INPUT(13)
    POPULATE_NORMALMAP_INPUT(14)
    POPULATE_NORMALMAP_INPUT(15)

    POPULATE_NORMALMAP_INPUT(16)
    POPULATE_NORMALMAP_INPUT(17)
    POPULATE_NORMALMAP_INPUT(18)
    POPULATE_NORMALMAP_INPUT(19)
    POPULATE_NORMALMAP_INPUT(20)
    POPULATE_NORMALMAP_INPUT(21)
    POPULATE_NORMALMAP_INPUT(22)
    POPULATE_NORMALMAP_INPUT(23)
    POPULATE_NORMALMAP_INPUT(24)
    POPULATE_NORMALMAP_INPUT(25)
    POPULATE_NORMALMAP_INPUT(26)
    POPULATE_NORMALMAP_INPUT(27)
    POPULATE_NORMALMAP_INPUT(28)
    POPULATE_NORMALMAP_INPUT(29)
    POPULATE_NORMALMAP_INPUT(30)
    POPULATE_NORMALMAP_INPUT(31)

    POPULATE_NORMALMAP_INPUT(32)
    POPULATE_NORMALMAP_INPUT(33)
    POPULATE_NORMALMAP_INPUT(34)
    POPULATE_NORMALMAP_INPUT(35)
    POPULATE_NORMALMAP_INPUT(36)
    POPULATE_NORMALMAP_INPUT(37)
    POPULATE_NORMALMAP_INPUT(38)
    POPULATE_NORMALMAP_INPUT(39)
    POPULATE_NORMALMAP_INPUT(40)
    POPULATE_NORMALMAP_INPUT(41)
    POPULATE_NORMALMAP_INPUT(42)
    POPULATE_NORMALMAP_INPUT(43)
    POPULATE_NORMALMAP_INPUT(44)
    POPULATE_NORMALMAP_INPUT(45)
    POPULATE_NORMALMAP_INPUT(46)
    POPULATE_NORMALMAP_INPUT(47)

    POPULATE_NORMALMAP_INPUT(48)
    POPULATE_NORMALMAP_INPUT(49)
    POPULATE_NORMALMAP_INPUT(50)
    POPULATE_NORMALMAP_INPUT(51)
    POPULATE_NORMALMAP_INPUT(52)
    POPULATE_NORMALMAP_INPUT(53)
    POPULATE_NORMALMAP_INPUT(54)
    POPULATE_NORMALMAP_INPUT(55)
    POPULATE_NORMALMAP_INPUT(56)
    POPULATE_NORMALMAP_INPUT(57)
    POPULATE_NORMALMAP_INPUT(58)
    POPULATE_NORMALMAP_INPUT(59)
    POPULATE_NORMALMAP_INPUT(60)
    POPULATE_NORMALMAP_INPUT(61)
    POPULATE_NORMALMAP_INPUT(62)
    POPULATE_NORMALMAP_INPUT(63)

    for (size_t i = 0; i < ispc::SN_MAX_INPUTS; ++i) {
        NormalMap* nrm =
                reinterpret_cast<NormalMap*>(mIspc.mNormalMap[i]);
        mIspc.mSampleNormalFn[i] =
                (nrm != nullptr) ? (intptr_t) nrm->mSampleNormalFuncv : (intptr_t) nullptr;
    }
}

void
SwitchNormalMap::sampleNormal(const NormalMap* self, 
                              moonray::shading::TLState *tls,
                              const moonray::shading::State& state, 
                              Vec3f* sample)
{
    const SwitchNormalMap* me = static_cast<const SwitchNormalMap*>(self);
    int choice = std::max(0, static_cast<int>(std::round(evalFloat(me, attrChoice, tls, state))));
    // cycle through max choices allowed
    choice = choice % ispc::SN_MAX_INPUTS;

    Vec3f result = state.getN();
    if (me->mNormalMaps[choice] != nullptr) {
        me->mNormalMaps[choice]->sampleNormal(tls, state, &result);
    }

    *sample = result;
}


//---------------------------------------------------------------------------

