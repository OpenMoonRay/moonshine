// Copyright 2023-2024 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

/// @file SwitchFloatMap.ispc

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
    float result;
    varying int choice = max(0, (varying int) round(evalAttrChoice(map, tls, state)));

    // cycle through max choices allowed
    choice = choice % 64;

    switch(choice) {
        case 0:
            result = evalAttrInput0(map, tls, state);
            break;
        case 1:
            result = evalAttrInput1(map, tls, state);
            break;
        case 2:
            result = evalAttrInput2(map, tls, state);
            break;
        case 3:
            result = evalAttrInput3(map, tls, state);
            break;
        case 4:
            result = evalAttrInput4(map, tls, state);
            break;
        case 5:
            result = evalAttrInput5(map, tls, state);
            break;
        case 6:
            result = evalAttrInput6(map, tls, state);
            break;
        case 7:
            result = evalAttrInput7(map, tls, state);
            break;
        case 8:
            result = evalAttrInput8(map, tls, state);
            break;
        case 9:
            result = evalAttrInput9(map, tls, state);
            break;
        case 10:
            result = evalAttrInput10(map, tls, state);
            break;
        case 11:
            result = evalAttrInput11(map, tls, state);
            break;
        case 12:
            result = evalAttrInput12(map, tls, state);
            break;
        case 13:
            result = evalAttrInput13(map, tls, state);
            break;
        case 14:
            result = evalAttrInput14(map, tls, state);
            break;
        case 15:
            result = evalAttrInput15(map, tls, state);
            break;
        case 16:
            result = evalAttrInput16(map, tls, state);
            break;
        case 17:
            result = evalAttrInput17(map, tls, state);
            break;
        case 18:
            result = evalAttrInput18(map, tls, state);
            break;
        case 19:
            result = evalAttrInput19(map, tls, state);
            break;
        case 20:
            result = evalAttrInput20(map, tls, state);
            break;
        case 21:
            result = evalAttrInput21(map, tls, state);
            break;
        case 22:
            result = evalAttrInput22(map, tls, state);
            break;
        case 23:
            result = evalAttrInput23(map, tls, state);
            break;
        case 24:
            result = evalAttrInput24(map, tls, state);
            break;
        case 25:
            result = evalAttrInput25(map, tls, state);
            break;
        case 26:
            result = evalAttrInput26(map, tls, state);
            break;
        case 27:
            result = evalAttrInput27(map, tls, state);
            break;
        case 28:
            result = evalAttrInput28(map, tls, state);
            break;
        case 29:
            result = evalAttrInput29(map, tls, state);
            break;
        case 30:
            result = evalAttrInput30(map, tls, state);
            break;
        case 31:
            result = evalAttrInput31(map, tls, state);
            break;
        case 32:
            result = evalAttrInput32(map, tls, state);
            break;
        case 33:
            result = evalAttrInput33(map, tls, state);
            break;
        case 34:
            result = evalAttrInput34(map, tls, state);
            break;
        case 35:
            result = evalAttrInput35(map, tls, state);
            break;
        case 36:
            result = evalAttrInput36(map, tls, state);
            break;
        case 37:
            result = evalAttrInput37(map, tls, state);
            break;
        case 38:
            result = evalAttrInput38(map, tls, state);
            break;
        case 39:
            result = evalAttrInput39(map, tls, state);
            break;
        case 40:
            result = evalAttrInput40(map, tls, state);
            break;
        case 41:
            result = evalAttrInput41(map, tls, state);
            break;
        case 42:
            result = evalAttrInput42(map, tls, state);
            break;
        case 43:
            result = evalAttrInput43(map, tls, state);
            break;
        case 44:
            result = evalAttrInput44(map, tls, state);
            break;
        case 45:
            result = evalAttrInput45(map, tls, state);
            break;
        case 46:
            result = evalAttrInput46(map, tls, state);
            break;
        case 47:
            result = evalAttrInput47(map, tls, state);
            break;
        case 48:
            result = evalAttrInput48(map, tls, state);
            break;
        case 49:
            result = evalAttrInput49(map, tls, state);
            break;
        case 50:
            result = evalAttrInput50(map, tls, state);
            break;
        case 51:
            result = evalAttrInput51(map, tls, state);
            break;
        case 52:
            result = evalAttrInput52(map, tls, state);
            break;
        case 53:
            result = evalAttrInput53(map, tls, state);
            break;
        case 54:
            result = evalAttrInput54(map, tls, state);
            break;
        case 55:
            result = evalAttrInput55(map, tls, state);
            break;
        case 56:
            result = evalAttrInput56(map, tls, state);
            break;
        case 57:
            result = evalAttrInput57(map, tls, state);
            break;
        case 58:
            result = evalAttrInput58(map, tls, state);
            break;
        case 59:
            result = evalAttrInput59(map, tls, state);
            break;
        case 60:
            result = evalAttrInput60(map, tls, state);
            break;
        case 61:
            result = evalAttrInput61(map, tls, state);
            break;
        case 62:
            result = evalAttrInput62(map, tls, state);
            break;
        case 63:
            result = evalAttrInput63(map, tls, state);
            break;
        default:
            result = 0.0f;
    }

    return Color_ctor(result, result, result);
}

DEFINE_MAP_SHADER(SwitchFloatMap, sample)

