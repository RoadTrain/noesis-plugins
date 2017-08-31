#include "LOD.h"

////
//-- Visual Object (including SINGLEMESH/SINGLEMORPH/MORPH)
////
struct VisualObject : public Node
{
	UINT16 instanceID;

	vector<lodData_t> lods;
	BYTE numLODs;

	// read from stream
	virtual void read(RichBitStream *bs, UINT16 ver)
	{
		//RichBitStream *bs = model->bs;
		bs->ReadBytes(&instanceID, 2);

		if (instanceID != 0)
		{
			//model->nodes[ID] = model->nodes[instanceID-1];
		}
		else
		{
			numLODs = bs->ReadByte();
			// we firstly need to gather all LODs data to assign morphing and skeletal animations
			lods.reserve(numLODs);

			for (int j = 0; j < numLODs; j++)
			{
				if (j > 0)
				{
					char lodName[260];
					sprintf_s(lodName, "%s:%d", nodeName, j);
					lods[j].name = lodName;
				}

				float clippingRange = bs->ReadFloat();

				if (ver != VERSION_MAFIA)
				{
					UINT32 extraDataLength = bs->ReadInt();
				}

				UINT16 numVerts;
				bs->ReadBytes(&numVerts, 2);
				lods[j].numVerts = numVerts;

				lods[j].verts = new vert_t[numVerts];
				bs->ReadBytes(lods[j].verts, numVerts*sizeof(vert_t));

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

			if (visualType == VISUAL_SINGLEMESH || visualType == VISUAL_SINGLEMORPH)
			{
				if (ver == VERSION_MAFIA)
				{
					for (int i = 0; i <  numLODs; i++)
					{
						lodData_t *lod = &lods[i];
						BYTE numBones = bs->ReadByte();
						lods[i].numBones = numBones;

						UINT32 unk1 = bs->ReadInt(); //unknown
						RichVec3 min, max;
						bs->ReadBytes(&min, sizeof(RichVec3));
						bs->ReadBytes(&max, sizeof(RichVec3));

						//DrawBBox(min, max, rapi);

						//lods[i].bones = rapi->Noesis_AllocBones(numBones);

						for (int j = 0; j < numBones; j++)
						{
							float invMat[16];						
							bs->ReadBytes(invMat, 16*sizeof(float));
						
							RichMat44 inverseTransform = RichMat44(invMat);

							UINT32 numUnk1 = bs->ReadInt();
							UINT32 numUnk2 = bs->ReadInt();
							UINT32 boneID = bs->ReadInt();

							RichMat43 mat = inverseTransform.GetInverse().ToMat43();
							lods[i].bones[j].mat = mat.m;

							sprintf_s(lods[i].bones[j].name, "boneID%d:%d", j, i);
							lods[i].bones[j].eData.parent = (boneID > 0) ? &lods[i].bones[boneID-1] : NULL;

							RichVec3 bmin, bmax;
							bs->ReadBytes(&bmin, sizeof(RichVec3));
							bs->ReadBytes(&bmax, sizeof(RichVec3));

							bmin = mat.TransformPoint(bmin);
							bmax = mat.TransformPoint(bmax);

							vert_t *verts = lod->verts;

							int nV = 0;
							for (int v = 0; v < lod->numVerts; v++)
							{
								RichVec3 pos = verts[v].pos;
								/*Vector3d pos3d = Map<Vector3f>(verts[v].pos.v).cast<double>();

								Vector3d pos3d2 = trans * pos3d;

								if (pos3d2.x() >= bmind.x() &&
									pos3d2.y() >= bmind.y() &&
									pos3d2.z() >= bmind.z() &&
									pos3d2.x() <= bmaxd.x() &&
									pos3d2.y() <= bmaxd.y() &&
									pos3d2.z() <= bmaxd.z()
									)
									nV++;

								RichVec3 pos = inverseTransform.ToMat43().TransformPoint(verts[v].pos);
								if ((pos.v[0] > bmin.v[0] || AlmostEqualUlps(pos.v[0], bmin.v[0], 50)) &&
									(pos.v[1] > bmin.v[1] || AlmostEqualUlps(pos.v[1], bmin.v[1], 50)) &&
									(pos.v[2] > bmin.v[2] || AlmostEqualUlps(pos.v[2], bmin.v[2], 50)) &&
									(pos.v[0] < bmax.v[0] || AlmostEqualUlps(pos.v[0], bmax.v[0], 50)) &&
									(pos.v[1] < bmax.v[1] || AlmostEqualUlps(pos.v[1], bmax.v[1], 50)) &&
									(pos.v[2] < bmax.v[2] || AlmostEqualUlps(pos.v[2], bmax.v[2], 50))
									)*/
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
							wchar_t *buf = new wchar_t[100];
							//swprintf(buf, L"%d numVerts %d, numWeights %d", j, nV, numUnk1+numUnk2);
							//MessageBox(wnd, buf, L"MD2 Exporter", MB_YESNO);

							//if (numUnk1 > 0 || numUnk2 > 0)
							//	DrawBBox(bmin, bmax, rapi, lods[i].bones[j].name);

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

	// render to rapi
	virtual void render(noeRAPI_t *rapi)
	{
		for (auto lod = lods.begin(); lod != lods.end(); lod++) 
		{
			if (lod != lods.begin())
				rapi->rpgSetName(lod->name);

			rapi->rpgBindPositionBuffer(lod->verts[0].pos.v, RPGEODATA_FLOAT, sizeof(vert_t));
			rapi->rpgBindNormalBuffer(lod->verts[0].norm.v, RPGEODATA_FLOAT, sizeof(vert_t));
			rapi->rpgBindUV1Buffer(lod->verts[0].uv, RPGEODATA_FLOAT, sizeof(vert_t));

			if (lod->bones)
			{
				rapi->rpgBindBoneIndexBuffer(lod->boneIdx, RPGEODATA_UINT, sizeof(UINT32), 1);
				rapi->rpgBindBoneWeightBuffer(lod->boneWeights, RPGEODATA_FLOAT, sizeof(float), 1);
			}

			//morphing
			if (lod->numChannels > 0 && (lod == lods.begin())) // only LOD0 can have morphs
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
};