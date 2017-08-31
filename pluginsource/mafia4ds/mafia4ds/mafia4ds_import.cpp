#include "stdafx.h"
#include "mafia4ds.h"

// draw a bounding box
void DrawBBox(RichVec3 min, RichVec3 max, noeRAPI_t *rapi, char *name = "bbox")
{
	RichVec3 verts[] =
	{
		RichVec3(min.v[0], max.v[1], max.v[2]),
		RichVec3(min.v[0], max.v[1], min.v[2]),
		RichVec3(min.v[0], min.v[1], max.v[2]),
		min,
		max,
		RichVec3(max.v[0], max.v[1], min.v[2]),
		RichVec3(max.v[0], min.v[1], max.v[2]),
		RichVec3(max.v[0], min.v[1], min.v[2]),
	};

	UINT16 faces[] = 
	{
		0, 1, 2,    // side 1
		2, 1, 3,
		4, 0, 6,    // side 2
		6, 0, 2,
		7, 5, 6,    // side 3
		6, 5, 4,
		3, 1, 7,    // side 4
		7, 1, 5,
		4, 5, 0,    // side 5
		0, 5, 1,
		3, 7, 2,    // side 6
		2, 7, 6,
	};
	rapi->rpgSetName(name);
	rapi->rpgBindPositionBuffer(verts[0].v, RPGEODATA_FLOAT, sizeof(RichVec3));
	rapi->rpgCommitTriangles(faces, RPGEODATA_USHORT, 12*3, RPGEO_TRIANGLE, true);
	rapi->rpgClearBufferBinds();
}

// reads material definitions from 4ds
void Model_ReadMaterials(UINT16 ver, RichBitStream *bs, noeRAPI_t *rapi, CArrayList<noesisTex_t *> &texList, CArrayList<noesisMaterial_t *> &matList) 
{	
	UINT16 matNum;
	bs->ReadBytes(&matNum, 2);

	for (int i = 0; i < matNum; i++) 
	{
		UINT32 flags = bs->ReadInt();

		noesisMaterial_t *mat = rapi->Noesis_GetMaterialList(1, true);

		color_t ambient, diffuse, emissive;
		bs->ReadBytes(mat->ex->ambientColor, sizeof(color_t));

		if (ver != VERSION_MAFIA)
			bs->ReadInt(); //unknown

		bs->ReadBytes(mat->diffuse, sizeof(color_t));

		if (ver != VERSION_MAFIA)
		{
			bs->ReadInt(); //unknown
			color_t specular;
			bs->ReadBytes(&specular, sizeof(color_t));
			bs->ReadInt(); //unknown
		}

		bs->ReadBytes(&emissive, sizeof(color_t));

		if (ver != VERSION_MAFIA)
		{
			bs->ReadInt(); //unknown
			float shininess = bs->ReadFloat();
		}

		float opacity = bs->ReadFloat();

		bool hasTex = false;

		if (flags & MTL_REFLECTIONTEX) 
		{
			float reflection = bs->ReadFloat();
			mat->ex->envMapColor[3] = reflection;

			BYTE reflectionTextureNameLength = bs->ReadByte();

			if (reflectionTextureNameLength > 0) 
			{
				char reflectionTextureName[256];
				bs->ReadString(reflectionTextureName, reflectionTextureNameLength+1);

				char texPath[280];
				sprintf_s(texPath, "../MAPS/%s", reflectionTextureName);

				noesisTex_t *tex = rapi->Noesis_LoadExternalTex(texPath);
				texList.Append(tex);

				mat->envTexIdx = texList.Num() - 1;
				
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
					mat->texIdx = texList.Num() - 1;
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
objectData_t* Model_ReadObject(UINT16 ver, char *nodeName, RichBitStream *bs, noeRAPI_t *rapi, bool singleMesh = false, bool morph = false)
{
	objectData_t *obj = new objectData_t;

	UINT16 instanceID;
	bs->ReadBytes(&instanceID, 2);
	obj->instanceID = instanceID;

	if (instanceID == 0)
	{
		BYTE numLODs = bs->ReadByte();
		lodData_t *lods = new lodData_t[numLODs]; // we firstly need to gather all LODs data to assign morphing and skeletal animations
		obj->numLODs = numLODs;
		obj->lods = lods;
					
		for (int j = 0; j < numLODs; j++)
		{
			if (j > 0)
			{
				char lodName[260];
				sprintf_s(lodName, "%s:%d", nodeName, j);
				lods[j].name = lodName;
			}

			float clippingRange = bs->ReadFloat();

			UINT32 extraDataLength;
			if (ver != VERSION_MAFIA)
			{
				extraDataLength = bs->ReadInt();
			}

			UINT16 numVerts;
			bs->ReadBytes(&numVerts, 2);
			lods[j].numVerts = numVerts;

			lods[j].verts = new vert_t[numVerts];
			bs->ReadBytes(lods[j].verts, numVerts*sizeof(vert_t));

			// extra data
			if (ver != VERSION_MAFIA && extraDataLength)
			{
				float* extra = new float[numVerts*extraDataLength];
				bs->ReadBytes(extra, numVerts*extraDataLength*sizeof(float));
			}

			BYTE numFaceGroups = bs->ReadByte();
			lods[j].numFaceGroups = numFaceGroups;
			lods[j].faceGroups = new faceGroup_t[numFaceGroups];

			for (int k = 0; k < numFaceGroups; k++) 
			{
				UINT16 numFaces;
				bs->ReadBytes(&numFaces, 2);
				lods[j].faceGroups[k].numFaces = numFaces;

				lods[j].faceGroups[k].faces = new face_t[numFaces];
				bs->ReadBytes(lods[j].faceGroups[k].faces, numFaces*sizeof(face_t));

				bs->ReadBytes(&lods[j].faceGroups[k].mtlID, 2);
			}
		}

		int numWeightsPerVert = 2;

		// read a SINGLEMESH extension (bones and weights)
		if (singleMesh)
		{
			if (ver == VERSION_MAFIA)
			{
				for (int i = 0; i <  numLODs; i++)
				{
					lodData_t *lod = &lods[i];
					BYTE numBones = bs->ReadByte();
					lod->numBones = numBones;

					UINT32 numNonWeightedVerts = bs->ReadInt(); //unknown
					RichVec3 min, max;
					bs->ReadBytes(&min, sizeof(RichVec3));
					bs->ReadBytes(&max, sizeof(RichVec3));

					//DrawBBox(min, max, rapi);

					lod->bones = rapi->Noesis_AllocBones(numBones);
					lod->boneVerts = new boneData_t[numBones];

					UINT32 skipVerts = numNonWeightedVerts*numWeightsPerVert;

					UINT32 *boneIdx = new UINT32[lod->numVerts*numWeightsPerVert];
					float *boneWeights = new float[lod->numVerts*numWeightsPerVert];
					//memset(boneIdx, 0, lod->numVerts*sizeof(UINT32));
					//memset(boneWeights, 0, lod->numVerts*sizeof(float));

					for (int j = 0; j < numNonWeightedVerts; j++)
					{
						boneIdx[j*numWeightsPerVert] = 0;
						boneIdx[j*numWeightsPerVert+1] = 0;
						boneWeights[j*numWeightsPerVert] = 0.0f;
						boneWeights[j*numWeightsPerVert+1] = 0.0f;
					}

					for (int j = 0; j < numBones; j++)
					{
						float invMat[16];
						bs->ReadBytes(invMat, 16*sizeof(float));
						
						RichMat44 inverseTransform = RichMat44(invMat);

						UINT32 numOneWeightedVerts = bs->ReadInt();
						UINT32 numWeightedVerts = bs->ReadInt();
						UINT32 boneID = bs->ReadInt();

						RichMat43 mat = inverseTransform.GetInverse().ToMat43();
						lods[i].bones[j].mat = mat.m;

						//sprintf_s(lods[i].bones[j].name, "boneID%d:%d", j, i);
						lods[i].bones[j].eData.parent = (boneID > 0) ? &lods[i].bones[boneID-1] : NULL;

						RichVec3 bmin, bmax;
						bs->ReadBytes(&bmin, sizeof(RichVec3));
						bs->ReadBytes(&bmax, sizeof(RichVec3));

						bmin = mat.TransformPoint(bmin);
						bmax = mat.TransformPoint(bmax);

						/*vert_t *verts = lod->verts;

						int nV = 0;
						std::string ids = "";
						for (int v = 0; v < lod->numVerts; v++)
						{
							Vector3d pos3d = Map<Vector3f>(verts[v].pos.v).cast<double>();

							Vector3d pos3d2 = trans * pos3d;

							if (pos3d2.x() >= bmind.x() &&
								pos3d2.y() >= bmind.y() &&
								pos3d2.z() >= bmind.z() &&
								pos3d2.x() <= bmaxd.x() &&
								pos3d2.y() <= bmaxd.y() &&
								pos3d2.z() <= bmaxd.z()
								)
							{
								nV++;
								ids += std::to_string((unsigned long long)v) + ",";
							}

							RichVec3 pos = inverseTransform.ToMat43().TransformPoint(verts[v].pos);
							if ((pos.v[0] > bmin.v[0] || AlmostEqualUlps(pos.v[0], bmin.v[0], 50)) &&
								(pos.v[1] > bmin.v[1] || AlmostEqualUlps(pos.v[1], bmin.v[1], 50)) &&
								(pos.v[2] > bmin.v[2] || AlmostEqualUlps(pos.v[2], bmin.v[2], 50)) &&
								(pos.v[0] < bmax.v[0] || AlmostEqualUlps(pos.v[0], bmax.v[0], 50)) &&
								(pos.v[1] < bmax.v[1] || AlmostEqualUlps(pos.v[1], bmax.v[1], 50)) &&
								(pos.v[2] < bmax.v[2] || AlmostEqualUlps(pos.v[2], bmax.v[2], 50))
								)
							if (
								pos.v[0] > bmin.v[0] &&
								pos.v[1] > bmin.v[1] &&
								pos.v[2] > bmin.v[2] &&
								pos.v[0] < bmax.v[0] &&
								pos.v[1] < bmax.v[1] &&
								pos.v[2] < bmax.v[2]
								)
							{
								nV++;
							}
						}

						HWND wnd = g_nfn->NPAPI_GetMainWnd();
						wchar_t *buf = new wchar_t[10000];
						swprintf(buf, L"%d numVerts %d, numWeights %d, %S", j, nV, numOneWeightedVerts+numWeightedVerts, ids.c_str());
						//MessageBox(wnd, buf, L"MD2 Exporter", MB_YESNO);
						*/

						//if (boneID == 0 && (numOneWeightedVerts > 0 || numWeightedVerts > 0))
						//	DrawBBox(bmin, bmax, rapi, lods[i].bones[j].name);

						float *data = new float[numWeightedVerts];
						bs->ReadBytes(data, numWeightedVerts*sizeof(float));

						lod->boneVerts[j].numOneWeightedVerts = numOneWeightedVerts;
						lod->boneVerts[j].numWeightedVerts = numWeightedVerts;
						lod->boneVerts[j].vertIndices1 = new UINT16[numOneWeightedVerts];
						lod->boneVerts[j].vertIndices2 = new UINT16[numWeightedVerts];
						lod->boneVerts[j].vertWeights2 = new float[numWeightedVerts];

						for (int v = 0; v < numOneWeightedVerts; v++)
						{
							boneIdx[skipVerts+v*numWeightsPerVert] = j;
							boneWeights[skipVerts+v*numWeightsPerVert] = 1.0f;
							//lod->boneVerts[j].vertIndices1[v] = skipVerts + v;
							//lod->boneVerts[j].vertWeights[v] = 1.0f;
						}

						skipVerts += numOneWeightedVerts*numWeightsPerVert;

						for (int v = 0; v < numWeightedVerts; v++)
						{
							boneIdx[skipVerts+v*numWeightsPerVert] = j;
							boneWeights[skipVerts+v*numWeightsPerVert] = data[v];
							boneIdx[skipVerts+v*numWeightsPerVert+1] = boneID;
							boneWeights[skipVerts+v*numWeightsPerVert+1] = 1.0f - data[v];
							//lod->boneVerts[j].vertIndices2[v] = skipVerts + v;
							//lod->boneVerts[j].vertWeights2[v] = data[v];
						}

						skipVerts += numWeightedVerts*numWeightsPerVert;

						//skipVerts += numOneWeightedVerts + numWeightedVerts;						
					}

					lod->boneIdx = boneIdx;
					lod->boneWeights = boneWeights;
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

		// read morphed vertices
		if (morph)
		{
			lodData_t *lod = &lods[0];
			BYTE numTargets = bs->ReadByte();
			BYTE numChannels = bs->ReadByte();
			BYTE unknown = bs->ReadByte();

			lod->numTargets = numTargets;
			lod->numChannels = numChannels;

			lod->channels = new morphChannel_t[numChannels];

			if (numTargets > 0)
			{
				for (int i = 0; i < numChannels; i++)
				{
					UINT16 numVerts;
					bs->ReadBytes(&numVerts, 2);
					lod->channels[i].numVerts = numVerts;

					if (numVerts > 0)
					{
						lod->channels[i].verts = new morphedVert_t*[numVerts];

						for (int j = 0; j < numVerts; j++)
						{
							lod->channels[i].verts[j] = new morphedVert_t[numTargets];
							bs->ReadBytes(lod->channels[i].verts[j], numTargets*sizeof(morphedVert_t));
						}

						BYTE unk = bs->ReadByte(); //unknown
						if (unk != 0)
						{
							lod->channels[i].vertIndices = new UINT16[numVerts];
							bs->ReadBytes(lod->channels[i].vertIndices, numVerts*sizeof(UINT16));
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

		//constructing mesh
		for (int i = 0; i < numLODs; i++)
		{
			lodData_t *lod = &lods[i];

			if (i > 0)
				rapi->rpgSetName(lod->name);

			rapi->rpgBindPositionBuffer(lod->verts[0].pos.v, RPGEODATA_FLOAT, sizeof(vert_t));
			rapi->rpgBindNormalBuffer(lod->verts[0].norm.v, RPGEODATA_FLOAT, sizeof(vert_t));
			rapi->rpgBindUV1Buffer(lod->verts[0].uv, RPGEODATA_FLOAT, sizeof(vert_t));

			if (lod->bones)
			{
				rapi->rpgBindBoneIndexBuffer(lod->boneIdx, RPGEODATA_UINT, numWeightsPerVert*sizeof(UINT32), numWeightsPerVert);
				rapi->rpgBindBoneWeightBuffer(lod->boneWeights, RPGEODATA_FLOAT, numWeightsPerVert*sizeof(float), numWeightsPerVert);
			}

			//morphing
			if (morph && (i == 0)) // only LOD0 can have morphs
			{
				UINT16 numVerts = lod->numVerts;

				for (int h = 0; h < lod->numChannels; h++)
				{
					for (int j = 0; j < lod->numTargets; j++)
					{
						vert_t *morphedVerts = (vert_t *)rapi->Noesis_PooledAlloc(numVerts*sizeof(vert_t));
						memcpy(morphedVerts, lod->verts, numVerts*sizeof(vert_t));

						for (int k = 0; k < lod->channels[h].numVerts; k++)
						{
							UINT16 vertId = lod->channels[h].vertIndices[k];
							morphedVert_t morphedVert = lod->channels[h].verts[k][j];
							morphedVerts[vertId].pos  = morphedVert.pos;
							morphedVerts[vertId].norm = morphedVert.norm;
						}

						rapi->rpgFeedMorphTargetPositions(morphedVerts[0].pos.v, RPGEODATA_FLOAT, sizeof(vert_t));
						rapi->rpgFeedMorphTargetNormals(morphedVerts[0].norm.v, RPGEODATA_FLOAT, sizeof(vert_t));
						rapi->rpgCommitMorphFrame(numVerts);
					}
					rapi->rpgCommitMorphFrameSet();
				}
			}

			for (int j = 0; j < lod->numFaceGroups; j++)
			{
				rapi->rpgSetMaterialIndex(lod->faceGroups[j].mtlID-1);
				rapi->rpgCommitTriangles(lod->faceGroups[j].faces, RPGEODATA_USHORT, lod->faceGroups[j].numFaces*3, RPGEO_TRIANGLE, true);
			}
			rapi->rpgClearBufferBinds();
		}
	}

	return obj;
}

// reads a mirror object
void Model_ReadMirror(UINT16 ver, RichBitStream *bs, noeRAPI_t *rapi)
{
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

	float unk[4];
	bs->ReadBytes(unk, 4*sizeof(float));

	RichMat44 reflectionMatrix;
	bs->ReadBytes(&reflectionMatrix, sizeof(RichMat44));

	color_t color;
	bs->ReadBytes(&color, sizeof(color_t));

	if (ver == VERSION_HD2)
		bs->ReadInt();//unknown

	float reflectionStrength = bs->ReadFloat();
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
	bs->ReadBytes(faces, numFaces*sizeof(face_t));
	//rapi->rpgSetMaterialIndex(0);
	rapi->rpgCommitTriangles(faces, RPGEODATA_USHORT, numFaces*3, RPGEO_TRIANGLE, true);
	rapi->rpgClearBufferBinds();
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
	bs->ReadBytes(faces, numFaces*sizeof(face_t));

	rapi->rpgCommitTriangles(faces, RPGEODATA_USHORT, numFaces*3, RPGEO_TRIANGLE, true);
	rapi->rpgClearBufferBinds();
}