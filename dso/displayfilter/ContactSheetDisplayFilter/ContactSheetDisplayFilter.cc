// Copyright 2026-2027 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0


#include <moonray/rendering/displayfilter/DisplayFilter.h>
#include <moonray/rendering/displayfilter/InputBuffer.h>
#include <scene_rdl2/common/math/Math.h>
#include <scene_rdl2/common/math/MathUtil.h>
#include <scene_rdl2/scene/rdl2/rdl2.h>

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>

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

    void renderLabels(int imgW, int imgH) const;

    mutable ispc::ContactSheetDisplayFilter mIspc;

    mutable std::shared_ptr<scene_rdl2::fb_util::VariablePixelBuffer> mTextPixelBuffer;
    mutable moonray::displayfilter::InputBuffer mTextInputBuffer;

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
    mIspc.mFontScale = 1.0f;
    mIspc.mShowLabels = true;
}


std::string getAovLabel(const SceneObject* so)
{
    std::string fullName = so->getName();
    size_t lastSlash = fullName.find_last_of('/');
    if (lastSlash != std::string::npos && lastSlash + 1 < fullName.length()) {
        return fullName.substr(lastSlash + 1);
    }

    return fullName;
}

void
ContactSheetDisplayFilter::renderLabels(int imgW, int imgH) const
{
    OIIO::ImageSpec spec(imgW, imgH, 4, OIIO::TypeDesc::FLOAT);
    OIIO::ImageBuf textBuf(spec);

    SceneObjectVector inputs = get(attrInputs);
    StringVector userLabels = get(attrLabels);

    int gridSize = mIspc.mGridSize;
    int cellW = imgW / gridSize;
    int cellH = imgH / gridSize;

    scene_rdl2::math::Color labelColor = get(attrLabelColor);
    float textColor[4] = {labelColor.r, labelColor.g, labelColor.b, 1.0f};
    float scale = get(attrFontScale);
    int fontSize = std::max(12, cellH / 18) * scale;

    if (!mTextPixelBuffer || mTextPixelBuffer->getWidth() != imgW || mTextPixelBuffer->getHeight() != imgH) {
        mTextPixelBuffer = std::make_shared<scene_rdl2::fb_util::VariablePixelBuffer>();
        mTextPixelBuffer->init(scene_rdl2::fb_util::VariablePixelBuffer::FLOAT4, imgW, imgH);
    }

    mTextPixelBuffer->getFloat4Buffer().clear(scene_rdl2::math::Vec4f(0.0f, 0.0f, 0.0f, 0.0f));

    for (int i = 0; i < mIspc.mNumInputs; ++i) {
        int row = gridSize - i / gridSize - 1;
        int col = i % gridSize;

        int labelX = col * cellW + 15;
        int labelY = (gridSize - 1 - row) * cellH + cellH + fontSize / 2;


        std::string labelText;
        if (i < userLabels.size() && !userLabels[i].empty()) {
            labelText = userLabels[i];
        }
        else {
            labelText = getAovLabel(inputs[i]);
        }

        OIIO::ImageBufAlgo::render_text(
            textBuf,
            labelX,
            labelY,
            labelText,
            fontSize,
            get(attrFontPath),
            textColor,
            OIIO::ImageBufAlgo::TextAlignX::Left,
            OIIO::ImageBufAlgo::TextAlignY::Bottom,
            0 //Drop shadow radius. Doesn't work since we are rendering onto a transparent background.
        );
    }

    textBuf = OIIO::ImageBufAlgo::flip(textBuf);

    textBuf.get_pixels(
        OIIO::ROI::All(),
        OIIO::TypeDesc::FLOAT,
        mTextPixelBuffer->getFloat4Buffer().getData()
    );

    mTextInputBuffer.init(mTextPixelBuffer);
    mTextInputBuffer.setStartPixel(0, 0);

    mIspc.mTextInputBuffer = reinterpret_cast<const ispc::InputBuffer*>(&mTextInputBuffer);
}


void
ContactSheetDisplayFilter::update()
{
    int numInputs = get(attrInputs).size();

    mIspc.mGridSize  = (int)ceil(sqrt((float)numInputs));
    mIspc.mNumInputs = numInputs;
    mIspc.mShowLabels = get(attrShowLabels);
}

void
ContactSheetDisplayFilter::getInputData(const displayfilter::InitializeData& initData,
                                        displayfilter::InputData& inputData) const
{
    inputData.mInputs = get(attrInputs);
    for (int i = 0; i < inputData.mInputs.size(); i++) {
        inputData.mWindowWidths.push_back(0);
    }

    renderLabels(initData.mImageWidth, initData.mImageHeight);

    if (get(attrMask) != nullptr) {
        inputData.mInputs.push_back(get(attrMask));
        inputData.mWindowWidths.push_back(1);
    }
}

//---------------------------------------------------------------------------
