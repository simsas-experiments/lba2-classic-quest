// ─────────────────────────────────────────────────────────────────────────────
// tests/render/test_normal_smooth.cpp
//
// Normal smoothing: rebuild per-point normals from adjacent face normals.
//
// The body renderer stores one authored normal per point plus separate face
// normals.  NormalSmooth_Build() averages the face normals touching a point,
// excluding faces whose normal is more than 60 degrees from the authored point
// normal so sharp creases stay close to the author's normal.  These cases pin
// the averaging, the crease threshold, group isolation, degenerate faces and
// the extra authored normals that follow the point list.
// ─────────────────────────────────────────────────────────────────────────────
#include "../test_harness.h"

#include <OBJECT/AFF_OBJ.H>
#include <3D/DATAMAT.H>
#include <3D/LIGHT.H>

static T_OBJ_POINT make_point(S16 x, S16 y, S16 z, S16 group) {
    T_OBJ_POINT p;
    p.X = x;
    p.Y = y;
    p.Z = z;
    p.Group = group;
    return p;
}

static TYPE_VT16 make_normal(S16 x, S16 y, S16 z) {
    TYPE_VT16 n;
    n.X = x;
    n.Y = y;
    n.Z = z;
    n.Grp = 0;
    return n;
}

static void make_face(T_NORMAL_SMOOTH_FACE *face, U8 count, U8 group,
                      U16 p0, U16 p1, U16 p2, U16 p3) {
    face->P[0] = p0;
    face->P[1] = p1;
    face->P[2] = p2;
    face->P[3] = p3;
    face->Count = count;
    face->Group = group;
}

static void test_flat_plane_uses_geometric_normal(void) {
    T_OBJ_POINT points[4];
    points[0] = make_point(0, 0, 0, 0);
    points[1] = make_point(100, 0, 0, 0);
    points[2] = make_point(0, 100, 0, 0);
    points[3] = make_point(100, 100, 0, 0);

    TYPE_VT16 authored[4];
    memset(authored, 0, sizeof(authored));
    TYPE_VT16 out[4];

    T_NORMAL_SMOOTH_FACE faces[2];
    make_face(&faces[0], 3, 0, 0, 1, 2, 0);
    make_face(&faces[1], 3, 0, 0, 3, 2, 0);

    ASSERT_EQ_INT(1, NormalSmooth_Build(points, authored, 4, 4, faces, 2, out));
    for (int i = 0; i < 4; i++) {
        ASSERT_EQ_INT(0, out[i].X);
        ASSERT_EQ_INT(0, out[i].Y);
        ASSERT_EQ_INT(LIB_NORMAL_UNIT, out[i].Z);
    }
}

static void test_fold_averages_adjacent_faces(void) {
    /* One triangle in XY (+Z), one in YZ (+X), sharing points 0 and 2. */
    T_OBJ_POINT points[4];
    points[0] = make_point(0, 0, 0, 0);
    points[1] = make_point(100, 0, 0, 0);
    points[2] = make_point(0, 100, 0, 0);
    points[3] = make_point(0, 0, 100, 0);

    TYPE_VT16 authored[4];
    memset(authored, 0, sizeof(authored));
    TYPE_VT16 out[4];

    T_NORMAL_SMOOTH_FACE faces[2];
    make_face(&faces[0], 3, 0, 0, 1, 2, 0);
    make_face(&faces[1], 3, 0, 0, 2, 3, 0);

    ASSERT_EQ_INT(1, NormalSmooth_Build(points, authored, 4, 4, faces, 2, out));

    /* Shared points see both faces; the average is the 45-degree diagonal. */
    ASSERT_NEAR_F(10861.5f, (float)out[0].X, 2.0f);
    ASSERT_EQ_INT(0, out[0].Y);
    ASSERT_NEAR_F(10861.5f, (float)out[0].Z, 2.0f);
    ASSERT_NEAR_F(10861.5f, (float)out[2].X, 2.0f);
    ASSERT_NEAR_F(10861.5f, (float)out[2].Z, 2.0f);

    /* The unshared points keep the single face direction. */
    ASSERT_EQ_INT(0, out[1].X);
    ASSERT_EQ_INT(LIB_NORMAL_UNIT, out[1].Z);
    ASSERT_EQ_INT(LIB_NORMAL_UNIT, out[3].X);
    ASSERT_EQ_INT(0, out[3].Z);
}

static void test_authored_normal_rejects_crease(void) {
    /* Same fold, but the shared authored normals point along +Z.  The +X face
       is 90 degrees away and must not be dragged into the average. */
    T_OBJ_POINT points[4];
    points[0] = make_point(0, 0, 0, 0);
    points[1] = make_point(100, 0, 0, 0);
    points[2] = make_point(0, 100, 0, 0);
    points[3] = make_point(0, 0, 100, 0);

    TYPE_VT16 authored[4];
    authored[0] = make_normal(LIB_NORMAL_UNIT, 0, 0);
    authored[1] = make_normal(0, 0, LIB_NORMAL_UNIT);
    authored[2] = make_normal(LIB_NORMAL_UNIT, 0, 0);
    authored[3] = make_normal(LIB_NORMAL_UNIT, 0, 0);
    TYPE_VT16 out[4];

    T_NORMAL_SMOOTH_FACE faces[2];
    make_face(&faces[0], 3, 0, 0, 1, 2, 0);
    make_face(&faces[1], 3, 0, 0, 2, 3, 0);

    ASSERT_EQ_INT(1, NormalSmooth_Build(points, authored, 4, 4, faces, 2, out));

    ASSERT_EQ_INT(LIB_NORMAL_UNIT, out[0].X);
    ASSERT_EQ_INT(0, out[0].Z);
    ASSERT_EQ_INT(LIB_NORMAL_UNIT, out[2].X);
    ASSERT_EQ_INT(0, out[2].Z);
}

static void test_groups_do_not_cross(void) {
    /* Two independent flat squares in different groups.  Each point must only
       average the face in its own group. */
    T_OBJ_POINT points[6];
    points[0] = make_point(0, 0, 0, 0);
    points[1] = make_point(100, 0, 0, 0);
    points[2] = make_point(0, 100, 0, 0);
    points[3] = make_point(0, 0, 100, 1);
    points[4] = make_point(100, 0, 100, 1);
    points[5] = make_point(0, 100, 100, 1);

    TYPE_VT16 authored[6];
    memset(authored, 0, sizeof(authored));
    TYPE_VT16 out[6];

    T_NORMAL_SMOOTH_FACE faces[2];
    make_face(&faces[0], 3, 0, 0, 1, 2, 0);
    make_face(&faces[1], 3, 1, 3, 4, 5, 0);

    ASSERT_EQ_INT(1, NormalSmooth_Build(points, authored, 6, 6, faces, 2, out));
    for (int i = 0; i < 3; i++) {
        ASSERT_EQ_INT(0, out[i].X);
        ASSERT_EQ_INT(0, out[i].Y);
        ASSERT_EQ_INT(LIB_NORMAL_UNIT, out[i].Z);
    }
    for (int i = 3; i < 6; i++) {
        ASSERT_EQ_INT(0, out[i].X);
        ASSERT_EQ_INT(0, out[i].Y);
        ASSERT_EQ_INT(LIB_NORMAL_UNIT, out[i].Z);
    }
}

static void test_degenerate_face_keeps_authored(void) {
    T_OBJ_POINT points[3];
    points[0] = make_point(0, 0, 0, 0);
    points[1] = make_point(10, 0, 0, 0);
    points[2] = make_point(20, 0, 0, 0); /* collinear: zero area */

    TYPE_VT16 authored[3];
    authored[0] = make_normal(0, 0, LIB_NORMAL_UNIT);
    authored[1] = make_normal(0, 0, LIB_NORMAL_UNIT);
    authored[2] = make_normal(0, 0, LIB_NORMAL_UNIT);
    TYPE_VT16 out[3];

    T_NORMAL_SMOOTH_FACE face;
    make_face(&face, 3, 0, 0, 1, 2, 0);

    ASSERT_EQ_INT(1, NormalSmooth_Build(points, authored, 3, 3, &face, 1, out));
    for (int i = 0; i < 3; i++) {
        ASSERT_EQ_INT(0, out[i].X);
        ASSERT_EQ_INT(0, out[i].Y);
        ASSERT_EQ_INT(LIB_NORMAL_UNIT, out[i].Z);
    }
}

static void test_authored_extras_are_preserved(void) {
    T_OBJ_POINT points[3];
    points[0] = make_point(0, 0, 0, 0);
    points[1] = make_point(100, 0, 0, 0);
    points[2] = make_point(0, 100, 0, 0);

    TYPE_VT16 authored[5];
    memset(authored, 0, sizeof(authored));
    authored[3] = make_normal(1, 2, 3);
    authored[4] = make_normal(4, 5, 6);
    TYPE_VT16 out[5];

    T_NORMAL_SMOOTH_FACE face;
    make_face(&face, 3, 0, 0, 1, 2, 0);

    ASSERT_EQ_INT(1, NormalSmooth_Build(points, authored, 3, 5, &face, 1, out));
    ASSERT_EQ_INT(1, out[3].X);
    ASSERT_EQ_INT(2, out[3].Y);
    ASSERT_EQ_INT(3, out[3].Z);
    ASSERT_EQ_INT(4, out[4].X);
    ASSERT_EQ_INT(5, out[4].Y);
    ASSERT_EQ_INT(6, out[4].Z);
}

int main(void) {
    RUN_TEST(test_flat_plane_uses_geometric_normal);
    RUN_TEST(test_fold_averages_adjacent_faces);
    RUN_TEST(test_authored_normal_rejects_crease);
    RUN_TEST(test_groups_do_not_cross);
    RUN_TEST(test_degenerate_face_keeps_authored);
    RUN_TEST(test_authored_extras_are_preserved);
    TEST_SUMMARY();
    return test_failures ? 1 : 0;
}
