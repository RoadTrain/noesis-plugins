////
//-- FRAME_TARGET
////
struct Target : public Node
{
	UINT16 *links;
	BYTE numLinks;

	// read from stream
	void read(RichBitStream *bs, UINT16 ver)
	{
		UINT16 unk;
		bs->ReadBytes(&unk, 2);

		numLinks = bs->ReadByte();

		links = new UINT16[numLinks];
		bs->ReadBytes(links, numLinks*sizeof(UINT16));
	};

	// render to rapi
	void render(noeRAPI_t *rapi) { };
};