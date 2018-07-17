#ifndef SHADOWLISTENER_H__
#define SHADOWLISTENER_H__

#include "Ogre.h"
#include "rormemory.h"

#if OGRE_VERSION>0x010602
class SoftShadowListener : public Ogre::SceneManager::Listener, public MemoryAllocatedObject
#else
class SoftShadowListener : public Ogre::ShadowListener
#endif //OGRE_VERSION
{
    // this is a callback we'll be using to set up our shadow camera
#if OGRE_VERSION>0x010602
	void shadowTextureCasterPreViewProj(Ogre::Light* light, Ogre::Camera* camera, size_t iteration);
#else
    void shadowTextureCasterPreViewProj(Ogre::Light *light, Ogre::Camera *cam);
#endif //OGRE_VERSION
    
	virtual void shadowTexturesUpdated(size_t);
	virtual void shadowTextureReceiverPreViewProj(Ogre::Light*, Ogre::Frustum*);
    virtual void preFindVisibleObjects(Ogre::SceneManager*, Ogre::SceneManager::IlluminationRenderStage, Ogre::Viewport*);
    virtual void postFindVisibleObjects(Ogre::SceneManager*, Ogre::SceneManager::IlluminationRenderStage, Ogre::Viewport*);
#if OGRE_VERSION>0x010602
	virtual bool sortLightsAffectingFrustum(Ogre::LightList& lightList);
#endif
};

#endif //SHADOWLISTENER_H__
