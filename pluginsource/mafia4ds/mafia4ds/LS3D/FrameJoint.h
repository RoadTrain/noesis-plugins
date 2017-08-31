////
//-- FRAME_JOINT
////
struct Joint : public Node
{
	RichMat44 matrix;
	UINT32 ID;

	// read from stream
	void read(RichBitStream *bs, UINT16 ver)
	{
		if (ver == VERSION_MAFIA)
		{
			bs->ReadBytes(&matrix, sizeof(RichMat44));
		}

		ID = bs->ReadInt();
	};

	// render to rapi
	void render(noeRAPI_t *rapi) { };
};