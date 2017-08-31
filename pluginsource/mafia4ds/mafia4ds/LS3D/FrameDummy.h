////
//-- The dummy helper object/bounding box
////
template<class T>
struct Dummy : public Node
{
	T min;
	T max;

	// read from stream
	void read(RichBitStream *bs, UINT16 ver) 
	{
		bs->ReadBytes(&min, sizeof(T));
		bs->ReadBytes(&max, sizeof(T));
	};

	// render to rapi
	void render(noeRAPI_t *rapi) { };
};