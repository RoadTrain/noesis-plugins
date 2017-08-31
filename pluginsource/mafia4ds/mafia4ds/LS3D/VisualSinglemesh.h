#include "LOD.h"

////
//-- Visual Object with bones (SINGLEMESH)
////
struct VisualSinglemesh : public VisualObject
{
	void read(RichBitStream *bs, UINT16 ver)
	{
		// call parent method to read base object data
		VisualObject::read(bs, ver);

		
	}
};