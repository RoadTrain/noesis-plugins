////
//-- The light-sector class
////
template<class T>
struct Sector : public Node
{
	UINT32 flags[2];

	T min;
	T max;

	T *verts;
	UINT32 numVerts;

	face_t *faces;
	UINT32 numFaces;

	// read from stream
	void read(RichBitStream *bs, UINT16 ver) 
	{
		bs->ReadBytes(flags, 8);

		numVerts = bs->ReadInt();
		numFaces = bs->ReadInt();

		if (ver != VERSION_MAFIA)
		{
			bs->ReadBytes(&min, sizeof(T));
			bs->ReadBytes(&max, sizeof(T));
		}
		verts = new T[numVerts];
		bs->ReadBytes(verts, numVerts*sizeof(T));

		faces = new face_t[numFaces];
		bs->ReadBytes(faces, numFaces*sizeof(face_t));

		if (ver == VERSION_MAFIA)
		{
			bs->ReadBytes(&min, sizeof(T));
			bs->ReadBytes(&max, sizeof(T));
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
	};

	// render to rapi
	void render(noeRAPI_t *rapi) { };
};