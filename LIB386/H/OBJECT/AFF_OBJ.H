#pragma once

#include <SYSTEM/ADELINE_TYPES.H>
#include <ANIM/LIBINIT.H>
#include <3D/DATAMAT.H>

// -----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif

/* Set while the GL renderer owns the world pass.  Interior (isometric)
   bodies then need a view depth for FBO occlusion; the software path keeps
   its painter-order primitive behavior. */
extern S32 Obj_GLWorld;

// -----------------------------------------------------------------------------
#pragma pack(push, 1)
typedef struct {
    S16 Type;
    S16 Alpha;
    S16 Beta;
    S16 Gamma;
} T_GROUP_INFO;
#pragma pack(pop)

#pragma pack(push, 1)
typedef union {
    void *Ptr;
    S32 Num;
} T_PTR_NUM;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct {
    S32 X; // world coor
    S32 Y;
    S32 Z;

    S32 Alpha; // local angles
    S32 Beta;
    S32 Gamma;

    T_PTR_NUM Body;
    ; // ->.O3D
    T_PTR_NUM NextBody;
    ;               // ->.O3D
    T_PTR_NUM Anim; // ->.A3D

    void *Texture;     //
    void *NextTexture; //

    U32 LastOfsIsPtr;

    S32 LastFrame;
    PTR_U32 LastOfsFrame;
    U32 LastTimer; // last TimerRef
    U32 LastNbGroups;

    S32 NextFrame;
    PTR_U32 NextOfsFrame;
    U32 NextTimer; // TimerRef when frame reached
    U32 NextNbGroups;

    S32 LoopFrame;
    U32 LoopOfsFrame;

    U32 NbFrames;

    S32 LastAnimStepX;
    S32 LastAnimStepY;
    S32 LastAnimStepZ;

    S32 LastAnimStepAlpha;
    S32 LastAnimStepBeta;
    S32 LastAnimStepGamma;

    U32 Interpolator; // lib internal

    U32 Time; // TimerRef last modif

    U32 Status; // BitField FLAG_*

    U32 Master; // BitField ANIM_MASTER_ROT

    U32 NbGroups;

    T_GROUP_INFO CurrentFrame[30];
} T_OBJ_3D;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct {
    S16 X;
    S16 Y;
    S16 Z;
    S16 Group;
} T_OBJ_POINT;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct {
    S32 Info;
    S16 SizeHeader;
    S16 Dummy;
    S32 XMin;
    S32 XMax;
    S32 YMin;
    S32 YMax;
    S32 ZMin;
    S32 ZMax;
    S32 NbGroupes;
    S32 OffGroupes;
    S32 NbPoints;
    S32 OffPoints;
    S32 NbNormales;
    S32 OffNormales;
    S32 NbNormFaces;
    S32 OffNormFaces;
    S32 NbPolys;
    S32 OffPolys;
    S32 NbLines;
    S32 OffLines;
    S32 NbSpheres;
    S32 OffSpheres;
    S32 NbTextures;
    S32 OffTextures;
} T_BODY_HEADER;
#pragma pack(pop)

/* Normal smoothing -----------------------------------------------------------------
   A body stores one authored normal per point plus a separate list of face
   normals.  When smoothing is enabled the renderer rebuilds the point normals
   as an area-weighted average of the adjacent face normals, keeping faces that
   deviate too far from the authored point normal out of the sum so sharp
   creases are not dragged across.  Flat-shaded polygons then promote to the
   matching Gouraud type and light from those smoothed point normals. */
#define NORMAL_SMOOTH_MAX_POINTS 550
#define NORMAL_SMOOTH_MAX_FACES 550

typedef struct {
    U16 P[4];
    U8 Count; /* 3 or 4 */
    U8 Group; /* group of P[0]; smoothing never crosses groups */
} T_NORMAL_SMOOTH_FACE;

/* Rebuild `out` for the first `pointCount` points from `faces`.  Entries
   [pointCount, authoredCount) are copied from `authored` unchanged.  Returns
   0 when the input does not fit the fixed scratch limits; callers then keep
   the authored normals. */
S32 NormalSmooth_Build(const T_OBJ_POINT *points, const TYPE_VT16 *authored,
                       S32 pointCount, S32 authoredCount,
                       const T_NORMAL_SMOOTH_FACE *faces, S32 faceCount,
                       TYPE_VT16 *out);

/* Set by the GL renderer each frame; the software path leaves this off. */
void NormalSmooth_SetEnabled(S32 enabled);
S32 NormalSmooth_Enabled(void);

// -----------------------------------------------------------------------------
#define TYPE_ROTATE 0
#define TYPE_TRANSLATE 1
#define MASK_OBJECT_VERSION (0xff) //; Dans Obj_Info
#define MASK_OBJECT_ANIMATED (1 << 8)
#define MASK_OBJECT_NO_SORT (1 << 9)
#define MASK_OBJECT_TRANSPARENT (1 << 10)

// -----------------------------------------------------------------------------
extern U8 *ObjPtrMap; // Ptr on the current texture page
/* Optional render-time mask of body groups to skip (first-person head). */
extern U32 ObjHiddenGroupMask;
extern T_OBJ_POINT Obj_ListRotatedPoints[];

extern Func_TransNumPtr *TransFctBody; // Pointer to a conversion fct that converts a body number into its address

// -----------------------------------------------------------------------------
extern S32 ObjectDisplay(T_OBJ_3D *obj);
extern S32 BodyDisplay(S32 x, S32 y, S32 z, S32 alpha, S32 beta, S32 gamma,
                       void *obj);
extern S32 BodyDisplay_AlphaBeta(S32 x, S32 y, S32 z, S32 alpha, S32 beta,
                                 S32 gamma, void *obj);

// =============================================================================
#ifdef __cplusplus
}
#endif
