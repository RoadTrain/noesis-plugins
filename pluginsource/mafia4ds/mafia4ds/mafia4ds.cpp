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
	if (hdr->ver != VERSION_MAFIA && hdr->ver != VERSION_HD2 && hdr->ver != VERSION_CHAMELEON)
	{
		return false;
	}

	return true;
}

void Model_ReadMaterials(UINT16 ver, RichBitStream *bs, noeRAPI_t *rapi, CArrayList<noesisTex_t *> &texList, CArrayList<noesisMaterial_t *> &matList) 
{	
	short matNum;
	bs->ReadBytes(&matNum, 2);

	for (int i = 0; i < matNum; i++) 
	{
		UINT32 flags = bs->ReadInt();

		if (ver == VERSION_MAFIA)
		{
			mafiaMtl_t mtl;
			bs->ReadBytes(&mtl, sizeof(mafiaMtl_t));
		}
		else
		{
			chamMtl_t mtl;
			bs->ReadBytes(&mtl, sizeof(chamMtl_t));
		}

		noesisMaterial_t *mat = rapi->Noesis_GetMaterialList(1, true);

		if (flags & MTL_REFLECTIONTEX) 
		{
			float reflection = bs->ReadFloat();
			BYTE reflectionTextureNameLength = bs->ReadByte();

			if (reflectionTextureNameLength > 0) 
			{
				char *reflectionTextureName = new char[reflectionTextureNameLength];
				bs->ReadString(reflectionTextureName, reflectionTextureNameLength+1);
			}			
		}

		BYTE diffuseTextureNameLength = bs->ReadByte();
				
		if (diffuseTextureNameLength > 0) 
		{
			char *diffuseTextureName = new char[diffuseTextureNameLength];
			bs->ReadString(diffuseTextureName, diffuseTextureNameLength+1);

			mat->name = diffuseTextureName;

			char texPath[280];
			sprintf(texPath, "../MAPS/%s", diffuseTextureName);

			noesisTex_t *tex = rapi->Noesis_LoadExternalTex(texPath);

			if (tex != NULL)
			{
				texList.Append(tex);
				mat->texIdx = texList.Num()-1;
			}			
		}

		if (flags & MTL_OPACITYTEX && !(flags & MTL_IMAGEALPHA))
		{
			BYTE opacityTextureNameLength = bs->ReadByte();

			if (opacityTextureNameLength > 0) 
			{
				char *opacityTextureName = new char[opacityTextureNameLength];
				bs->ReadString(opacityTextureName, opacityTextureNameLength);
			}	
		}

		if (flags & MTL_OPACITYTEX_ANIM)
		{
			mtlAnim_t opacityTextureAnimation;
			bs->ReadBytes(&opacityTextureAnimation, sizeof(mtlAnim_t));
		}
		else if (flags & MTL_DIFFUSETEX_ANIM)
        {
			mtlAnim_t diffuseTextureAnimation;
			bs->ReadBytes(&diffuseTextureAnimation, sizeof(mtlAnim_t));
		}

		matList.Append(mat);
	}
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

	UINT16 numNodes;
	bs->ReadBytes(&numNodes, 2);

	CArrayList<RichVec3> positions;

	for (int i = 0; i < numNodes; i++)
	{
		BYTE frameType = bs->ReadByte();

		if (frameType == FRAME_VISUAL) 
		{
			BYTE visualType = bs->ReadByte();
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
			bs->ReadBytes(&rotation, sizeof(RichVec3));
			bs->ReadBytes(&scale, sizeof(RichVec3));

			if (hdr.ver == VERSION_HD2)
			{
				bs->ReadInt(); //unknown
			}
		}		

		if (parentID != 0)
		{
			RichVec3 parentPos = positions[parentID-1];
			position += parentPos;
		}

		positions.Append(position);

		RichMat43 pos = RichMat43(RichVec3(1,0,0), RichVec3(0,1,0), RichVec3(0,0,1), position);
		RichMat43 scal = RichMat43(RichVec3(scale.v[0],0,0), RichVec3(0,scale.v[1],0), RichVec3(0,0,scale.v[2]), RichVec3(0,0,0));
		RichMat43 rot = rotation.ToMat43(); //I haven't figured out yet how to properly apply rotations to a model

		RichMat43 trans = pos*scal;
		rapi->rpgSetTransform(&trans.m);

		//rapi->rpgSetTransform(&scal.m);
		//rapi->rpgSetTransform(&rot.m);
		//rapi->rpgSetTransform(&pos.m);		

		BYTE cullingFlags = bs->ReadByte(); 

		BYTE nameLength = bs->ReadByte();
		if (nameLength > 0)
		{
			char *nodeName = new char[nameLength];
			bs->ReadString(nodeName, nameLength+1);

			rapi->rpgSetName(nodeName);			
		}

		BYTE userPropertiesLength = bs->ReadByte();
		if (userPropertiesLength > 0)
		{
			char *userProperties = new char[userPropertiesLength];
			bs->ReadString(userProperties, userPropertiesLength+1);
		}

		if (frameType == FRAME_VISUAL) 
		{		
			//read obj
			UINT16 instanceID;
			bs->ReadBytes(&instanceID, 2);

			if (instanceID == 0)
			{
				BYTE numLODs = bs->ReadByte();				
					
				for (int j = 0; j < numLODs; j++)
				{
					float clippingRange = bs->ReadFloat();

					if (hdr.ver != VERSION_MAFIA)
					{
						UINT32 extraDataLength = bs->ReadInt();
					}

					UINT16 numVerts;
					bs->ReadBytes(&numVerts, 2);

					vert_t *verts = new vert_t[numVerts];
					bs->ReadBytes(verts, numVerts*sizeof(vert_t));	

					rapi->rpgBindPositionBuffer(verts[0].pos.v, RPGEODATA_FLOAT, sizeof(vert_t));
					rapi->rpgBindNormalBuffer(verts[0].norm.v, RPGEODATA_FLOAT, sizeof(vert_t));
					rapi->rpgBindUV1Buffer(verts[0].uv, RPGEODATA_FLOAT, sizeof(vert_t));
					
					BYTE numFaceGroups = bs->ReadByte();					

					for (int k = 0; k < numFaceGroups; k++) 
					{
						UINT16 numFaces;
						bs->ReadBytes(&numFaces, 2);

						face_t *faces = new face_t[numFaces];
						bs->ReadBytes(faces, numFaces*sizeof(face_t));

						UINT16 mtlID;
						bs->ReadBytes(&mtlID, 2);

						rapi->rpgSetMaterialIndex(mtlID-1);
						rapi->rpgCommitTriangles(faces, RPGEODATA_USHORT, numFaces*3, RPGEO_TRIANGLE, true);
					}
					rapi->rpgClearBufferBinds();
				}
			}
		}
		else if (frameType == FRAME_DUMMY)
		{
			RichVec3 min, max;
			bs->ReadBytes(&min, sizeof(RichVec3));

			if (hdr.ver == VERSION_HD2)
				bs->ReadInt();

			bs->ReadBytes(&max, sizeof(RichVec3));

			if (hdr.ver == VERSION_HD2)
				bs->ReadInt();

			//draw a cube
			/*float color[3] = {0.9, 0.9, 0.9};

			rapi->rpgBegin(RPGEO_QUAD);
			rapi->rpgVertColor3f(color);
			rapi->rpgVertex3f(min.v);
			rapi->rpgVertex3f(RichVec3(min.v[0], min.v[1], max.v[2]).v);
			rapi->rpgVertex3f(RichVec3(min.v[0], max.v[1], min.v[2]).v);	
			rapi->rpgVertex3f(RichVec3(min.v[0], max.v[1], max.v[2]).v);
			rapi->rpgEnd();

			rapi->rpgBegin(RPGEO_QUAD);
			rapi->rpgVertColor3f(color);
			rapi->rpgVertex3f(min.v);
			rapi->rpgVertex3f(RichVec3(min.v[0], min.v[1], max.v[2]).v);
			rapi->rpgVertex3f(RichVec3(max.v[0], min.v[1], min.v[2]).v);
			rapi->rpgVertex3f(RichVec3(max.v[0], min.v[1], max.v[2]).v);
			rapi->rpgEnd();

			rapi->rpgBegin(RPGEO_QUAD);
			rapi->rpgVertColor3f(color);
			rapi->rpgVertex3f(min.v);
			rapi->rpgVertex3f(RichVec3(max.v[0], min.v[1], min.v[2]).v);
			rapi->rpgVertex3f(RichVec3(min.v[0], max.v[1], min.v[2]).v);
			rapi->rpgVertex3f(RichVec3(max.v[0], max.v[1], min.v[2]).v);
			rapi->rpgEnd();*/
			
		}
		else if (frameType == FRAME_SECTOR)
		{
		}
	}

	if (texList.Num() > 0)
	{
		noesisMatData_t *md = rapi->Noesis_GetMatDataFromLists(matList, texList);
		rapi->rpgSetExData_Materials(md);
	}

	//rapi->rpgOptimize();
	//4DS uses a different handedness
	rapi->rpgSetOption(RPGOPT_SWAPHANDEDNESS, true);
	noesisModel_t *mdl = rapi->rpgConstructModel();
	if (mdl)
	{
		numMdl = 1; //it's important to set this on success! you can set it to > 1 if you have more than 1 contiguous model in memory
		float mdlAngOfs[3] = {0.0f, 90.0f, 270.0f};
		//rapi->SetPreviewAngOfs(mdlAngOfs);
		//rapi->SetPreviewOption("noTextureLoad", "1");
	}
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