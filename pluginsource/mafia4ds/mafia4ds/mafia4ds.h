#include "../../pluginshare.h"

#pragma pack(push, 1)

typedef struct modelHdr 
{
	BYTE id[4];
	UINT16 ver;
	UINT64 timestamp;
} modelHdr_t;

enum FileVersion 
{
	VERSION_MAFIA     = 29,
	VERSION_HD2       = 41,
	VERSION_CHAMELEON = 42
};

enum FrameType
{
    FRAME_VISUAL    = 1,
    FRAME_LIGHT     = 2,
    FRAME_CAMERA    = 3,
    FRAME_SOUND     = 4,
    FRAME_SECTOR    = 5,
    FRAME_DUMMY     = 6,
    FRAME_TARGET    = 7,
    FRAME_USER      = 8,
    FRAME_MODEL     = 9,
    FRAME_JOINT     = 10,
    FRAME_VOLUME    = 11,
    FRAME_OCCLUDER  = 12,
    FRAME_SCENE     = 13,
    FRAME_AREA      = 14,
    FRAME_LANDSCAPE = 15
};

enum VisualType
{
    VISUAL_OBJECT       = 0,
    VISUAL_LITOBJECT    = 1,
    VISUAL_SINGLEMESH   = 2,
    VISUAL_SINGLEMORPH  = 3,
    VISUAL_BILLBOARD    = 4,
    VISUAL_MORPH        = 5,
    VISUAL_LENS         = 6,
    VISUAL_PROJECTOR    = 7,
    VISUAL_MIRROR       = 8,
    VISUAL_EMITOR       = 9,
    VISUAL_SHADOW       = 10,
    VISUAL_LANDPATCH    = 11
};

enum mtlFlags
{
    MTL_DIFFUSETEX      = (1 << 18),
    MTL_REFLECTIONTEX   = (1 << 19),
    MTL_IMAGEALPHA      = (1 << 24),
    MTL_OPACITYTEX_ANIM = (1 << 25),
    MTL_DIFFUSETEX_ANIM = (1 << 26),
    MTL_OPACITYTEX      = (1 << 30)
};

typedef struct color_s
{
	float r,g,b;
} color_t;

typedef struct mafiaMtl_s
{
	color_t ambient;
	color_t diffuse;
	color_t emissive;
	float opacity;
} mafiaMtl_t;

typedef struct chamMtl_s
{
	color_t ambient;
	INT32 unk1;
	color_t diffuse;
	INT32 unk2;
	color_t specular;
	INT32 unk3;
	color_t emissive;
	INT32 unk4;
	float shininess;
	float opacity;
} chamMtl_t;

typedef struct mtlAnim_s
{
	UINT32 numFrames;
	UINT16 unknown;
	UINT32 delay;
    UINT32 unknown2;
    UINT32 unknown3;
} mtlAnim_t;

typedef struct quat_wxyz_s
{
	float w;
	float x, y, z;

	RichQuat ToQuat()
	{
		return RichQuat(x, y, z, w);
	}
} quat_wxyz_t;

typedef struct transform_s
{
	RichVec3 position;
	RichVec3 scale;
	RichQuat rotation;
} transform_t;

typedef struct vert_s 
{
	RichVec3 pos;
	RichVec3 norm;
	float uv[2];
} vert_t;

typedef struct face_s 
{
	UINT16 i[3];
} face_t;

typedef struct morphedVert_s 
{
	RichVec3 pos;
	RichVec3 norm;
} morphedVert_t;

typedef struct weight_s 
{
	BYTE boneID;
	BYTE unk;
} weight_t;

// format-unrelated structs for internal processing
typedef struct faceGroup_s
{
	UINT16 numFaces;
	UINT16 mtlID;
	face_t *faces;

	~faceGroup_s()
    {
		if (faces) delete[] faces;
    }
} faceGroup_t;

typedef struct morphChannel_s
{
	UINT16 numVerts;
	morphedVert_t **verts; // verts[vertId][targetId]
	UINT16 *vertIndices;

	~morphChannel_s()
    {
		if (verts)       delete[] verts;
		if (vertIndices) delete[] vertIndices;
    }
} morphChannel_t;

typedef struct boneData_s
{
	UINT32 numOneWeightedVerts;
	UINT32 numWeightedVerts;
	UINT16 *vertIndices1;
	UINT16 *vertIndices2;
	float  *vertWeights2;

	~boneData_s()
    {
		if (vertIndices1) delete[] vertIndices1;
		if (vertIndices2) delete[] vertIndices2;
		if (vertWeights2) delete[] vertWeights2;
    }
} boneData_t;

typedef struct lodData_s
{
	char *name;

	UINT16 numVerts;
	vert_t *verts;

	BYTE numFaceGroups;
	faceGroup_t *faceGroups;

	BYTE numBones;
	modelBone_t *bones;
	boneData_t *boneVerts;

	UINT32 *boneIdx;
	float *boneWeights;

	BYTE numTargets;
	BYTE numChannels;
	morphChannel_t *channels;

	lodData_s() : verts(NULL), faceGroups(NULL), bones(NULL), channels(NULL) {};

	~lodData_s()
    {
		if (verts)      delete[] verts;
		if (faceGroups) delete[] faceGroups;
		if (bones)      delete[] bones;
		if (channels)   delete[] channels;
    }
} lodData_t;

typedef struct objectData_s
{
	UINT16 instanceID;
	BYTE numLODs;
	lodData_t *lods;

	objectData_s() : lods(NULL) {};

	~objectData_s()
    {
		if (lods)  delete[] lods;
    }
} objectData_t;

union Float_t
{
    Float_t(float num = 0.0f) : f(num) {}
    // Portable extraction of components.
    bool Negative() const { return (i >> 31) != 0; }
    INT32 RawMantissa() const { return i & ((1 << 23) - 1); }
    INT32 RawExponent() const { return (i >> 23) & 0xFF; }
 
    INT32 i;
    float f;
#ifdef _DEBUG
    struct
    {   // Bitfields for exploration. Do not use in production code.
        UINT32 mantissa : 23;
        UINT32 exponent : 8;
        UINT32 sign : 1;
    } parts;
#endif
};
 
extern bool AlmostEqualUlps(float A, float B, int maxUlpsDiff);

extern mathImpFn_t *g_mfn;
extern noePluginFn_t *g_nfn;
extern int g_fmtHandle;

NPLUGIN_API bool NPAPI_Init(void);
NPLUGIN_API void NPAPI_Shutdown(void);
NPLUGIN_API int NPAPI_GetPluginVer(void);