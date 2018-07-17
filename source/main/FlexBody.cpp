/*
This source file is part of Rigs of Rods
Copyright 2005,2006,2007,2008,2009 Pierre-Michel Ricordel
Copyright 2007,2008,2009 Thomas Fischer

For more information, see http://www.rigsofrods.com/

Rigs of Rods is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3, as 
published by the Free Software Foundation.

Rigs of Rods is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "FlexBody.h"
#include "ResourceBuffer.h"
#include "Settings.h"
#include "rormemory.h"
#include "skin.h"

FlexBody::FlexBody(SceneManager *manager, node_t *nds, int numnds, char* meshname, char* uname, int ref, int nx, int ny, Vector3 offset, Quaternion rot, char* setdef, MaterialFunctionMapper *mfm, Skin *usedSkin)
{
	nodes=nds;
	numnodes=numnds;
	cref=ref; nodes[cref].iIsSkin=true;
	cx=nx; nodes[cx].iIsSkin=true;
	cy=ny; nodes[cy].iIsSkin=true;
	coffset=offset;

	haveshadows=(manager->getShadowTechnique()==SHADOWTYPE_STENCIL_MODULATIVE || manager->getShadowTechnique()==SHADOWTYPE_STENCIL_ADDITIVE);
	havetangents=false;
	haveblend=true;

	freenodeset=0;
	//parsing set definition
	char* pos=setdef;
	char* end=pos;
	char endwas='G';
	while (endwas!=0)
	{
		unsigned int val1, val2;
		end=pos;
		while (*end!='-' && *end!=',' && *end!=0) end++;
		endwas=*end;
		*end=0;
		val1=strtoul(pos, 0, 10);
		if (endwas=='-')
		{
			pos=end+1;
			end=pos;
			while (*end!=',' && *end!=0) end++;
			endwas=*end;
			*end=0;
			val2=strtoul(pos, 0, 10);
			addinterval(val1, val2);
		}
		else addinterval(val1, val1);
		pos=end+1;
	}

	for (int i=0; i<freenodeset; i++)
		LogManager::getSingleton().logMessage("FLEXBODY node interval "+StringConverter::toString(i)+": "+StringConverter::toString(nodeset[i].from)+"-"+StringConverter::toString(nodeset[i].to));

	Vector3 normal = Vector3::UNIT_Y;
	Vector3 position = Vector3::ZERO;
	Quaternion orientation = Quaternion::ZERO;
	if(ref >= 0)
	{
		normal=(nodes[ny].smoothpos-nodes[ref].smoothpos).crossProduct(nodes[nx].smoothpos-nodes[ref].smoothpos);
		normal.normalise();
		//position
		position=nodes[ref].smoothpos+offset.x*(nodes[nx].smoothpos-nodes[ref].smoothpos)+offset.y*(nodes[ny].smoothpos-nodes[ref].smoothpos);
		position=(position+normal*offset.z);
		//orientation
		Vector3 refx=nodes[nx].smoothpos-nodes[ref].smoothpos;
		refx.normalise();
		Vector3 refy=refx.crossProduct(normal);
		orientation=Quaternion(refx, normal, refy)*rot;
	} else
	{
		// special case!
		normal = Vector3::UNIT_Y;
		position=nodes[0].smoothpos+offset;
		orientation = rot;
	}

	// load unique mesh (load original mesh and clone it with unique name)

	// find group that contains the mesh
	String groupname="";
	try
	{
		groupname = ResourceGroupManager::getSingleton().findGroupContainingResource(meshname);
	}catch(...)
	{
	}
	if(groupname == "")
	{
		LogManager::getSingleton().logMessage("FLEXBODY mesh not found: "+String(meshname));
		return;
	}
	// build new unique mesh name
	char uname_mesh[255];
	memset(uname_mesh, 0, 254);
	strcpy(uname_mesh, uname);
	strcat(uname_mesh, "_mesh");
	MeshPtr mesh = Ogre::MeshManager::getSingleton().load(meshname, groupname);
	MeshPtr newmesh = mesh->clone(uname_mesh);
	
	// now find possible LODs
 	String basename, ext;
	StringUtil::splitBaseFilename(String(meshname), basename, ext);
	for(int i=0; i<4;i++)
	{
		String fn = basename + "_" + StringConverter::toString(i) + ".mesh";
		bool exists = ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(fn);
		if(!exists) continue;

		float distance = 3;
		if(i == 1) distance = 20;
		if(i == 2) distance = 50;
		if(i == 3) distance = 200;
		newmesh->createManualLodLevel(distance, fn);
	}

	Entity *ent = manager->createEntity(uname, uname_mesh);
	MaterialFunctionMapper::replaceSimpleMeshMaterials(ent, ColourValue(0.5, 0.5, 1));
	if(mfm) mfm->replaceMeshMaterials(ent);
	if(usedSkin) usedSkin->replaceMeshMaterials(ent);
	LogManager::getSingleton().logMessage("FLEXBODY unique mesh created: "+String(meshname)+" -> "+String(uname_mesh));

	msh=ent->getMesh();

	//determine if we have texture coordinates everywhere
	havetexture=true;
	if (msh->sharedVertexData && msh->sharedVertexData->vertexDeclaration->findElementBySemantic(VES_TEXTURE_COORDINATES)==0) havetexture=false;
	for (int i=0; i<msh->getNumSubMeshes(); i++) if (!msh->getSubMesh(i)->useSharedVertices && msh->getSubMesh(i)->vertexData->vertexDeclaration->findElementBySemantic(VES_TEXTURE_COORDINATES)==0) havetexture=false;
	if (!havetexture) LogManager::getSingleton().logMessage("FLEXBODY Warning: at least one part of this mesh does not have texture coordinates, switching off texturing!");
	if (!havetexture) {havetangents=false;haveblend=false;}; //we can't do this

	//detect the anomalous case where a mesh is exported without normal vectors
	bool havenormal=true;
	if (msh->sharedVertexData && msh->sharedVertexData->vertexDeclaration->findElementBySemantic(VES_NORMAL)==0) havenormal=false;
	for (int i=0; i<msh->getNumSubMeshes(); i++) if (!msh->getSubMesh(i)->useSharedVertices && msh->getSubMesh(i)->vertexData->vertexDeclaration->findElementBySemantic(VES_NORMAL)==0) havenormal=false;
	if (!havenormal) LogManager::getSingleton().logMessage("FLEXBODY Error: at least one part of this mesh does not have normal vectors, export your mesh with normal vectors! THIS WILL CRASH IN 3.2.1...");

	//create optimal VertexDeclaration
	VertexDeclaration* optimalVD=HardwareBufferManager::getSingleton().createVertexDeclaration();
	optimalVD->addElement(0, 0, VET_FLOAT3, VES_POSITION);
	optimalVD->addElement(1, 0, VET_FLOAT3, VES_NORMAL);
	if (haveblend) optimalVD->addElement(2, 0, VET_COLOUR_ARGB, VES_DIFFUSE);
	if (havetexture) optimalVD->addElement(3, 0, VET_FLOAT2, VES_TEXTURE_COORDINATES);
	if (havetangents) optimalVD->addElement(4, 0, VET_FLOAT3, VES_TANGENT);
	optimalVD->sort();
	optimalVD->closeGapsInSource();

	BufferUsageList optimalBufferUsages;
	for (size_t u = 0; u <= optimalVD->getMaxSource(); ++u) optimalBufferUsages.push_back(HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY_DISCARDABLE);

	//print mesh information
	LogManager::getSingleton().logMessage("FLEXBODY Printing input mesh informations:");
	printMeshInfo(ent->getMesh().getPointer());

	//adding color buffers, well get the reference later
	if (haveblend)
	{
		if (msh->sharedVertexData)
		{
			if (msh->sharedVertexData->vertexDeclaration->findElementBySemantic(VES_DIFFUSE)==0)
			{
				//add buffer
				int index=msh->sharedVertexData->vertexDeclaration->getMaxSource()+1;
				msh->sharedVertexData->vertexDeclaration->addElement(index, 0, VET_COLOUR_ARGB, VES_DIFFUSE);
				msh->sharedVertexData->vertexDeclaration->sort();
				index=msh->sharedVertexData->vertexDeclaration->findElementBySemantic(VES_DIFFUSE)->getSource();
				HardwareVertexBufferSharedPtr vbuf=HardwareBufferManager::getSingleton().createVertexBuffer(VertexElement::getTypeSize(VET_COLOUR_ARGB), msh->sharedVertexData->vertexCount, HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY_DISCARDABLE);
				msh->sharedVertexData->vertexBufferBinding->setBinding(index, vbuf);
			}
		}
		for (int i=0; i<msh->getNumSubMeshes(); i++) if (!msh->getSubMesh(i)->useSharedVertices)
		{
			if (msh->getSubMesh(i)->vertexData->vertexDeclaration->findElementBySemantic(VES_DIFFUSE)==0)
			{
				//add buffer
				int index=msh->getSubMesh(i)->vertexData->vertexDeclaration->getMaxSource()+1;
				msh->getSubMesh(i)->vertexData->vertexDeclaration->addElement(index, 0, VET_COLOUR_ARGB, VES_DIFFUSE);
				msh->getSubMesh(i)->vertexData->vertexDeclaration->sort();
				index=msh->getSubMesh(i)->vertexData->vertexDeclaration->findElementBySemantic(VES_DIFFUSE)->getSource();
				HardwareVertexBufferSharedPtr vbuf=HardwareBufferManager::getSingleton().createVertexBuffer(VertexElement::getTypeSize(VET_COLOUR_ARGB), msh->getSubMesh(i)->vertexData->vertexCount, HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY_DISCARDABLE);
				msh->getSubMesh(i)->vertexData->vertexBufferBinding->setBinding(index, vbuf);
			}
		}
	}

	//tangents for envmapping
	if (havetangents)
	{
		LogManager::getSingleton().logMessage("FLEXBODY preparing for tangents");
		unsigned short srcTex, destTex;
		bool existing = msh->suggestTangentVectorBuildParams(VES_TANGENT, srcTex, destTex);
		if (!existing) msh->buildTangentVectors(VES_TANGENT, srcTex, destTex);
	}

	//reorg
	LogManager::getSingleton().logMessage("FLEXBODY reorganizing buffers");
	if (msh->sharedVertexData)
	{
		msh->sharedVertexData->reorganiseBuffers(optimalVD, optimalBufferUsages);
		msh->sharedVertexData->removeUnusedBuffers();
		msh->sharedVertexData->closeGapsInBindings();
	}
    Mesh::SubMeshIterator smIt = msh->getSubMeshIterator();
	unsigned short idx = 0;
	while (smIt.hasMoreElements())
	{
		SubMesh* sm = smIt.getNext();
		if (!sm->useSharedVertices)
		{
			sm->vertexData->reorganiseBuffers(optimalVD, optimalBufferUsages);
			sm->vertexData->removeUnusedBuffers();
			sm->vertexData->closeGapsInBindings();
		}
	}

	//print mesh information
	LogManager::getSingleton().logMessage("FLEXBODY Printing modififed mesh informations:");
	printMeshInfo(ent->getMesh().getPointer());

	//get the buffers
	//getMeshInformation(ent->getMesh().getPointer(),vertex_count,vertices,index_count,indices, position, orientation, Vector3(1,1,1));

	//getting vertex counts
	vertex_count=0;
	hasshared=false;
	numsubmeshbuf=0;
	if (msh->sharedVertexData) 
	{
		vertex_count+=msh->sharedVertexData->vertexCount;
		hasshared=true;
	}
	for (int i=0; i<msh->getNumSubMeshes(); i++) 
		if (!msh->getSubMesh(i)->useSharedVertices) 
		{
			vertex_count+=msh->getSubMesh(i)->vertexData->vertexCount;
			numsubmeshbuf++;
		}

	LogManager::getSingleton().logMessage("FLEXBODY Vertices in mesh "+String(meshname)+": "+ StringConverter::toString(vertex_count));
	//LogManager::getSingleton().logMessage("Triangles in mesh: %u",index_count / 3);

	//getting buffers bindings and data
	if (numsubmeshbuf>0)
	{
		submeshnums=(int*)ror_malloc(sizeof(int)*numsubmeshbuf);
		subnodecounts=(int*)ror_malloc(sizeof(int)*numsubmeshbuf);
		//C++ is just dumb! 
		//How can they manage to break such a fundamental programming mechanisms?
		//They invented the un-initializable and un-attribuable objects you can't allocate dynamically!
		//I'm sure they have a fancy way to do that but they won't pry my precious malloc() from my cold, dead hands! goddamit!
		//subpbufs=(HardwareVertexBufferSharedPtr*)malloc(sizeof(HardwareVertexBufferSharedPtr)*numsubmeshbuf);
		//subpbufs[0]=HardwareVertexBufferSharedPtr(); //crash!
		//subnbufs=(HardwareVertexBufferSharedPtr*)malloc(sizeof(HardwareVertexBufferSharedPtr)*numsubmeshbuf);
		//subnbufs[0]=HardwareVertexBufferSharedPtr(); //crash!
		if (numsubmeshbuf>=16) 	LogManager::getSingleton().logMessage("FLEXBODY You have more than 16 submeshes! Blame Bjarne for this crash.");
	}
	vertices=(Vector3*)ror_malloc(sizeof(Vector3)*vertex_count);
	dstpos=(Vector3*)ror_malloc(sizeof(Vector3)*vertex_count);
	srcnormals=(Vector3*)ror_malloc(sizeof(Vector3)*vertex_count);
	dstnormals=(Vector3*)ror_malloc(sizeof(Vector3)*vertex_count);
	if (haveblend) 
	{
		srccolors=(ARGB*)ror_malloc(sizeof(ARGB)*vertex_count);
		for (int i=0; i<(int)vertex_count; i++) srccolors[i]=0x00000000;
	}
	Vector3* vpt=vertices;
	Vector3* npt=srcnormals;
	int cursubmesh=0;
	if (msh->sharedVertexData) 
	{
		sharedcount=msh->sharedVertexData->vertexCount;
		//vertices
		int source=msh->sharedVertexData->vertexDeclaration->findElementBySemantic(VES_POSITION)->getSource();
		sharedpbuf=msh->sharedVertexData->vertexBufferBinding->getBuffer(source);
		sharedpbuf->readData(0, msh->sharedVertexData->vertexCount*sizeof(Vector3), (void*)vpt);
		vpt+=msh->sharedVertexData->vertexCount;
		//normals
		source=msh->sharedVertexData->vertexDeclaration->findElementBySemantic(VES_NORMAL)->getSource();
		sharednbuf=msh->sharedVertexData->vertexBufferBinding->getBuffer(source);
		sharednbuf->readData(0, msh->sharedVertexData->vertexCount*sizeof(Vector3), (void*)npt);
		npt+=msh->sharedVertexData->vertexCount;
		//colors
		if (haveblend)
		{
			source=msh->sharedVertexData->vertexDeclaration->findElementBySemantic(VES_DIFFUSE)->getSource();
			sharedcbuf=msh->sharedVertexData->vertexBufferBinding->getBuffer(source);
			sharedcbuf->writeData(0, msh->sharedVertexData->vertexCount*sizeof(ARGB), (void*)srccolors);
		}
	}
	for (int i=0; i<msh->getNumSubMeshes(); i++) if (!msh->getSubMesh(i)->useSharedVertices) 
	{
		submeshnums[cursubmesh]=i;
		subnodecounts[cursubmesh]=msh->getSubMesh(i)->vertexData->vertexCount;
		//vertices
		int source=msh->getSubMesh(i)->vertexData->vertexDeclaration->findElementBySemantic(VES_POSITION)->getSource();
		subpbufs[cursubmesh]=msh->getSubMesh(i)->vertexData->vertexBufferBinding->getBuffer(source);
		subpbufs[cursubmesh]->readData(0, msh->getSubMesh(i)->vertexData->vertexCount*sizeof(Vector3), (void*)vpt);
		vpt+=msh->getSubMesh(i)->vertexData->vertexCount;
		//normals
		source=msh->getSubMesh(i)->vertexData->vertexDeclaration->findElementBySemantic(VES_NORMAL)->getSource();
		subnbufs[cursubmesh]=msh->getSubMesh(i)->vertexData->vertexBufferBinding->getBuffer(source);
		subnbufs[cursubmesh]->readData(0, msh->getSubMesh(i)->vertexData->vertexCount*sizeof(Vector3), (void*)npt);
		npt+=msh->getSubMesh(i)->vertexData->vertexCount;
		//colors
		if (haveblend)
		{
			source=msh->getSubMesh(i)->vertexData->vertexDeclaration->findElementBySemantic(VES_DIFFUSE)->getSource();
			subcbufs[cursubmesh]=msh->getSubMesh(i)->vertexData->vertexBufferBinding->getBuffer(source);
			subcbufs[cursubmesh]->writeData(0, msh->getSubMesh(i)->vertexData->vertexCount*sizeof(ARGB), (void*)srccolors);
		}
		cursubmesh++;
	}

	//transform
	for (int i=0; i<(int)vertex_count; i++)
	{
		vertices[i]=(orientation*vertices[i])+position;
	}

	locs=(Locator_t*)ror_malloc(sizeof(Locator_t)*vertex_count);
	for (int i=0; i<(int)vertex_count; i++)
	{
		//search nearest node as the local origin
		float mindist=100000.0;
		int minnode=-1;
		for (int k=0; k<numnodes; k++)
		{
			if (!isinset(k)) continue;
			//if (nodes[k].iswheel) continue;
			float dist=(vertices[i]-nodes[k].smoothpos).length();
			if (dist<mindist) {mindist=dist;minnode=k;};
		}
		if (minnode==-1) LogManager::getSingleton().logMessage("FLEXBODY ERROR on mesh "+String(meshname)+": REF node not found");
		locs[i].ref=minnode;
		nodes[minnode].iIsSkin=true;

//	LogManager::getSingleton().logMessage("FLEXBODY distance to "+StringConverter::toString(minnode)+" "+StringConverter::toString(mindist));

		//search the second nearest node as the X vector
		mindist=100000.0;
		minnode=-1;
		for (int k=0; k<numnodes; k++)
		{
			if (!isinset(k)) continue;
			//if (nodes[k].iswheel) continue;
			if (k==locs[i].ref) continue;
			float dist=(vertices[i]-nodes[k].smoothpos).length();
			if (dist<mindist) {mindist=dist;minnode=k;};
		}
		if (minnode==-1) LogManager::getSingleton().logMessage("FLEXBODY ERROR on mesh "+String(meshname)+": VX node not found");
		locs[i].nx=minnode;
		nodes[minnode].iIsSkin=true;

		//search another close, orthogonal node as the Y vector
		mindist=100000.0;
		minnode=-1;
		Vector3 vx=nodes[locs[i].nx].smoothpos-nodes[locs[i].ref].smoothpos;
		vx.normalise();
		for (int k=0; k<numnodes; k++)
		{
			if (!isinset(k)) continue;
			//if (nodes[k].iswheel) continue;
			if (k==locs[i].ref) continue;
			if (k==locs[i].nx) continue;
			Vector3 vt=nodes[k].smoothpos-nodes[locs[i].ref].smoothpos;
			vt.normalise();
			float cost=vx.dotProduct(vt);
			if (cost>0.707 || cost<-0.707) continue; //rejection, fails the orthogonality criterion (+-45 degree)
			float dist=(vertices[i]-nodes[k].smoothpos).length();
			if (dist<mindist) {mindist=dist;minnode=k;};
		}
		if (minnode==-1) LogManager::getSingleton().logMessage("FLEXBODY ERROR on mesh "+String(meshname)+": VY node not found");
		locs[i].ny=minnode;
		nodes[minnode].iIsSkin=true;
/*
		//search the final close, orthogonal node as the Z vector
		mindist=100000.0;
		minnode=-1;
		Vector3 vy=nodes[locs[i].ny].smoothpos-nodes[locs[i].ref].smoothpos;
		vy.normalise();
		for (int k=0; k<numnodes; k++)
		{
			if (nodes[k].iswheel) continue;
			if (k==locs[i].ref) continue;
			if (k==locs[i].nx) continue;
			if (k==locs[i].ny) continue;
			Vector3 vt=nodes[k].smoothpos-nodes[locs[i].ref].smoothpos;
			vt.normalise();
			float cost=vx.dotProduct(vt);
			if (cost>0.707 || cost<-0.707) continue; //rejection, fails the orthogonality criterion (+-45 degree)
			cost=vy.dotProduct(vt);
			if (cost>0.707 || cost<-0.707) continue; //rejection, fails the orthogonality criterion (+-45 degree)
			float dist=(vertices[i]-nodes[k].smoothpos).length();
			if (dist<mindist) {mindist=dist;minnode=k;};
		}
		if (minnode==-1) LogManager::getSingleton().logMessage("FLEXBODY ERROR on mesh "+String(meshname)+": VZ node not found");
		locs[i].nz=minnode;

		//rright, check orientation
		Vector3 xyn=vx.crossProduct(vy);
		if (xyn.dotProduct(nodes[locs[i].nz].smoothpos-nodes[locs[i].ref].smoothpos)<0)
		{
			//the base is messed up
			int t=locs[i].nz;
			locs[i].nz=locs[i].ny;
			locs[i].ny=t;
		}
*/
		Vector3 vz=(nodes[locs[i].nx].smoothpos-nodes[locs[i].ref].smoothpos).crossProduct(nodes[locs[i].ny].smoothpos-nodes[locs[i].ref].smoothpos);
		vz.normalise();
		Matrix3 mat;
		mat.SetColumn(0, nodes[locs[i].nx].smoothpos-nodes[locs[i].ref].smoothpos);
		mat.SetColumn(1, nodes[locs[i].ny].smoothpos-nodes[locs[i].ref].smoothpos);
//		mat.SetColumn(2, nodes[locs[i].nz].smoothpos-nodes[locs[i].ref].smoothpos);
		mat.SetColumn(2, vz);
		mat=mat.Inverse();

		//compute coordinates in the newly formed euclidian basis
		locs[i].coords=mat*(vertices[i]-nodes[locs[i].ref].smoothpos);

		//thats it!
	}

	//shadow
	if (haveshadows)
	{
		LogManager::getSingleton().logMessage("FLEXBODY preparing for shadow volume");
		msh->prepareForShadowVolume(); //we do this always so we have only one datastructure format to manage
		msh->buildEdgeList();
	}

	//adjusting bounds
	AxisAlignedBox aab=msh->getBounds();
	Vector3 v=aab.getMinimum();
	float mi=v.x;
	if (v.y<mi) mi=v.y;
	if (v.z<mi) mi=v.z;
	mi=fabs(mi);
	v=aab.getMaximum();
	float ma=v.x;
	if (ma<v.y) ma=v.y;
	if (ma<v.z) ma=v.z;
	ma=fabs(ma);
	if (mi>ma) ma=mi;
	aab.setMinimum(Vector3(-ma,-ma,-ma));
	aab.setMaximum(Vector3(ma,ma,ma));
	msh->_setBounds(aab, true);

	LogManager::getSingleton().logMessage("FLEXBODY show mesh");
	//okay, show the mesh now
	snode=manager->getRootSceneNode()->createChildSceneNode();
	snode->attachObject(ent);
	snode->setPosition(position);

#if 0
	// XXX TODO: fix 1.7 LODs
	if(enable_truck_lod)
	{
		String lodstr = "FLEXBODY LODs: ";
		for(int i=0;i<msh->getNumLodLevels();i++)
		{
			if(i) lodstr += ", ";
			lodstr += StringConverter::toString(Real(sqrt(msh->getLodLevel(i).fromDepthSquared))) + "m";

			if(msh->getLodLevel(i).edgeData)
			{
				lodstr += "(" + StringConverter::toString(msh->getLodLevel(i).edgeData->triangles.size()) + " triangles)";
			} else
			{
				if(msh->getEdgeList(i))
					lodstr += "(" + StringConverter::toString(msh->getEdgeList(i)->triangles.size()) +" triangles)";
			}
		}
		LogManager::getSingleton().logMessage(lodstr);
	}
#endif //0


	for (int i=0; i<(int)vertex_count; i++)
	{
		Vector3 vz=(nodes[locs[i].nx].smoothpos-nodes[locs[i].ref].smoothpos).crossProduct(nodes[locs[i].ny].smoothpos-nodes[locs[i].ref].smoothpos);
		vz.normalise();
		Matrix3 mat;
		mat.SetColumn(0, nodes[locs[i].nx].smoothpos-nodes[locs[i].ref].smoothpos);
		mat.SetColumn(1, nodes[locs[i].ny].smoothpos-nodes[locs[i].ref].smoothpos);
//		mat.SetColumn(2, nodes[locs[i].nz].smoothpos-nodes[locs[i].ref].smoothpos);
		mat.SetColumn(2, vz);
		mat=mat.Inverse();

		//compute coordinates in the euclidian basis
		srcnormals[i]=mat*(orientation*srcnormals[i]);
	}


	LogManager::getSingleton().logMessage("FLEXBODY ready");

}

void FlexBody::setVisible(bool visible)
{
	if(snode) snode->setVisible(visible);
}

void FlexBody::printMeshInfo(Mesh* mesh)
{
	if (mesh->sharedVertexData)
	{
		LogManager::getSingleton().logMessage("FLEXBODY Mesh has Shared Vertices:");
		VertexData* vt=mesh->sharedVertexData;
		LogManager::getSingleton().logMessage("FLEXBODY element count:"+StringConverter::toString(vt->vertexDeclaration->getElementCount()));
		for (int j=0; j<(int)vt->vertexDeclaration->getElementCount(); j++)
		{
			const VertexElement* ve=vt->vertexDeclaration->getElement(j);
			LogManager::getSingleton().logMessage("FLEXBODY element "+StringConverter::toString(j)+" source "+StringConverter::toString(ve->getSource()));
			LogManager::getSingleton().logMessage("FLEXBODY element "+StringConverter::toString(j)+" offset "+StringConverter::toString(ve->getOffset()));
			LogManager::getSingleton().logMessage("FLEXBODY element "+StringConverter::toString(j)+" type "+StringConverter::toString(ve->getType()));
			LogManager::getSingleton().logMessage("FLEXBODY element "+StringConverter::toString(j)+" semantic "+StringConverter::toString(ve->getSemantic()));
			LogManager::getSingleton().logMessage("FLEXBODY element "+StringConverter::toString(j)+" size "+StringConverter::toString(ve->getSize()));
		}
	}
	LogManager::getSingleton().logMessage("FLEXBODY Mesh has "+StringConverter::toString(mesh->getNumSubMeshes())+" submesh(es)");
	for (int i=0; i<mesh->getNumSubMeshes(); i++)
	{
		SubMesh* submesh = mesh->getSubMesh(i);
		LogManager::getSingleton().logMessage("FLEXBODY SubMesh "+StringConverter::toString(i)+": uses shared?:"+StringConverter::toString(submesh->useSharedVertices));
		if (!submesh->useSharedVertices)
		{
			VertexData* vt=submesh->vertexData;
			LogManager::getSingleton().logMessage("FLEXBODY element count:"+StringConverter::toString(vt->vertexDeclaration->getElementCount()));
			for (int j=0; j<(int)vt->vertexDeclaration->getElementCount(); j++)
			{
				const VertexElement* ve=vt->vertexDeclaration->getElement(j);
				LogManager::getSingleton().logMessage("FLEXBODY element "+StringConverter::toString(j)+" source "+StringConverter::toString(ve->getSource()));
				LogManager::getSingleton().logMessage("FLEXBODY element "+StringConverter::toString(j)+" offset "+StringConverter::toString(ve->getOffset()));
				LogManager::getSingleton().logMessage("FLEXBODY element "+StringConverter::toString(j)+" type "+StringConverter::toString(ve->getType()));
				LogManager::getSingleton().logMessage("FLEXBODY element "+StringConverter::toString(j)+" semantic "+StringConverter::toString(ve->getSemantic()));
				LogManager::getSingleton().logMessage("FLEXBODY element "+StringConverter::toString(j)+" size "+StringConverter::toString(ve->getSize()));
			}
		}
	}
}


bool FlexBody::isinset(int n)
{
	for (int i=0; i<freenodeset; i++) if (n>=nodeset[i].from && n<=nodeset[i].to) return true;
	return false;
}

Vector3 FlexBody::flexit()
{
	if (haveblend) updateBlend();
	//compute the local center

	Vector3 normal;
	Vector3 center;
	if(cref >= 0)
	{
		normal=(nodes[cy].smoothpos-nodes[cref].smoothpos).crossProduct(nodes[cx].smoothpos-nodes[cref].smoothpos);
		normal.normalise();
		center=nodes[cref].smoothpos+coffset.x*(nodes[cx].smoothpos-nodes[cref].smoothpos)+coffset.y*(nodes[cy].smoothpos-nodes[cref].smoothpos);
		center=(center+normal*coffset.z);
	} else
	{
		normal = Vector3::UNIT_Y;
		center = nodes[0].smoothpos;
	}

	//okay
	for (int i=0; i<(int)vertex_count; i++)
	{
		Locator_t *loc=&locs[i];
		Matrix3 mat;
		mat.SetColumn(0, nodes[loc->nx].smoothpos-nodes[loc->ref].smoothpos);
		mat.SetColumn(1, nodes[loc->ny].smoothpos-nodes[loc->ref].smoothpos);
//		mat.SetColumn(2, nodes[loc->nz].smoothpos-nodes[loc->ref].smoothpos);
		Vector3 vz=(nodes[loc->nx].smoothpos-nodes[loc->ref].smoothpos).crossProduct(nodes[loc->ny].smoothpos-nodes[loc->ref].smoothpos);
		vz.normalise();
		mat.SetColumn(2, vz);

		dstpos[i]=mat*loc->coords+nodes[loc->ref].smoothpos-center;
		dstnormals[i]=mat*srcnormals[i];
		dstnormals[i].normalise(); //painfull but necessary!
	}
	
	Vector3 *ppt=dstpos;
	Vector3 *npt=dstnormals;
	if (hasshared)
	{
		sharedpbuf->writeData(0, sharedcount*sizeof(Vector3), ppt, true);
		ppt+=sharedcount;
		sharednbuf->writeData(0, sharedcount*sizeof(Vector3), npt, true);
		npt+=sharedcount;
	}
	for (int i=0; i<numsubmeshbuf; i++)
	{
		subpbufs[i]->writeData(0, subnodecounts[i]*sizeof(Vector3), ppt, true);
		ppt+=subnodecounts[i];
		subnbufs[i]->writeData(0, subnodecounts[i]*sizeof(Vector3), npt, true);
		npt+=subnodecounts[i];
	}

	if (haveshadows)
	{
		EdgeData * 	ed=msh->getEdgeList();
		//ed->updateFaceNormals(0, pbuf); //wtf?
	}

	snode->setPosition(center);
	return center;
}

void FlexBody::reset()
{
	if (haveblend) 
	{
		for (int i=0; i<(int)vertex_count; i++) srccolors[i]=0x00000000;
		writeBlend();
	}
}

void FlexBody::writeBlend()
{
	if (!haveblend) return;
	ARGB *cpt=srccolors;
	if (hasshared)
	{
		sharedcbuf->writeData(0, sharedcount*sizeof(ARGB), (void*)cpt, true);
		cpt+=sharedcount;
	}
	for (int i=0; i<numsubmeshbuf; i++)
	{
		subcbufs[i]->writeData(0, subnodecounts[i]*sizeof(ARGB), (void*)cpt, true);
		cpt+=subnodecounts[i];
	}
}

void FlexBody::updateBlend() //so easy!
{
	bool changed=false;
	for (int i=0; i<(int)vertex_count; i++)
	{
		node_t *nd=&nodes[locs[i].ref];
		ARGB col=srccolors[i];
		if (nd->contacted && !(col&0xFF000000))
		{
			srccolors[i]=col|0xFF000000;
			changed=true;
		}
		if ((nd->wetstate!=DRY) ^ ((col&0x000000FF)>0))
		{
			srccolors[i]=(col&0xFFFFFF00)+0x000000FF*(nd->wetstate!=DRY);
			changed=true;
		}
	}
	if (changed) writeBlend();
}
