// Copyright 2023-2024 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

/// @file SwitchColorMap.ispc

#include "attributes.isph"

#include <moonray/rendering/shading/ispc/MapApi.isph>

// ===================================================================
// DUAL VERSION
// ===================================================================

static Color
sample(const uniform Map *uniform map,
       uniform ShadingTLState * uniform tls,
       const varying State &state)
{
    varying int choice = max(0, (varying int) round(evalAttrChoice(map, tls, state)));

    // cycle through max choices allowed
    choice = choice % 64;

    switch(choice) {
        case 0:
            return evalAttrInput0(map, tls, state);
        case 1:
            return evalAttrInput1(map, tls, state);
        case 2:
            return evalAttrInput2(map, tls, state);
        case 3:
            return evalAttrInput3(map, tls, state);
        case 4:
            return evalAttrInput4(map, tls, state);
        case 5:
            return evalAttrInput5(map, tls, state);
        case 6:
            return evalAttrInput6(map, tls, state);
        case 7:
            return evalAttrInput7(map, tls, state);
        case 8:
            return evalAttrInput8(map, tls, state);
        case 9:
            return evalAttrInput9(map, tls, state);
        case 10:
            return evalAttrInput10(map, tls, state);
        case 11:
            return evalAttrInput11(map, tls, state);
        case 12:
            return evalAttrInput12(map, tls, state);
        case 13:
            return evalAttrInput13(map, tls, state);
        case 14:
            return evalAttrInput14(map, tls, state);
        case 15:
            return evalAttrInput15(map, tls, state);
        case 16:
            return evalAttrInput16(map, tls, state);
        case 17:
            return evalAttrInput17(map, tls, state);
        case 18:
            return evalAttrInput18(map, tls, state);
        case 19:
            return evalAttrInput19(map, tls, state);
        case 20:
            return evalAttrInput20(map, tls, state);
        case 21:
            return evalAttrInput21(map, tls, state);
        case 22:
            return evalAttrInput22(map, tls, state);
        case 23:
            return evalAttrInput23(map, tls, state);
        case 24:
            return evalAttrInput24(map, tls, state);
        case 25:
            return evalAttrInput25(map, tls, state);
        case 26:
            return evalAttrInput26(map, tls, state);
        case 27:
            return evalAttrInput27(map, tls, state);
        case 28:
            return evalAttrInput28(map, tls, state);
        case 29:
            return evalAttrInput29(map, tls, state);
        case 30:
            return evalAttrInput30(map, tls, state);
        case 31:
            return evalAttrInput31(map, tls, state);



        case 32:
            return evalAttrInput32(map, tls, state);
        case 33:
            return evalAttrInput33(map, tls, state);
        case 34:
            return evalAttrInput34(map, tls, state);
        case 35:
            return evalAttrInput35(map, tls, state);
        case 36:
            return evalAttrInput36(map, tls, state);
        case 37:
            return evalAttrInput37(map, tls, state);
        case 38:
            return evalAttrInput38(map, tls, state);
        case 39:
            return evalAttrInput39(map, tls, state);
        case 40:
            return evalAttrInput40(map, tls, state);
        case 41:
            return evalAttrInput41(map, tls, state);
        case 42:
            return evalAttrInput42(map, tls, state);
        case 43:
            return evalAttrInput43(map, tls, state);
        case 44:
            return evalAttrInput44(map, tls, state);
        case 45:
            return evalAttrInput45(map, tls, state);
        case 46:
            return evalAttrInput46(map, tls, state);
        case 47:
            return evalAttrInput47(map, tls, state);
        case 48:
            return evalAttrInput48(map, tls, state);
        case 49:
            return evalAttrInput49(map, tls, state);
        case 50:
            return evalAttrInput50(map, tls, state);
        case 51:
            return evalAttrInput51(map, tls, state);
        case 52:
            return evalAttrInput52(map, tls, state);
        case 53:
            return evalAttrInput53(map, tls, state);
        case 54:
            return evalAttrInput54(map, tls, state);
        case 55:
            return evalAttrInput55(map, tls, state);
        case 56:
            return evalAttrInput56(map, tls, state);
        case 57:
            return evalAttrInput57(map, tls, state);
        case 58:
            return evalAttrInput58(map, tls, state);
        case 59:
            return evalAttrInput59(map, tls, state);
        case 60:
            return evalAttrInput60(map, tls, state);
        case 61:
            return evalAttrInput61(map, tls, state);
        case 62:
            return evalAttrInput62(map, tls, state);
        case 63:
            return evalAttrInput63(map, tls, state);
        default:
            return Color_ctor(0.0f);
    }
}

DEFINE_MAP_SHADER(SwitchColorMap, sample)

