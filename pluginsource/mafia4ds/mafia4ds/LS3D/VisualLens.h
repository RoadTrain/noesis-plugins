////
//-- Glowing object (LENSFLARE)
////
struct VisualLens : public Node
{
	struct glow 
	{
		float position;
		float scale;
		UINT16 mtlID;
	} *glows;
	BYTE numGlows;

	// read from stream
	void read(RichBitStream *bs, UINT16 ver)
	{
		numGlows = bs->ReadByte();

		glows = new glow[numGlows];

		for (int i = 0; i < numGlows; i++)
		{
			glows[i].position = bs->ReadFloat();

			if (ver != VERSION_MAFIA)
				glows[i].scale = bs->ReadFloat();

			bs->ReadBytes(&glows[i].mtlID, 2);
		}
	}

	// render to rapi
	void render(noeRAPI_t *rapi) 
	{

	};
};