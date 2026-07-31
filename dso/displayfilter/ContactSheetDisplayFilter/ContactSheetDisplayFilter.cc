// Copyright 2026-2027 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0


#include <moonray/rendering/displayfilter/DisplayFilter.h>
#include <moonray/rendering/displayfilter/InputBuffer.h>
#include <scene_rdl2/common/math/Math.h>
#include <scene_rdl2/common/math/MathUtil.h>
#include <scene_rdl2/scene/rdl2/rdl2.h>

#include "attributes.cc"
#include "ContactSheetDisplayFilter_ispc_stubs.h"

using namespace moonray;

RDL2_DSO_CLASS_BEGIN(ContactSheetDisplayFilter, DisplayFilter)

public:
    ContactSheetDisplayFilter(const SceneClass& sceneClass, const std::string& name);

    virtual void update() override;

private:
    virtual void getInputData(const displayfilter::InitializeData& initData,
                              displayfilter::InputData& inputData) const override;

    ispc::ContactSheetDisplayFilter mIspc;

RDL2_DSO_CLASS_END(ContactSheetDisplayFilter)

//---------------------------------------------------------------------------

ContactSheetDisplayFilter::ContactSheetDisplayFilter(
        const SceneClass& sceneClass, const std::string& name) :
    Parent(sceneClass, name)
{
    mFilterFuncv = (DisplayFilterFuncv) ispc::ContactSheetDisplayFilter_getFilterFunc();

    mIspc.mMask = false;
    mIspc.mInvertMask = false;
    mIspc.mMix = 0.f;
    mIspc.mGridSize = 1;
    mIspc.mNumInputs = 1;

}

void
ContactSheetDisplayFilter::update()
{
    SceneObjectVector inputs = get(attrInputs);
    int numInputs = inputs.size();

    mIspc.mGridSize  = (int)ceil(sqrt((float)numInputs));
    mIspc.mNumInputs = numInputs;
}

void
ContactSheetDisplayFilter::getInputData(const displayfilter::InitializeData& initData,
                                        displayfilter::InputData& inputData) const
{
    inputData.mInputs = get(attrInputs);
    for (int i = 0; i < inputData.mInputs.size(); i++)
    {
        inputData.mWindowWidths.push_back(0);
    }

    // mask
    if (get(attrMask) != nullptr) {
        inputData.mInputs.push_back(get(attrMask));
        inputData.mWindowWidths.push_back(1);
    }
}

//---------------------------------------------------------------------------
