#include "Model.h"

using namespace LS3D;

////
//-- LS3D 4DS Model methods
////
void Model::initFromBitStream(RichBitStream *bs)
{
	bs->SetOffset(0);
	this->bs = bs;

	readHeader();
	readMaterials();
	readNodes();
}

void Model::readHeader()
{
	modelHdr_t hdr;
	bs->ReadBytes(&hdr, sizeof(modelHdr_t));
	this->ver = hdr.ver;
}

void Model::readMaterials()
{
	bs->ReadBytes(&numMaterials, 2);

	for (int i = 0; i < numMaterials; i++) 
	{
		Material mtl;
		mtl.flags = bs->ReadInt();

		bs->ReadBytes(&mtl.ambient, sizeof(color_t));

		if (ver != VERSION_MAFIA)
			bs->ReadInt(); //unknown

		bs->ReadBytes(&mtl.diffuse, sizeof(color_t));

		if (ver != VERSION_MAFIA)
		{
			bs->ReadInt(); //unknown
			bs->ReadBytes(&mtl.specular, sizeof(color_t));
		}

		bs->ReadBytes(&mtl.emissive, sizeof(color_t));

		if (ver != VERSION_MAFIA)
		{
			bs->ReadInt(); //unknown
			mtl.shininess = bs->ReadFloat();
		}

		mtl.opacity = bs->ReadFloat();

		bool hasTex = false;

		if (mtl.flags & MTL_REFLECTIONTEX) 
		{
			mtl.reflection = bs->ReadFloat();

			BYTE reflectionTextureNameLength = bs->ReadByte();

			if (reflectionTextureNameLength > 0) 
			{
				bs->ReadString(mtl.reflectionTextureName, reflectionTextureNameLength+1);
			}			
		}

		if (mtl.flags & MTL_DIFFUSETEX)
		{
			hasTex = true;
			BYTE diffuseTextureNameLength = bs->ReadByte();
				
			if (diffuseTextureNameLength > 0) 
			{
				bs->ReadString(mtl.diffuseTextureName, diffuseTextureNameLength+1);
			}
		}		

		if (mtl.flags & MTL_OPACITYTEX && !(mtl.flags & MTL_IMAGEALPHA))
		{
			hasTex = true;
			BYTE opacityTextureNameLength = bs->ReadByte();

			if (opacityTextureNameLength > 0) 
			{
				bs->ReadString(mtl.opacityTextureName, opacityTextureNameLength+1);
			}	
		}

		if (!hasTex)
		{
			bs->ReadByte();
		}

		if (mtl.flags & MTL_OPACITYTEX_ANIM)
		{
			mtlAnim_t opacityTextureAnimation;
			bs->ReadBytes(&opacityTextureAnimation, sizeof(mtlAnim_t));
		}
		else if (mtl.flags & MTL_DIFFUSETEX_ANIM)
        {
			mtlAnim_t diffuseTextureAnimation;
			bs->ReadBytes(&diffuseTextureAnimation, sizeof(mtlAnim_t));
		}

		materials.push_back(mtl);
	}
}

void Model::readNodes()
{
	bs->ReadBytes(&numNodes, 2);

	nodes.reserve(numNodes);

	for (int i = 0; i < numNodes; i++)
	{
		std::shared_ptr<Node> node;

		BYTE frameType = bs->ReadByte();
		BYTE visualType;

		// customize objects according to their types
		if (frameType == FRAME_VISUAL)
		{
			visualType = bs->ReadByte();

			if (visualType == VISUAL_OBJECT || visualType == VISUAL_SINGLEMESH || visualType == VISUAL_SINGLEMORPH || visualType == VISUAL_MORPH || visualType == VISUAL_BILLBOARD) 
			{
				node = std::shared_ptr<VisualObject>(new VisualObject);
				node->visualType = visualType;
			}
			else if (visualType == VISUAL_LENS)
			{
				node = std::shared_ptr<VisualLens>(new VisualLens);
			}
			else if (visualType == VISUAL_MIRROR)
			{
				if (ver == VERSION_HD2)
					node = std::shared_ptr<VisualMirror<RichVec4>>(new VisualMirror<RichVec4>);
				else
					node = std::shared_ptr<VisualMirror<RichVec3>>(new VisualMirror<RichVec3>);
			}
		}
		else if (frameType == FRAME_SECTOR)
		{
			if (ver == VERSION_HD2)
				node = std::shared_ptr<Sector<RichVec4>>(new Sector<RichVec4>);
			else
				node = std::shared_ptr<Sector<RichVec3>>(new Sector<RichVec3>);
		}
		else if (frameType == FRAME_DUMMY)
		{
			if (ver == VERSION_HD2)
				node = std::shared_ptr<Dummy<RichVec4>>(new Dummy<RichVec4>);
			else
				node = std::shared_ptr<Dummy<RichVec3>>(new Dummy<RichVec3>);
		}
		else if (frameType == FRAME_TARGET)
		{
			node = std::shared_ptr<Target>(new Target);
		}
		else if (frameType == FRAME_JOINT)
		{
			node = std::shared_ptr<Joint>(new Joint);
		}
		else if (frameType == FRAME_OCCLUDER)
		{
			if (ver == VERSION_HD2)
				node = std::shared_ptr<Occluder<RichVec4>>(new Occluder<RichVec4>);
			else
				node = std::shared_ptr<Occluder<RichVec3>>(new Occluder<RichVec3>);
		}

		//set up parent as a pointer
		bs->ReadBytes(&node->parentID, 2);
		if (node->parentID > 0)
			node->parent = nodes[node->parentID-1];
		else 
			node->parent = NULL;

		RichVec3 position, scale;
		RichQuat rotation;
		bs->ReadBytes(&position, sizeof(RichVec3));

		if (ver == VERSION_MAFIA)
		{
			bs->ReadBytes(&scale, sizeof(RichVec3));

			quat_wxyz_t quat_wxyz;
			bs->ReadBytes(&quat_wxyz, sizeof(quat_wxyz_t));
			rotation = quat_wxyz.ToQuat();
		}
		else
		{
			bs->ReadBytes(&rotation, sizeof(RichQuat));
			bs->ReadBytes(&scale, sizeof(RichVec3));

			if (ver == VERSION_HD2)
			{
				bs->ReadInt(); //unknown
			}
		}

		RichMat43 pos = RichMat43(RichVec3(1,0,0), RichVec3(0,1,0), RichVec3(0,0,1), position);
		RichMat43 scal = RichMat43(RichVec3(scale.v[0],0,0), RichVec3(0,scale.v[1],0), RichVec3(0,0,scale.v[2]), RichVec3(0,0,0));
		RichMat43 rot = rotation.ToMat43(); 

		node->transform = scal*rot*pos;

		BYTE cullingFlags = bs->ReadByte();

		BYTE nameLength = bs->ReadByte();
		
		if (nameLength > 0)
		{
			bs->ReadString(node->nodeName, nameLength+1);			
		}

		BYTE userPropertiesLength = bs->ReadByte();
		if (userPropertiesLength > 0)
		{
			char userProperties[256];
			bs->ReadString(userProperties, userPropertiesLength+1);
		}

		node->read(bs, ver);
		nodes[i] = node;
	}

	has5ds = bs->ReadByte();
}

noesisModel_t *Model::constructModel(noeRAPI_t *rapi)
{


	//4DS uses a different handedness
	rapi->rpgSetOption(RPGOPT_SWAPHANDEDNESS, true);

	noesisModel_t *mdl = rapi->rpgConstructModel();
	return mdl;
}