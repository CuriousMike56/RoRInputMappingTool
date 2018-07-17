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

#include "materialFunctionMapper.h"
#include "Settings.h"

MaterialFunctionMapper::MaterialFunctionMapper()
{
	useSSAO = (SETTINGS.getSetting("SSAO") == "Yes");
}

MaterialFunctionMapper::~MaterialFunctionMapper()
{
}

void MaterialFunctionMapper::addMaterial(int flareid, materialmapping_t t)
{
	MaterialPtr m = Ogre::MaterialManager::getSingleton().getByName(t.material);
	if(m.isNull()) return;
	Technique *tech = m->getTechnique(0);
	if(!tech) return;
	Pass *p = tech->getPass(0);
	if(!p) return;
	// save emissive colour and then set to zero (light disabled by default)
	t.emissiveColour = p->getSelfIllumination();
	t.laststate = false;
	p->setSelfIllumination(ColourValue::ZERO);
	materialBindings[flareid].push_back(t);
}

void MaterialFunctionMapper::toggleFunction(int flareid, bool isvisible)
{
	std::vector<materialmapping_t> mb = materialBindings[flareid];
	std::vector<materialmapping_t>::iterator it;
	for(it=mb.begin(); it!=mb.end(); it++)
	{
		//if(it->laststate == isvisible)
		//	continue;
		//it->laststate = isvisible;

		MaterialPtr m = Ogre::MaterialManager::getSingleton().getByName(it->material);
		if(m.isNull()) continue;

		for(int i=0;i<m->getNumTechniques();i++)
		{
			Technique *tech = m->getTechnique(i);
			if(!tech) continue;

			if(tech->getSchemeName() == "glow")
			{
				// glowing technique
				// set the ambient value as glow amount
				Pass *p = tech->getPass(0);
				if(!p) continue;

				if(isvisible)
				{
					p->setSelfIllumination(it->emissiveColour);
					p->setAmbient(ColourValue::White);
					p->setDiffuse(ColourValue::White);
				} else
				{
					p->setSelfIllumination(ColourValue::ZERO);
					p->setAmbient(ColourValue::Black);
					p->setDiffuse(ColourValue::Black);
				}
			} else
			{
				// normal technique
				Pass *p = tech->getPass(0);
				if(!p) continue;

				TextureUnitState *tus = p->getTextureUnitState(0);
				if(!tus) continue;

				if(tus->getNumFrames() < 2)
					continue;

				int frame = isvisible?1:0;

				tus->setCurrentFrame(frame);

				if(isvisible)
					p->setSelfIllumination(it->emissiveColour);
				else
					p->setSelfIllumination(ColourValue::ZERO);
			}
		}
	}
}

MaterialPtr MaterialFunctionMapper::addSSAOToMaterial(MaterialPtr material)
{
	if(material.isNull())
		return MaterialPtr();

	LogManager::getSingleton().logMessage("##addSSAOToEntity");

	if(!material->getNumTechniques()) return MaterialPtr();
	Technique *tp = material->getTechnique(0);
	if(!tp) return MaterialPtr();

	if(!tp->getNumPasses()) return MaterialPtr();
	Pass *pp = tp->getPass(0);
	if(!pp) return MaterialPtr();

	if(!pp->getNumTextureUnitStates()) return MaterialPtr();
	TextureUnitState *tusp = pp->getTextureUnitState(0);
	if(!tusp) return MaterialPtr();

	String texture1 = tusp->getTextureName();
	if(texture1.empty()) return MaterialPtr();

	String targetName = material->getName()+"PlusSSAO";
	LogManager::getSingleton().logMessage("##addSSAOToEntity0: " + texture1);

	// check if already existing
	if(!MaterialManager::getSingleton().getByName(targetName).isNull())
		return MaterialManager::getSingleton().getByName(targetName);

	MaterialPtr m = MaterialManager::getSingleton().create(targetName, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	Technique *t0 = m->createTechnique();
	t0->setSchemeName("lighting");
	t0->setName("lighting");
	Pass *p0 = t0->createPass();
	p0->setAmbient(1,1,1);
	p0->setDiffuse(0,0,0,0);
	p0->setSpecular(0,0,0,0);
	p0->setSelfIllumination(0,0,0);
	p0->setVertexProgram("ambient_vs");
	p0->setFragmentProgram("ambient_ps");
	p0->createTextureUnitState(texture1); // FIRST TEXTURE HERE
	LogManager::getSingleton().logMessage("##addSSAOToEntity1: " + texture1);

	Pass *p1 = t0->createPass();
	p1->setMaxSimultaneousLights(8);
	p1->setSceneBlending(SBT_ADD);
	p1->setIteratePerLight(true);
	p1->setAmbient(0,0,0);
	p1->setDiffuse(1,1,1,0);
	p1->setSpecular(1,1,1,128);
	p1->setVertexProgram("diffuse_vs");
	p1->setFragmentProgram("diffuse_ps");
	p1->createTextureUnitState(texture1); // FIRST TEXTURE HERE
	LogManager::getSingleton().logMessage("##addSSAOToEntity2: " + texture1);

	TextureUnitState *tus = p1->createTextureUnitState();
	tus->setContentType(TextureUnitState::CONTENT_SHADOW);
	tus->setTextureFiltering(TFO_ANISOTROPIC);
	tus->setTextureAnisotropy(16);
	tus->setTextureAddressingMode(TextureUnitState::TAM_BORDER);
	tus->setTextureBorderColour(ColourValue(1,1,1));
	LogManager::getSingleton().logMessage("##addSSAOToEntity3: " + texture1);


	Technique *t1 = m->createTechnique();
	t1->setSchemeName("geom");
	p0 = t1->createPass();
	p0->setVertexProgram("geom_vs");
	p0->setFragmentProgram("geom_ps");
	LogManager::getSingleton().logMessage("##addSSAOToEntity4: " + texture1);

	return m;
}

void MaterialFunctionMapper::addSSAOToEntity(Ogre::Entity *e)
{
	MeshPtr m = e->getMesh();
	if(!m.isNull())
	{
		for(int n=0; n<(int)m->getNumSubMeshes();n++)
		{
			SubMesh *sm = m->getSubMesh(n);
			MaterialPtr material = Ogre::MaterialManager::getSingleton().getByName(sm->getMaterialName());
			if(material.isNull()) continue;

			MaterialPtr new_material = addSSAOToMaterial(material);
			if(!new_material.isNull())
			{
				sm->setMaterialName(new_material->getName());
				LogManager::getSingleton().logMessage("addSSAOToEntity: replaced mesh material " + sm->getMaterialName());
			}
		}
	}

	for(int n=0; n<(int)e->getNumSubEntities();n++)
	{
		SubEntity *subent = e->getSubEntity(n);
		MaterialPtr material = Ogre::MaterialManager::getSingleton().getByName(subent->getMaterialName());
		if(material.isNull()) continue;

		MaterialPtr new_material = addSSAOToMaterial(material);
		if(!new_material.isNull())
		{
			subent->setMaterialName(new_material->getName());
			LogManager::getSingleton().logMessage("addSSAOToEntity2: replaced mesh material " + subent->getMaterialName());
		}
	}
}

void MaterialFunctionMapper::replaceMeshMaterials(Ogre::Entity *e)
{
	if(!e)
	{
		LogManager::getSingleton().logMessage("MaterialFunctionMapper: got invalid Entity in replaceMeshMaterials");
		return;
	}
	// this is not nice, but required (its not so much performance relevant ...
	for(std::map <int, std::vector<materialmapping_t> >::iterator mfb=materialBindings.begin();mfb!=materialBindings.end();mfb++)
	{
		for(std::vector<materialmapping_t>::iterator mm=mfb->second.begin();mm!=mfb->second.end();mm++)
		{
			MeshPtr m = e->getMesh();
			if(!m.isNull())
			{
				for(int n=0; n<(int)m->getNumSubMeshes();n++)
				{
					SubMesh *sm = m->getSubMesh(n);
					if(sm->getMaterialName() ==  mm->originalmaterial)
					{
						sm->setMaterialName(mm->material);
						LogManager::getSingleton().logMessage("MaterialFunctionMapper: replaced mesh material " + mm->originalmaterial + " with new new material " + mm->material + " on entity " + e->getName());
					}
				}
			}

			for(int n=0; n<(int)e->getNumSubEntities();n++)
			{
				SubEntity *subent = e->getSubEntity(n);
				if(subent->getMaterialName() ==  mm->originalmaterial)
				{
					subent->setMaterialName(mm->material);
					LogManager::getSingleton().logMessage("MaterialFunctionMapper: replaced entity material " + mm->originalmaterial + " with new new material " + mm->material + " on entity " + e->getName());
				}
			}
		}
	}

	if(useSSAO)
		addSSAOToEntity(e);
}

int MaterialFunctionMapper::simpleMaterialCounter = 0;
void MaterialFunctionMapper::replaceSimpleMeshMaterials(Ogre::Entity *e, Ogre::ColourValue c)
{
	if(!e)
	{
		LogManager::getSingleton().logMessage("MaterialFunctionMapper: got invalid Entity in replaceSimpleMeshMaterials");
		return;
	}
	if (SETTINGS.getSetting("SimpleMaterials") != "Yes") return;

	MaterialPtr mat = MaterialManager::getSingleton().getByName("tracks/simple");
	if(mat.isNull()) return;

	String newMatName = "tracks/simple/" + StringConverter::toString(simpleMaterialCounter);
	MaterialPtr newmat = mat->clone(newMatName);

	newmat->getTechnique(0)->getPass(0)->setAmbient(c);

	simpleMaterialCounter++;
	
	MeshPtr m = e->getMesh();
	if(!m.isNull())
	{
		for(int n=0; n<(int)m->getNumSubMeshes();n++)
		{
			SubMesh *sm = m->getSubMesh(n);
			sm->setMaterialName(newMatName);
		}
	}

	for(int n=0; n<(int)e->getNumSubEntities();n++)
	{
		SubEntity *subent = e->getSubEntity(n);
		subent->setMaterialName(newMatName);
	}
}
