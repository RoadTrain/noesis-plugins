#include "../../pluginshare.h"

typedef struct modelHdr {
	BYTE id[4];
	int  ver;
	long timestamp;

} modelHdr_t;

enum mtlFlags
{
    MTL_DIFFUSETEX      = (1 << 18),
    MTL_REFLECTIONTEX   = (1 << 19),
    MTL_IMAGEALPHA      = (1 << 24),
    MTL_OPACITYTEX_ANIM = (1 << 25),
    MTL_DIFFUSETEX_ANIM = (1 << 26),
    MTL_OPACITYTEX      = (1 << 30)
};

typedef struct mtl {
	unsigned int flags;
	float ambient[3];
	float diffuse[3];
	float emissive[3];
	float opacity;
} mtl_t;

extern mathImpFn_t *g_mfn;
extern noePluginFn_t *g_nfn;
extern int g_fmtHandle;

NPLUGIN_API bool NPAPI_Init(void);
NPLUGIN_API void NPAPI_Shutdown(void);
NPLUGIN_API int NPAPI_GetPluginVer(void);