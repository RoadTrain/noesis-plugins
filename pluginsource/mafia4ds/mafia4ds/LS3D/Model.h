#include "../../../pluginshare.h"
#include "../mafia4ds.h"
#include <vector>

#include "Base.h"

#pragma pack(push, 1)

using namespace std;

namespace LS3D {

	struct Material;
	struct Node;

	// a generic 4ds model class
	class Model
	{
	public:
		Model() : materials(NULL), numMaterials(0), numNodes(0), has5ds(0) { }

		~Model() { }

		void initFromBitStream(RichBitStream *bs);
		noesisModel_t *constructModel(noeRAPI_t *rapi);

		RichBitStream *bs;

		UINT16 ver;

		vector<Material> materials;
		UINT16 numMaterials;

		vector<std::shared_ptr<Node>> nodes;
		UINT16 numNodes;

		BYTE has5ds;
	private:
		void readHeader();
		void readMaterials();
		void readNodes();
	};

	struct Material
	{
		UINT32 flags;

		color_t ambient;
		color_t diffuse;
		color_t specular;
		color_t emissive;
		float shininess;
		float opacity;

		float reflection;
		char reflectionTextureName[256];
	
		char diffuseTextureName[256];

		char opacityTextureName[256];
	};

	//describes a node inside a 4ds file
	struct Node
	{
		UINT16 ID; //zero-based sequential ID

		BYTE frameType;
		BYTE visualType;

		UINT16 parentID; //1-based index exactly as in 4DS
		shared_ptr<Node> parent;

		RichMat43 transform;

		char nodeName[256];

		virtual void read(RichBitStream *bs, UINT16 ver) = 0;
		virtual void render(noeRAPI_t *rapi) = 0;
		// recursive transform calculation
		RichMat43 getTransform()
		{
			if (parentID != 0)
				return transform * parent->getTransform();
			else
				return transform;
		}
	};

	//visual object/singlemesh/singlemorph/morph
	#include "VisualObject.h"
	#include "VisualSinglemesh.h"

	//glowing
	#include "VisualLens.h"

	//mirror object (RichVec3/RichVec4-based)
	#include "VisualMirror.h"

	//light-sector
	#include "FrameSector.h"

	//dummy
	#include "FrameDummy.h"

	//target
	#include "FrameTarget.h"

	//joint
	#include "FrameJoint.h"

	//occluder
	#include "FrameOccluder.h"
}