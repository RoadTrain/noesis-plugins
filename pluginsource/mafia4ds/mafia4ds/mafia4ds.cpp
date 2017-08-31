#include "stdafx.h"
//#include "mafia4ds.h"
#include "LS3D\Model.h"

extern void Model_ReadMaterials(UINT16 ver, RichBitStream *bs, noeRAPI_t *rapi, CArrayList<noesisTex_t *> &texList, CArrayList<noesisMaterial_t *> &matList);
extern objectData_s* Model_ReadObject(UINT16 ver, char *nodeName, RichBitStream *bs, noeRAPI_t *rapi, bool singleMesh = false, bool morph = false);
extern void Model_ReadMirror(UINT16 ver, RichBitStream *bs, noeRAPI_t *rapi);
extern void Model_ReadSector(UINT16 ver, RichBitStream *bs, noeRAPI_t *rapi);
extern void Model_ReadDummy(UINT16 ver, RichBitStream *bs, noeRAPI_t *rapi);
extern void Model_ReadTarget(UINT16 ver, RichBitStream *bs, noeRAPI_t *rapi);
extern void Model_ReadJoint(UINT16 ver, RichBitStream *bs, noeRAPI_t *rapi);
extern void Model_ReadOccluder(UINT16 ver, RichBitStream *bs, noeRAPI_t *rapi);

const char *g_pPluginName = "mafia4ds";
const char *g_pPluginDesc = "Mafia/HD2/Chameleon 4DS format handler, by RoadTrain.";
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
	if (hdr->ver != VERSION_MAFIA && hdr->ver != VERSION_HD2 && hdr->ver != VERSION_CHAMELEON)
	{
		return false;
	}

	return true;
}

noesisModel_t *Model_LoadModel(BYTE *fileBuffer, int bufferLen, int &numMdl, noeRAPI_t *rapi)
{
	RichBitStream *bs = new RichBitStream(fileBuffer, bufferLen);

	modelHdr_t hdr;
	bs->ReadBytes(&hdr, sizeof(modelHdr_t));
	
	void *pgctx = rapi->rpgCreateContext();

	CArrayList<noesisTex_t *> texList;
	CArrayList<noesisMaterial_t *> matList;
	Model_ReadMaterials(hdr.ver, bs, rapi, texList, matList);

	objectData_t* object = NULL;

	UINT16 numNodes;
	bs->ReadBytes(&numNodes, 2);

	//that's a really crappy way of getting bone hierarchy
	UINT32 *boneIDs = new UINT32[numNodes];
	memset(boneIDs, 0, numNodes*sizeof(INT32));

	CArrayList<transform_t> transforms;
	CArrayList<RichMat43> ts;

	for (int i = 0; i < numNodes; i++)
	{
		BYTE frameType = bs->ReadByte();
		BYTE visualType;

		if (frameType == FRAME_VISUAL) 
		{
			visualType = bs->ReadByte();
			UINT16 visualFlags;
			bs->ReadBytes(&visualFlags, 2);
		}

		UINT16 parentID;
		bs->ReadBytes(&parentID, 2);

		RichVec3 position, scale;
		RichQuat rotation;
		bs->ReadBytes(&position, sizeof(RichVec3));

		if (hdr.ver == VERSION_MAFIA)
		{
			bs->ReadBytes(&scale, sizeof(RichVec3));

			quat_wxyz_t quat_wxyz;
			bs->ReadBytes(&quat_wxyz, sizeof(quat_wxyz_t));
			rotation = quat_wxyz.ToQuat();
		}
		else
		{
			bs->ReadBytes(&rotation, sizeof(RichQuat));
			bs->ReadBytes(&scale, sizeof(RichVec3));

			if (hdr.ver == VERSION_HD2)
			{
				bs->ReadInt(); //unknown
			}
		}

		/*if (parentID != 0)
		{
			transform_t transform = transforms[parentID-1];			
			position = position + transform.position;
			scale = scale * transform.scale;
			rotation = rotation * transform.rotation;
		}
		
		transform_t transform;
		transform.position = position;
		transform.scale = scale;
		transform.rotation = rotation;
		transforms.Append(transform);*/

		RichMat43 pos = RichMat43(RichVec3(1,0,0), RichVec3(0,1,0), RichVec3(0,0,1), position);
		RichMat43 scal = RichMat43(RichVec3(scale.v[0],0,0), RichVec3(0,scale.v[1],0), RichVec3(0,0,scale.v[2]), RichVec3(0,0,0));
		RichMat43 rot = rotation.ToMat43(); 

		RichMat43 trans = scal*rot*pos;

		if (parentID != 0)
		{
			RichMat43 parentTrans = ts[parentID-1];
			trans = trans * parentTrans;
		}

		rapi->rpgSetTransform(&trans.m);

		ts.Append(trans);

		//rapi->rpgSetTransform(&scal.m);
		//rapi->rpgSetTransform(&rot.m);
		//rapi->rpgSetTransform(&pos.m);

		BYTE cullingFlags = bs->ReadByte();

		BYTE nameLength = bs->ReadByte();
		
		char nodeName[256];
		if (nameLength > 0)
		{
			bs->ReadString(nodeName, nameLength+1);

			rapi->rpgSetName(nodeName);
		}

		BYTE userPropertiesLength = bs->ReadByte();
		if (userPropertiesLength > 0)
		{
			char userProperties[256];
			bs->ReadString(userProperties, userPropertiesLength+1);
		}

		if (frameType == FRAME_VISUAL) 
		{
			if (visualType == VISUAL_OBJECT)
			{
				Model_ReadObject(hdr.ver, nodeName, bs, rapi);
			}
			else if (visualType == VISUAL_SINGLEMESH)
			{
				object = Model_ReadObject(hdr.ver, nodeName, bs, rapi, true);

				//here we have to apply our mesh transform to every bone
				if (object->lods[0].bones) 
				{
					for (int j = 0; j < object->lods[0].numBones; j++)
					{
						RichMat43 boneMat = RichMat43(object->lods[0].bones[j].mat);
						boneMat = boneMat * trans;
						object->lods[0].bones[j].mat = boneMat.m;
					}
				}
			}
			else if (visualType == VISUAL_SINGLEMORPH)
			{
				object = Model_ReadObject(hdr.ver, nodeName, bs, rapi, true, true);

				//here we have to apply our mesh transform to every bone
				if (object->lods[0].bones) 
				{
					for (int j = 0; j < object->lods[0].numBones; j++)
					{
						RichMat43 boneMat = RichMat43(object->lods[0].bones[j].mat);
						boneMat = boneMat * trans;
						object->lods[0].bones[j].mat = boneMat.m;
					}
				}
			}
			else if (visualType == VISUAL_BILLBOARD)
			{
				Model_ReadObject(hdr.ver, nodeName, bs, rapi);
				UINT32 axis = bs->ReadInt();
				BYTE axisMode = bs->ReadByte();
			}
			else if (visualType == VISUAL_MORPH)
			{
				Model_ReadObject(hdr.ver, nodeName, bs, rapi, false, true);
			}
			else if (visualType == VISUAL_LENS)
			{
				BYTE numGlows = bs->ReadByte();

				for (int j = 0; j < numGlows; j++)
				{
					float position = bs->ReadFloat();

					if (hdr.ver != VERSION_MAFIA)
						float scale = bs->ReadFloat();

					UINT16 mtlID;
					bs->ReadBytes(&mtlID, 2);
				}
			}
			else if (visualType == VISUAL_MIRROR)
			{
				Model_ReadMirror(hdr.ver, bs, rapi);
			}
			else
			{
				rapi->LogOutput("Unknown visual object type %d", visualType);
			}
		}
		else if (frameType == FRAME_SECTOR)
		{
			Model_ReadSector(hdr.ver, bs, rapi);
		}
		else if (frameType == FRAME_DUMMY)
		{
			Model_ReadDummy(hdr.ver, bs, rapi);
		}
		else if (frameType == FRAME_TARGET)
		{
			Model_ReadTarget(hdr.ver, bs, rapi);
		}
		else if (frameType == FRAME_JOINT)
		{
			//Model_ReadJoint(hdr.ver, bs, rapi);

			RichMat44 matrix;
			if (hdr.ver == VERSION_MAFIA)
			{
				bs->ReadBytes(&matrix, sizeof(RichMat44));
			}

			UINT32 ID = bs->ReadInt();

			//boneIDs[i] = ID;

			//RichMat43 boneMat = RichMat43(object->lods[0].bones[ID].mat);
			//RichMat43 linkMat = matrix.ToMat43().GetInverse();
			//object->lods[0].bones[ID].mat = (boneMat*linkMat).m;

			//object->lods[0].bones[ID].eData.parent = (parentID > 1) ? &object->lods[0].bones[boneIDs[parentID-1]] : NULL;
		}
		else if (frameType == FRAME_OCCLUDER)
		{
			Model_ReadOccluder(hdr.ver, bs, rapi);
		}
	}

	BYTE has5ds = bs->ReadByte();

	if (matList.Num() > 0)
	{
		noesisMatData_t *md = rapi->Noesis_GetMatDataFromLists(matList, texList);
		rapi->rpgSetExData_Materials(md);
	}

	if (object)
	{
		rapi->rpgSetExData_Bones(object->lods[0].bones, object->lods[0].numBones);

		sharedPAnimParm_t aparms[2];
		memset(aparms, 0, sizeof(aparms)); //it's a good idea to do this, in case future noesis versions add more meaningful fields.
		aparms[0].angAmt = 0.0f;
		aparms[0].axis = 2; //rotate left and right
		aparms[0].boneIdx = 4; //upper torso bone
		aparms[0].timeScale = 0.1f; //acts as a framestep
		aparms[1].angAmt = -25.0f;
		aparms[1].axis = 1; //rotate back and forth
		aparms[1].boneIdx = 8; //hips
		aparms[1].timeScale = 0.1f; //acts as a framestep
		noesisAnim_t *anim = rapi->rpgCreateProceduralAnim(object->lods[0].bones, object->lods[0].numBones, aparms, 2, 500);
		if (anim)
		{
			rapi->rpgSetExData_AnimsNum(anim, 1);
		}
	}

	//4DS uses a different handedness
	rapi->rpgSetOption(RPGOPT_SWAPHANDEDNESS, true);
	noesisModel_t *mdl = rapi->rpgConstructModel();
	if (mdl)
	{
		numMdl = 1; //it's important to set this on success! you can set it to > 1 if you have more than 1 contiguous model in memory
		float mdlAngOfs[3] = {0.0f, 90.0f, 270.0f};
		//rapi->SetPreviewAngOfs(mdlAngOfs);
		rapi->SetPreviewOption("setSkelToShow", "1");
	}
	rapi->rpgDestroyContext(pgctx);
	return mdl;
}

noesisModel_t *Model_LoadModel1(BYTE *fileBuffer, int bufferLen, int &numMdl, noeRAPI_t *rapi)
{
	RichBitStream *bs = new RichBitStream(fileBuffer, bufferLen);

	LS3D::Model *model = new LS3D::Model();
	model->initFromBitStream(bs);

	void *pgctx = rapi->rpgCreateContext();
	noesisModel_t *mdl = model->constructModel(rapi);

	if (mdl)
	{
		numMdl = 1; //it's important to set this on success! you can set it to > 1 if you have more than 1 contiguous model in memory
		float mdlAngOfs[3] = {0.0f, 90.0f, 270.0f};
		//rapi->SetPreviewAngOfs(mdlAngOfs);
		rapi->SetPreviewOption("setSkelToShow", "1");
	}
	rapi->rpgDestroyContext(pgctx);
	return mdl;
}

//called by Noesis to init the plugin
bool NPAPI_InitLocal(void)
{
	g_fmtHandle = g_nfn->NPAPI_Register("Mafia/HD2/Chameleon 4DS", ".4ds");
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

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
    return TRUE;
}

bool AlmostEqualUlps(float A, float B, int maxUlpsDiff)
{
    Float_t uA(A);
    Float_t uB(B);
 
    // Different signs means they do not match.
    if (uA.Negative() != uB.Negative())
    {
        // Check for equality to make sure +0==-0
        if (A == B)
            return true;
        return false;
    }
 
    // Find the difference in ULPs.
    int ulpsDiff = abs(uA.i - uB.i);
    if (ulpsDiff <= maxUlpsDiff)
        return true;
 
    return false;
}