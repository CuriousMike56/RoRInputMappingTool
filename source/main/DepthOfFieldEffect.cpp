// "Depth of Field" demo for Ogre
// Copyright (C) 2006  Christian Lindequist Larsen
//
// This code is in the public domain. You may do whatever you want with it.

#include "DepthOfFieldEffect.h"
#include "Ogre.h"
#include "Lens.h"

using namespace Ogre;

const int DepthOfFieldEffect::BLUR_DIVISOR = 4;

DepthOfFieldEffect::DepthOfFieldEffect(Viewport* viewport)
	: mViewport(viewport)
	, mNearDepth(10.0)
	, mFocalDepth(100.0)
	, mFarDepth(190.0)
	, mFarBlurCutoff(1.0)
{
	createDepthRenderTexture();
	createCompositor();
	addCompositor();
}

DepthOfFieldEffect::~DepthOfFieldEffect()
{
	removeCompositor();
	destroyCompositor();
	destroyDepthRenderTexture();
}

void DepthOfFieldEffect::setFocalDepths(float nearDepth, float focalDepth, float farDepth)
{
	mNearDepth = nearDepth;
	mFocalDepth = focalDepth;
	mFarDepth = farDepth;
}

void DepthOfFieldEffect::setFarBlurCutoff(float cutoff)
{
	mFarBlurCutoff = cutoff;
}

bool DepthOfFieldEffect::getEnabled() const
{
	if(!mCompositor) return false;
	return mCompositor->getEnabled();
}

void DepthOfFieldEffect::setEnabled(bool value)
{
	if(mCompositor)
		mCompositor->setEnabled(value);
}

void DepthOfFieldEffect::createDepthRenderTexture()
{
	// Create the depth render texture
	TexturePtr depthTexture = TextureManager::getSingleton().createManual(
		"DoF_Depth", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
		TEX_TYPE_2D, mViewport->getActualWidth(), mViewport->getActualHeight(),
		0, PF_L8, TU_RENDERTARGET);

	// Get its render target and add a viewport to it
	RenderTexture* depthTarget = depthTexture->getBuffer()->getRenderTarget();
	Viewport* depthViewport = depthTarget->addViewport(mViewport->getCamera());

	// Register 'this' as a render target listener
	depthTarget->addListener(this);

	// Get the technique to use when rendering the depth render texture
	mDepthMaterial = MaterialManager::getSingleton().getByName("DoF_Depth");
	mDepthMaterial->load(); // needs to be loaded manually
	mDepthTechnique = mDepthMaterial->getBestTechnique();

	// Create a custom render queue invocation sequence for the depth render texture
    RenderQueueInvocationSequence* invocationSequence =
		Root::getSingleton().createRenderQueueInvocationSequence("DoF_Depth");

	// Add a render queue invocation to the sequence, and disable shadows for it
    RenderQueueInvocation* invocation = invocationSequence->add(RENDER_QUEUE_MAIN, "main");
    invocation->setSuppressShadows(true);

	// Set the render queue invocation sequence for the depth render texture viewport
    depthViewport->setRenderQueueInvocationSequenceName("DoF_Depth");

	// update debug texture
	MaterialPtr mat = MaterialManager::getSingleton().getByName("DoF_DepthDebug");
	mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName("DoF_Depth");

}

void DepthOfFieldEffect::destroyDepthRenderTexture()
{
	TextureManager::getSingleton().remove("DoF_Depth");
}

void DepthOfFieldEffect::createCompositor()
{
	// Create compositor
	CompositorPtr compositor = CompositorManager::getSingleton().create(
		"DoF_Compositor", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	{
		// Create technique
		CompositionTechnique* technique = compositor->createTechnique();
		{
			// Create texture definition 'scene'
			CompositionTechnique::TextureDefinition* definition = technique->createTextureDefinition("scene");
			definition->width = 0;  // target width
			definition->height = 0; // target height
#if OGRE_VERSION>0x010602
			definition->formatList.push_back(PF_X8R8G8B8);
#else
			definition->format = PF_X8R8G8B8;
#endif //OGRE_VERSION
		}
		{
			// Create texture definition 'blur'
			CompositionTechnique::TextureDefinition* definition = technique->createTextureDefinition("blur");
			definition->width = mViewport->getActualWidth() / BLUR_DIVISOR;
			definition->height = mViewport->getActualHeight() / BLUR_DIVISOR;
#if OGRE_VERSION>0x010602
			definition->formatList.push_back(PF_X8R8G8B8);
#else
			definition->format = PF_X8R8G8B8;
#endif //OGRE_VERSION
		}
		{
			// Create target pass 'scene'
			CompositionTargetPass* target = technique->createTargetPass();
			target->setOutputName("scene");
			target->setInputMode(CompositionTargetPass::IM_PREVIOUS);
		}
		{
			// Create target pass 'blur'
			CompositionTargetPass* target = technique->createTargetPass();
			target->setOutputName("blur");
			{
				// Create a composition pass
				CompositionPass* pass = target->createPass();
				pass->setIdentifier(BlurPass);
				pass->setMaterialName("DoF_Gaussian3x3");
				pass->setInput(0, "scene");
			}
		}
		{
			// Build the output target pass
			CompositionTargetPass* output = technique->getOutputTargetPass();
			{
				// Create a composition pass
				CompositionPass* pass = output->createPass();
				pass->setIdentifier(OutputPass);
				pass->setMaterialName("DoF_DepthOfField");
				pass->setInput(0, "scene");
				//pass->setInput(1, "depth"); // render texture
				pass->setInput(2, "blur");
			}
		}
	}
}

void DepthOfFieldEffect::destroyCompositor()
{
	CompositorManager::getSingleton().remove("DoF_Compositor");
}

void DepthOfFieldEffect::addCompositor()
{
	// Add the compositor to the viewport
	mCompositor = CompositorManager::getSingleton().addCompositor(
		mViewport, "DoF_Compositor_test");

	//TODO: throw error and exit if not found
	// Register 'this' as a listener
	if(mCompositor)
		mCompositor->addListener(this);
}

void DepthOfFieldEffect::removeCompositor()
{
	CompositorManager::getSingleton().removeCompositor(mViewport, "DoF_Compositor_test");
}

void DepthOfFieldEffect::notifyMaterialSetup(unsigned int passId, MaterialPtr& material)
{
	switch (passId)
	{
	case BlurPass:
		{
			//float pixelSize[2] = {
			//	1.0f / (mViewport->getActualWidth() / BLUR_DIVISOR),
			//	1.0f / (mViewport->getActualHeight() / BLUR_DIVISOR)};

			// Adjust fragment program parameters
			Ogre::Vector3 pixelSize = Ogre::Vector3(1.0f / (mViewport->getActualWidth() / BLUR_DIVISOR),1.0f / (mViewport->getActualHeight() / BLUR_DIVISOR), 1.0f);
			GpuProgramParametersSharedPtr fragParams = material->getBestTechnique()->getPass(0)->getFragmentProgramParameters();
			fragParams->setNamedConstant("pixelSize", pixelSize);

			break;
		}

	case OutputPass:
		{
			float pixelSizeScene[2] = {
				1.0f / mViewport->getActualWidth(),
				1.0f / mViewport->getActualHeight()};

			float pixelSizeBlur[2] = {
				1.0f / (mViewport->getActualWidth() / BLUR_DIVISOR),
				1.0f / (mViewport->getActualHeight() / BLUR_DIVISOR)};

			// Adjust fragment program parameters
			GpuProgramParametersSharedPtr fragParams =
				material->getBestTechnique()->getPass(0)->getFragmentProgramParameters();
			fragParams->setNamedConstant("pixelSizeScene", pixelSizeScene,
				sizeof(pixelSizeScene)/sizeof(pixelSizeScene[0]));
			fragParams->setNamedConstant("pixelSizeBlur", pixelSizeBlur,
				sizeof(pixelSizeBlur)/sizeof(pixelSizeBlur[0]));

			break;
		}
	}
}

void DepthOfFieldEffect::preViewportUpdate(const Ogre::RenderTargetViewportEvent& evt)
{
	float dofParams[4] = {mNearDepth, mFocalDepth, mFarDepth, mFarBlurCutoff};

	// Adjust fragment program parameters for depth pass
	GpuProgramParametersSharedPtr fragParams =
		mDepthTechnique->getPass(0)->getFragmentProgramParameters();
	fragParams->setNamedConstant("dofParams", dofParams,
		sizeof(dofParams)/sizeof(dofParams[0]));

	// Add 'this' as a RenderableListener to replace the technique for all renderables
	RenderQueue* queue = evt.source->getCamera()->getSceneManager()->getRenderQueue();
	queue->setRenderableListener(this);
}

void DepthOfFieldEffect::postViewportUpdate(const Ogre::RenderTargetViewportEvent& evt)
{
	// Reset the RenderableListener
	RenderQueue* queue = evt.source->getCamera()->getSceneManager()->getRenderQueue();
	queue->setRenderableListener(0);
}

#if OGRE_VERSION>0x010602
bool DepthOfFieldEffect::renderableQueued(Renderable* rend, Ogre::uint8 groupID, Ogre::ushort priority, Technique** ppTech, Ogre::RenderQueue* pQueue)
#else
bool DepthOfFieldEffect::renderableQueued(Renderable* rend, Ogre::uint8 groupID, Ogre::ushort priority, Technique** ppTech)
#endif //OGRE_VERSION
{
	// Replace the technique of all renderables
	*ppTech = mDepthTechnique;
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////
// DOFManager below
const char* DOFManager::FOCUS_MODES[3] = {"auto", "manual", "pinhole"};

DOFManager::DOFManager(Ogre::Root *mRoot, Camera* camera, SceneManager* sceneManager) : mSceneManager(sceneManager), mFocusMode(Manual), mRoot(mRoot), debugEnabled(false), mCamera(camera)
{
	mDepthOfFieldEffect = new DepthOfFieldEffect(mCamera->getViewport());
	mLens = new Lens(mCamera->getFOVy(), 1.5);
	mLens->setFocalDistance(171.5);
	//mLens->setFStop(3);
	mDepthOfFieldEffect->setEnabled(false);
	mRoot->addFrameListener(this);

	// Do not Show overlays - only crash if the overlays are missing ;)
	OverlayManager::getSingleton().getByName("DoF_SettingsOverlay")->hide();
	OverlayManager::getSingleton().getByName("DoF_FocusAreaOverlay")->hide();
	OverlayManager::getSingleton().getByName("DoF_DepthDebugOverlay")->hide();

	// Initialise and show debug depth overlay
	MaterialPtr material = MaterialManager::getSingleton().getByName("DoF_DepthDebug");
	material->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName("DoF_Depth");
	
	// debug overlays
	OverlayManager& om = OverlayManager::getSingleton();
	mFocalLengthText = om.getOverlayElement("DoF_FocalLength");
	mFStopKey = om.getOverlayElement("DoF_FStopKey");
	mFStopText = om.getOverlayElement("DoF_FStop");
	mFocusModeText = om.getOverlayElement("DoF_FocusMode");
	mFocalDistanceKey = om.getOverlayElement("DoF_FocalDistanceKey");
	mFocalDistanceText = om.getOverlayElement("DoF_FocalDistance");
}

void DOFManager::setEnabled(bool enabled)
{
	mDepthOfFieldEffect->setEnabled(enabled);
	/*
	// crashes for some reason
	if(enabled && !mDepthOfFieldEffect->getEnabled())
	{
		// turn on
		mDepthOfFieldEffect->setEnabled(true);
		mRoot->addFrameListener(this);
	} else if(!enabled && mDepthOfFieldEffect->getEnabled())
	{
		// turn off
		mDepthOfFieldEffect->setEnabled(false);
		mRoot->removeFrameListener(this);
	}
	*/
}

void DOFManager::setDebugEnabled(bool enabled)
{
	debugEnabled = enabled;
	if(debugEnabled)
	{
		OverlayManager::getSingleton().getByName("DoF_SettingsOverlay")->show();
		OverlayManager::getSingleton().getByName("DoF_FocusAreaOverlay")->show();
		OverlayManager::getSingleton().getByName("DoF_DepthDebugOverlay")->show();
	} else
	{
		OverlayManager::getSingleton().getByName("DoF_SettingsOverlay")->hide();
		OverlayManager::getSingleton().getByName("DoF_FocusAreaOverlay")->hide();
		OverlayManager::getSingleton().getByName("DoF_DepthDebugOverlay")->hide();
	}
}

bool DOFManager::getDebugEnabled()
{
	return debugEnabled;
}

bool DOFManager::getEnabled()
{
	return mDepthOfFieldEffect->getEnabled();
}

void DOFManager::toggleFocusMode()
{
	mFocusMode = static_cast<FocusMode>((mFocusMode + 1) % 3);
}

void DOFManager::zoomView(float delta)
{
	Real fieldOfView = mLens->getFieldOfView().valueRadians();
	fieldOfView += delta;
	fieldOfView = std::max<Real>(0.1, std::min<Real>(fieldOfView, 2.0));
	mLens->setFieldOfView(Radian(fieldOfView));
	mCamera->setFOVy(Radian(fieldOfView));
}

void DOFManager::setAperture(float delta)
{
	if (mFocusMode == Pinhole)
		return;
	Real fStop = mLens->getFStop();
	fStop += delta;
	fStop = std::max<Real>(0.5, std::min<Real>(fStop, 12.0));
	mLens->setFStop(fStop);
}

void DOFManager::moveFocus(float delta)
{
	mLens->setFocalDistance(mLens->getFocalDistance() + delta);
}

bool DOFManager::frameStarted(const FrameEvent& evt)
{
	// Focusing
	switch (mFocusMode)
	{
		case Auto:
		{
			// TODO: Replace with accurate ray/triangle collision detection

			// TODO: Continous AF / triggered

			Real currentFocalDistance = mLens->getFocalDistance();
			Real targetFocalDistance = currentFocalDistance;

			// Ryan Booker's (eyevee99) ray scene query auto focus
			Ray focusRay;
			focusRay.setOrigin(mCamera->getDerivedPosition());
			focusRay.setDirection(mCamera->getDerivedDirection());

			RaySceneQuery* query = 0;
			query = mSceneManager->createRayQuery(focusRay);
			query->setRay(focusRay);
			RaySceneQueryResult& queryResult = query->execute();
			RaySceneQueryResult::iterator i = queryResult.begin();
			if (i != queryResult.end())
				targetFocalDistance = i->distance;
			//else
			//	mLens->setFocalDistance(Math::POS_INFINITY);
			mSceneManager->destroyQuery(query);

			// Slowly adjust the focal distance (emulate auto focus motor)
			if (currentFocalDistance < targetFocalDistance)
			{
				mLens->setFocalDistance(
					std::min<Real>(currentFocalDistance + 240.0 * evt.timeSinceLastFrame, targetFocalDistance));
			}
			else if (currentFocalDistance > targetFocalDistance)
			{
				mLens->setFocalDistance(
					std::max<Real>(currentFocalDistance - 240.0 * evt.timeSinceLastFrame, targetFocalDistance));
			}

			break;
		}

		case Manual:
			//we set the values elsewhere
			break;
	}

	// Update Depth of Field effect
	if (mFocusMode != Pinhole)
	{
		mDepthOfFieldEffect->setEnabled(true);

		// Calculate and set depth of field using lens
		float nearDepth, focalDepth, farDepth;
		mLens->recalculateDepthOfField(nearDepth, focalDepth, farDepth);
		mDepthOfFieldEffect->setFocalDepths(nearDepth, focalDepth, farDepth);
	}
	else
	{
		mDepthOfFieldEffect->setEnabled(false);
	}

	// Update overlay
	updateOverlay();

	return true;
}

void DOFManager::updateOverlay()
{
	if(!debugEnabled) return;

	mFocalLengthText->setCaption(
		StringConverter::toString(mLens->getFocalLength(), 1, 0, 32, std::ios::fixed) + " (" +
		StringConverter::toString(mLens->getFieldOfView().valueDegrees(), 1, 0, 32, std::ios::fixed) + ")");
	mFocusModeText->setCaption(FOCUS_MODES[mFocusMode]);

	if (mFocusMode != Pinhole)
	{
		mFStopKey->setCaption("[1,2]");
		mFStopText->setCaption(StringConverter::toString(mLens->getFStop(), 1, 0, 32, std::ios::fixed));
		if (mFocusMode == Manual)
			mFocalDistanceKey->setCaption("[Wheel]");
		else
			mFocalDistanceKey->setCaption("[]");
		mFocalDistanceText->setCaption(StringConverter::toString(mLens->getFocalDistance(), 1, 0, 32, std::ios::fixed));
	}
	else
	{
		mFStopKey->setCaption("[]");
		mFStopText->setCaption("N/A");
		mFocalDistanceKey->setCaption("[]");
		mFocalDistanceText->setCaption("N/A");
	}
}
