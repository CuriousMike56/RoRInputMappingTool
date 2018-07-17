// "Depth of Field" demo for Ogre
// Copyright (C) 2006  Christian Lindequist Larsen
//
// This code is in the public domain. You may do whatever you want with it.

#ifndef __DepthOfFieldEffect_H__
#define __DepthOfFieldEffect_H__

#include "OgrePrerequisites.h"
#include "OgreCompositorInstance.h"
#include "OgreRenderTargetListener.h"
#include "OgreFrameListener.h"
#include "OgreRenderQueue.h"
#include "rormemory.h"

class Lens;

class DepthOfFieldEffect : public Ogre::CompositorInstance::Listener,
						   public Ogre::RenderTargetListener,
						   public Ogre::RenderQueue::RenderableListener,
						   public MemoryAllocatedObject
{
public:
	DepthOfFieldEffect(Ogre::Viewport* viewport);
	~DepthOfFieldEffect();

	float getNearDepth() const {return mNearDepth; }
	float getFocalDepth() const {return mFocalDepth; }
	float getFarDepth() const {return mFarDepth; }
	void setFocalDepths(float nearDepth, float focalDepth, float farDepth);
	float getFarBlurCutoff() const {return mFarBlurCutoff; }
	void setFarBlurCutoff(float cutoff);
	bool getEnabled() const;
	void setEnabled(bool value);

private:
	static const int BLUR_DIVISOR;

	enum PassId
	{
		BlurPass,
		OutputPass
	};

	Ogre::Viewport* mViewport;
	Ogre::MaterialPtr mDepthMaterial;
	Ogre::Technique* mDepthTechnique;
	Ogre::CompositorInstance* mCompositor;
	float mNearDepth;
	float mFocalDepth;
	float mFarDepth;
	float mFarBlurCutoff;

	void createDepthRenderTexture();
	void destroyDepthRenderTexture();
	void createCompositor();
	void destroyCompositor();
	void addCompositor();
	void removeCompositor();

	// Implementation of Ogre::CompositorInstance::Listener
	virtual void notifyMaterialSetup(Ogre::uint32 passId, Ogre::MaterialPtr& material);

	// Implementation of Ogre::RenderTargetListener
	virtual void preViewportUpdate(const Ogre::RenderTargetViewportEvent& evt);
	virtual void postViewportUpdate(const Ogre::RenderTargetViewportEvent& evt);

	// Implementation of Ogre::RenderQueue::RenderableListener
#if OGRE_VERSION>0x010602
	virtual bool renderableQueued(Ogre::Renderable* rend, Ogre::uint8 groupID, Ogre::ushort priority, Ogre::Technique** ppTech, Ogre::RenderQueue* pQueue);
#else
	virtual bool renderableQueued(Ogre::Renderable* rend, Ogre::uint8 groupID, Ogre::ushort priority, Ogre::Technique** ppTech);
#endif //OGRE_VERSION

};

class DOFManager : public Ogre::FrameListener, public MemoryAllocatedObject
{
public:
	DOFManager(Ogre::Root *mRoot, Ogre::Camera* camera, Ogre::SceneManager* sceneManager);

	void setEnabled(bool enabled);
	bool getEnabled();

	void setDebugEnabled(bool enabled);
	bool getDebugEnabled();

	// controls
	void toggleFocusMode();
	void zoomView(float delta);
	void setAperture(float delta);
	void moveFocus(float delta);

protected:
	Ogre::Root *mRoot;
	Ogre::SceneManager* mSceneManager;
	Ogre::Camera *mCamera;
	DepthOfFieldEffect* mDepthOfFieldEffect;
	Lens* mLens;
	bool debugEnabled;
	enum FocusMode {Auto, Manual, Pinhole};
	static const char* FOCUS_MODES[3];
	FocusMode mFocusMode;
	Ogre::OverlayElement* mFocalLengthText;
	Ogre::OverlayElement* mFStopKey;
	Ogre::OverlayElement* mFStopText;
	Ogre::OverlayElement* mFocusModeText;
	Ogre::OverlayElement* mFocalDistanceKey;
	Ogre::OverlayElement* mFocalDistanceText;

	virtual bool frameStarted(const Ogre::FrameEvent& evt);
	void updateOverlay();
};

#endif // __DepthOfFieldEffect_H__
