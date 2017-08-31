#pragma once
#pragma pack(push, 1)

using namespace std;

namespace LS3D
{
	struct Color
	{
		float r, g, b;
	};

	struct Vertex
	{
		float pos[3];
		float norm[3];
		float uv[2];
	};

	struct MorphedVertex
	{
		float pos[3];
		float norm[3];
	};

	struct Face
	{
		UINT16 i[3];
	};

	struct Facegroup
	{
		UINT16 mtlID;
		UINT16 numFaces;
		vector<Face> faces;
	};


}