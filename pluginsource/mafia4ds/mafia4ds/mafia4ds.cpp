#include "stdafx.h"
#include "mafia4ds.h"

const char *g_pPluginName = "mafia4ds";
const char *g_pPluginDesc = "Mafia 4DS format handler, by RoadTrain.";
int g_fmtHandle = -1;

//check whether it's 4ds
bool Model_TypeCheck(BYTE *fileBuffer, int bufferLen, noeRAPI_t *rapi)
{
	if (bufferLen < sizeof(modelHdr_t))
	{
		return false;
	}
	modelHdr_t *hdr = (modelHdr_t *)fileBuffer;
	if (memcmp(hdr->id, "4DS", 4))
	{
		return false;
	}
	if (hdr->ver != 29)
	{
		return false;
	}

	return true;
}

void Model_ReadMaterials(BYTE *fileBuffer, noeRAPI_t *rapi) {
	int offset = sizeof(modelHdr_t);
	int matNum = (int)(fileBuffer+offset);
	offset += 4;

	for (int i = 0; i < matNum; i++) {
		mtl_t *mtl = (mtl_t *)(matNum+offset);
		offset += sizeof(mtl_t);

		if (mtl->flags & MTL_REFLECTIONTEX) {
			float *reflection = (float *)(fileBuffer+offset);
			offset += sizeof(float);
			BYTE reflectionTextureNameLength = (BYTE)(fileBuffer+offset);
			offset++;

			char *reflectionTextureName = new char[reflectionTextureNameLength];
			reflectionTextureName = (char *)(fileBuffer+offset);
			rapi->LogOutput(reflectionTextureName);
			return;
		}
	}
}

noesisModel_t *Model_LoadModel(BYTE *fileBuffer, int bufferLen, int &numMdl, noeRAPI_t *rapi)
{
	Model_ReadMaterials(fileBuffer, rapi);
	void *pgctx = rapi->rpgCreateContext();
	rapi->rpgEnd();
	noesisModel_t *mdl = rapi->rpgConstructModel();
	rapi->rpgDestroyContext(pgctx);
	return mdl;
}

//called by Noesis to init the plugin
bool NPAPI_InitLocal(void)
{
	g_fmtHandle = g_nfn->NPAPI_Register("Mafia 4DS", ".4ds");
	if (g_fmtHandle < 0)
	{
		return false;
	}

	//set the data handlers for this format
	g_nfn->NPAPI_SetTypeHandler_TypeCheck(g_fmtHandle, Model_TypeCheck);
	g_nfn->NPAPI_SetTypeHandler_LoadModel(g_fmtHandle, Model_LoadModel);
	//export functions
	//g_nfn->NPAPI_SetTypeHandler_WriteModel(g_fmtHandle, Model_MD2_Write);
	//g_nfn->NPAPI_SetTypeHandler_WriteAnim(g_fmtHandle, Model_MD2_WriteAnim);

	return true;
}

//called by Noesis before the plugin is freed
void NPAPI_ShutdownLocal(void)
{
	//nothing to do in this plugin
}