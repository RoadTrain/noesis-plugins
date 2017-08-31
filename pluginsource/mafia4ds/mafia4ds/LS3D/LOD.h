#pragma once
#include "Base.h"

struct LOD
{
	char *name;

	UINT16 numVerts;
	vector<Vertex> verts;

	BYTE numFaceGroups;
	vector<Facegroup> faceGroups;

	BYTE numBones;
	modelBone_t *bones;
	boneData_t *boneVerts;

	UINT32 *boneIdx;
	float *boneWeights;

	BYTE numTargets;
	BYTE numChannels;
	morphChannel_t *channels;

	LOD() : numVerts(0), verts(NULL), numFaceGroups(0), faceGroups(NULL), numBones(0), bones(NULL), channels(NULL) {};

	~LOD()
    {
		if (bones)      delete[] bones;
		if (channels)   delete[] channels;
    }
};