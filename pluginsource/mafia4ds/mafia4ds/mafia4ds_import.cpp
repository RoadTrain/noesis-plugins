#include "stdafx.h"
#include "mafia4ds.h"

// reads material definitions from 4ds
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

		bool hasTex = false;

		if (flags & MTL_REFLECTIONTEX) 
		{
			hasTex = true;
			float reflection = bs->ReadFloat();
			BYTE reflectionTextureNameLength = bs->ReadByte();

			if (reflectionTextureNameLength > 0) 
			{
				char reflectionTextureName[256];
				bs->ReadString(reflectionTextureName, reflectionTextureNameLength+1);
			}			
		}

		if (flags & MTL_DIFFUSETEX)
		{
			hasTex = true;
			BYTE diffuseTextureNameLength = bs->ReadByte();
				
			if (diffuseTextureNameLength > 0) 
			{
				char diffuseTextureName[256];
				bs->ReadString(diffuseTextureName, diffuseTextureNameLength+1);

				mat->name = rapi->Noesis_PooledString(diffuseTextureName);

				char texPath[280];
				sprintf_s(texPath, "../MAPS/%s", diffuseTextureName);

				noesisTex_t *tex = rapi->Noesis_LoadExternalTex(texPath);

				if (tex != NULL)
				{
					texList.Append(tex);
					mat->texIdx = texList.Num()-1;
				}			
			}
		}		

		if (flags & MTL_OPACITYTEX && !(flags & MTL_IMAGEALPHA))
		{
			hasTex = true;
			BYTE opacityTextureNameLength = bs->ReadByte();

			if (opacityTextureNameLength > 0) 
			{
				char opacityTextureName[256];
				bs->ReadString(opacityTextureName, opacityTextureNameLength+1);
			}	
		}

		if (!hasTex)
		{
			bs->ReadByte();
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

// reads a mesh object from 4ds
void Model_ReadObject(UINT16 ver, char *nodeName, RichBitStream *bs, noeRAPI_t *rapi, bool singleMesh = false)
{
	UINT16 instanceID;
	bs->ReadBytes(&instanceID, 2);

	if (instanceID == 0)
	{
		BYTE numLODs = bs->ReadByte();
					
		for (int j = 0; j < numLODs; j++)
		{
			if (j > 0)
			{
				char lodName[260];
				sprintf_s(lodName, "%s:%d", nodeName, j);
				rapi->rpgSetName(lodName);
			}

			float clippingRange = bs->ReadFloat();

			if (ver != VERSION_MAFIA)
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

		// read a SINGLEMESH extension (bones and weights)
		if (singleMesh)
		{
			if (ver == VERSION_MAFIA)
			{
				for (int i = 0; i <  numLODs; i++)
				{
					BYTE numBones = bs->ReadByte();
					bs->ReadInt(); //unknown
					RichVec3 min, max;
					bs->ReadBytes(&min, sizeof(RichVec3));
					bs->ReadBytes(&max, sizeof(RichVec3));

					for (int j = 0; j < numBones; j++)
					{
						RichMat44 inverseTransform;
						bs->ReadBytes(&inverseTransform, sizeof(RichMat44));

						UINT32 numUnk1 = bs->ReadInt();
						UINT32 numUnk2 = bs->ReadInt();
						UINT32 boneID = bs->ReadInt();

						RichVec3 bmin, bmax;
						bs->ReadBytes(&bmin, sizeof(RichVec3));
						bs->ReadBytes(&bmax, sizeof(RichVec3));

						float *data = new float[numUnk2];
						bs->ReadBytes(data, numUnk2*sizeof(float));
					}
				}
			}
			else // HD2 & Chameleon
			{
				BYTE numBones = bs->ReadByte();

				if (ver == VERSION_HD2)
				{
					RichVec4 min, max;
					bs->ReadBytes(&min, sizeof(RichVec4));
					bs->ReadBytes(&max, sizeof(RichVec4));
				}
				else
				{
					RichVec3 min, max;
					bs->ReadBytes(&min, sizeof(RichVec3));
					bs->ReadBytes(&max, sizeof(RichVec3));
				}

				BYTE *parentIDs = new BYTE[numBones];
				bs->ReadBytes(parentIDs, numBones);

				for (int i = 0; i < numBones; i++)
				{
					RichMat44 inverseTransform;
					bs->ReadBytes(&inverseTransform, sizeof(RichMat44));

					if (ver == VERSION_HD2)
					{
						RichVec4 bmin, bmax;
						bs->ReadBytes(&bmin, sizeof(RichVec4));
						bs->ReadBytes(&bmax, sizeof(RichVec4));
					}
					else
					{
						RichVec3 bmin, bmax;
						bs->ReadBytes(&bmin, sizeof(RichVec3));
						bs->ReadBytes(&bmax, sizeof(RichVec3));
					}
				}

				for (int i = 0; i < numLODs; i++)
				{
					UINT32 numWeights = bs->ReadInt();

					weight_t *weights = new weight_t[numWeights];
					bs->ReadBytes(weights, numWeights*sizeof(weight_t));
				}
			}
		}
	}
}

// reads morphs
void Model_ReadMorph(UINT16 ver, RichBitStream *bs, noeRAPI_t *rapi)
{
	BYTE numTargets = bs->ReadByte();
	BYTE numChannels = bs->ReadByte();
	BYTE unknown = bs->ReadByte();

	if (numTargets > 0)
	{
		for (int i = 0; i < numChannels; i++)
		{
			UINT16 numVerts;
			bs->ReadBytes(&numVerts, 2);

			if (numVerts > 0)
			{
				for (int j = 0; j < numTargets; j++)
				{
					morphedVert_t *verts = new morphedVert_t[numVerts];
					bs->ReadBytes(verts, numVerts*sizeof(morphedVert_t));
				}

				BYTE unk = bs->ReadByte(); //unknown
				if (unk != 0)
				{
					UINT16 *vertIndices = new UINT16[numVerts];
					bs->ReadBytes(vertIndices, numVerts*sizeof(UINT16));
				}
			}
		}

		if (ver == VERSION_HD2)
		{
			float unknown2[12];
			bs->ReadBytes(unknown2, 12*sizeof(float));
		}
		else
		{
			float unknown2[10];
			bs->ReadBytes(unknown2, 10*sizeof(float));
		}
	}
}

// reads a light sector from 4ds
void Model_ReadSector(UINT16 ver, RichBitStream *bs, noeRAPI_t *rapi)
{
	UINT32 flags[2];
	bs->ReadBytes(flags, 8);

	UINT32 numVerts = bs->ReadInt();
	UINT32 numFaces = bs->ReadInt();

	if (ver == VERSION_HD2)
	{
		RichVec4 min, max;
		bs->ReadBytes(&min, sizeof(RichVec4));
		bs->ReadBytes(&max, sizeof(RichVec4));

		RichVec4 *verts = new RichVec4[numVerts];
		bs->ReadBytes(verts, numVerts*sizeof(RichVec4));
	}
	else
	{
		if (ver != VERSION_MAFIA)
		{
			RichVec3 min, max;
			bs->ReadBytes(&min, sizeof(RichVec3));
			bs->ReadBytes(&max, sizeof(RichVec3));
		}
		RichVec3 *verts = new RichVec3[numVerts];
		bs->ReadBytes(verts, numVerts*sizeof(RichVec3));
	}

	face_t *faces = new face_t[numFaces];
	bs->ReadBytes(faces, numFaces*sizeof(face_t));

	if (ver == VERSION_MAFIA)
	{
		RichVec3 min, max;
		bs->ReadBytes(&min, sizeof(RichVec3));
		bs->ReadBytes(&max, sizeof(RichVec3));
	}

	BYTE numPortals = bs->ReadByte();

	for (int i = 0; i < numPortals; i++)
	{
		BYTE numVerts = bs->ReadByte();

		RichVec3 plane_n;
		bs->ReadBytes(&plane_n, sizeof(RichVec3));
		float plane_d = bs->ReadFloat();

		UINT32 flags = bs->ReadInt();
		float nearRange = bs->ReadFloat();
		float farRange = bs->ReadFloat();

		if (ver != VERSION_MAFIA)
			bs->ReadInt(); //unknown

		if (ver == VERSION_HD2)
		{
			RichVec4 *verts = new RichVec4[numVerts];
			bs->ReadBytes(verts, numVerts*sizeof(RichVec4));
		}
		else
		{
			RichVec3 *verts = new RichVec3[numVerts];
			bs->ReadBytes(verts, numVerts*sizeof(RichVec3));
		}
	}
}

// reads a dummy box object
void Model_ReadDummy(UINT16 ver, RichBitStream *bs, noeRAPI_t *rapi)
{
	RichVec3 min, max;
	bs->ReadBytes(&min, sizeof(RichVec3));

	if (ver == VERSION_HD2)
		bs->ReadInt();

	bs->ReadBytes(&max, sizeof(RichVec3));

	if (ver == VERSION_HD2)
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

// reads a target
void Model_ReadTarget(UINT16 ver, RichBitStream *bs, noeRAPI_t *rapi)
{
	UINT16 unk;
	bs->ReadBytes(&unk, 2);

	BYTE numLinks = bs->ReadByte();

	UINT16 *links = new UINT16[numLinks];
	bs->ReadBytes(links, numLinks*sizeof(UINT16));
}

// reads a joint
void Model_ReadJoint(UINT16 ver, RichBitStream *bs, noeRAPI_t *rapi)
{
	if (ver == VERSION_MAFIA)
	{
		RichMat44 matrix;
		bs->ReadBytes(&matrix, sizeof(RichMat44));
	}

	UINT32 ID = bs->ReadInt();
}

// reads an occluder
void Model_ReadOccluder(UINT16 ver, RichBitStream *bs, noeRAPI_t *rapi)
{
	UINT32 numVerts = bs->ReadInt();
	UINT32 numFaces = bs->ReadInt();

	if (ver == VERSION_HD2)
	{
		RichVec4 *verts = new RichVec4[numVerts];
		bs->ReadBytes(verts, numVerts*sizeof(RichVec4));
		rapi->rpgBindPositionBuffer(verts[0].v, RPGEODATA_FLOAT, sizeof(RichVec4));
	}
	else
	{
		RichVec3 *verts = new RichVec3[numVerts];
		bs->ReadBytes(verts, numVerts*sizeof(RichVec3));
		rapi->rpgBindPositionBuffer(verts[0].v, RPGEODATA_FLOAT, sizeof(RichVec3));
	}

	face_t *faces = new face_t[numFaces];
	bs->ReadBytes(faces, numVerts*sizeof(face_t));

	rapi->rpgCommitTriangles(faces, RPGEODATA_USHORT, numFaces*3, RPGEO_TRIANGLE, true);
	rapi->rpgClearBufferBinds();
}