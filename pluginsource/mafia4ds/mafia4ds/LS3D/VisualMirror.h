////
//-- Mirror object
////
template<class T>
struct VisualMirror : public Node
{
	RichMat44 reflectionMatrix;
	color_t color;

	T *verts;
	UINT32 numVerts;

	face_t *faces;
	UINT32 numFaces;

	// read from stream
	void read(RichBitStream *bs, UINT16 ver)
	{
		T min, max;
		bs->ReadBytes(&min, sizeof(T));
		bs->ReadBytes(&max, sizeof(T));

		float unk[4];
		bs->ReadBytes(unk, 4*sizeof(float));

		bs->ReadBytes(&reflectionMatrix, sizeof(RichMat44));

		bs->ReadBytes(&color, sizeof(color_t));

		if (ver == VERSION_HD2)
			bs->ReadInt();//unknown

		float reflectionStrength = bs->ReadFloat();
		numVerts = bs->ReadInt();
		numFaces = bs->ReadInt();

		verts = new T[numVerts];
		bs->ReadBytes(verts, numVerts*sizeof(T));

		faces = new face_t[numFaces];
		bs->ReadBytes(faces, numFaces*sizeof(face_t));
			
	};

	// render to rapi
	void render(noeRAPI_t *rapi) 
	{
		rapi->rpgBindPositionBuffer(verts[0].v, RPGEODATA_FLOAT, sizeof(T));
		rapi->rpgCommitTriangles(faces, RPGEODATA_USHORT, numFaces*3, RPGEO_TRIANGLE, true);
		rapi->rpgClearBufferBinds();
	};
};