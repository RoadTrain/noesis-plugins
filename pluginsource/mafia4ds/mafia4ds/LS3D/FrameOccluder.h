////
//-- FRAME_OCCLUDER
////
template<class T>
struct Occluder : public Node
{
	T *verts;
	UINT32 numVerts;

	face_t *faces;
	UINT32 numFaces;

	// read from stream
	void read(RichBitStream *bs, UINT16 ver)
	{
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
		rapi->rpgBindPositionBuffer(verts, RPGEODATA_FLOAT, sizeof(T));
		rapi->rpgCommitTriangles(faces, RPGEODATA_USHORT, numFaces*3, RPGEO_TRIANGLE, true);
		rapi->rpgClearBufferBinds();
	};
};