// ─────────────────────────────────────────────────────────────────────────────
// tests/render/test_normal_smooth_promote.cpp
//
// Normal smoothing, polygon promotion: with the option on, flat-shaded
// triangles and quads switch to the matching Gouraud type and light from the
// per-vertex values instead of the face colour.  With it off the emitted
// primitive must be byte-for-byte the original flat call.
// ─────────────────────────────────────────────────────────────────────────────
#include "primitive_harness.h"

#include <cstdio>
#include <cstring>

extern S32 Triangle_Texture_Flat(U32 typePoly, void *poly);
extern "C" void NormalSmooth_SetEnabled(S32 enabled);
extern U32 *PtrTextures;
extern "C" U8 *ObjPtrMap;

#pragma pack(push, 1)
struct TEST_POLY3_TEXTURE {
    U16 P1, P2, P3, HandleText, Couleur, Normale;
    U16 U1, V1, U2, V2, U3, V3;
};
#pragma pack(pop)

static void set_flat_light(int index, U16 value) { ListLights[index] = value; }

static void test_flat_triangle_promotion(void) {
    TypeProj = TYPE_ISO;
    Obj_ListProjectedPoints[0].X = 10;
    Obj_ListProjectedPoints[0].Y = 20;
    Obj_ListProjectedPoints[1].X = 30;
    Obj_ListProjectedPoints[1].Y = 40;
    Obj_ListProjectedPoints[2].X = 50;
    Obj_ListProjectedPoints[2].Y = 60;

    set_flat_light(0, 0x0100);
    set_flat_light(1, 0x0200);
    set_flat_light(2, 0x0300);

    STRUC_POLY3_LIGHT poly;
    memset(&poly, 0, sizeof(poly));
    poly.P1 = 0;
    poly.P2 = 1;
    poly.P3 = 2;
    poly.Couleur = 0x20;
    poly.Normale = 0;

    NormalSmooth_SetEnabled(1);
    spy_reset();
    Triangle_Flat(0, &poly);
    check("promoted call emitted", g_spy.poly_calls, 1);
    check("flat -> gouraud type", g_spy.poly_type, 4);
    check("flat colour kept", g_spy.poly_color, 0x20);
    check("vertex count", g_spy.poly_nb, 3);
    check("v0 light", (S32)ListFillPoly[0].Pt_Light, 0x0100);
    check("v1 light", (S32)ListFillPoly[1].Pt_Light, 0x0200);
    check("v2 light", (S32)ListFillPoly[2].Pt_Light, 0x0300);

    NormalSmooth_SetEnabled(0);
    spy_reset();
    Triangle_Flat(0, &poly);
    check("disabled keeps flat type", g_spy.poly_type, 1);
    check("disabled keeps face shade", g_spy.poly_color, 0x21);
}

static void test_textured_flat_triangle_promotion(void) {
    TypeProj = TYPE_ISO;
    Obj_ListProjectedPoints[0].X = 10;
    Obj_ListProjectedPoints[0].Y = 20;
    Obj_ListProjectedPoints[1].X = 30;
    Obj_ListProjectedPoints[1].Y = 40;
    Obj_ListProjectedPoints[2].X = 50;
    Obj_ListProjectedPoints[2].Y = 60;

    set_flat_light(0, 0x0100);
    set_flat_light(1, 0x0200);
    set_flat_light(2, 0x0300);

    static U32 textures[1];
    static U8 page[256 * 256];
    memset(page, 0, sizeof(page));
    textures[0] = (0x3F3Fu << 16); /* RepMask */
    PtrTextures = textures;
    ObjPtrMap = page;

    TEST_POLY3_TEXTURE poly;
    memset(&poly, 0, sizeof(poly));
    poly.P1 = 0;
    poly.P2 = 1;
    poly.P3 = 2;
    poly.HandleText = 0;
    poly.Couleur = 0x20;
    poly.Normale = 0;

    NormalSmooth_SetEnabled(1);
    spy_reset();
    Triangle_Texture_Flat(9 << 2, &poly);
    check("textured promoted", g_spy.poly_calls, 1);
    check("textured flat -> gouraud", g_spy.poly_type, 10);
    check("textured promoted colour is unused", g_spy.poly_color, 0);
    check("textured v0 light", (S32)ListFillPoly[0].Pt_Light, 0x0100);

    NormalSmooth_SetEnabled(0);
    spy_reset();
    Triangle_Texture_Flat(9 << 2, &poly);
    check("textured disabled keeps flat", g_spy.poly_type, 9);
    check("textured disabled keeps face row", g_spy.poly_color, 0x01);
}

int main() {
    printf("=== normal smoothing: flat polygon promotion ===\n");
    test_flat_triangle_promotion();
    test_textured_flat_triangle_promotion();
    NormalSmooth_SetEnabled(0);
    printf("\n%s (%d failure%s)\n", g_failures ? "FAILED" : "PASSED",
           g_failures, g_failures == 1 ? "" : "s");
    return g_failures ? 1 : 0;
}
