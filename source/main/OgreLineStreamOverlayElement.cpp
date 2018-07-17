/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright  2000-2005 The OGRE Team
Also see acknowledgements in Readme.html

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple
Place - Suite 330, Boston, MA 02111-1307, USA, or go to
http://www.gnu.org/copyleft/lesser.txt.

You may alternatively use this source under the terms of a specific version of
the OGRE Unrestricted License provided you have obtained such a license from
Torus Knot Software Ltd.
-----------------------------------------------------------------------------
*/
#include "OgreStableHeaders.h"

#include "OgreLineStreamOverlayElement.h"
#include "OgreOverlayManager.h"
#include "OgreLogManager.h"
#include "OgreMaterial.h"
#include "OgreTechnique.h"
#include "OgrePass.h"
#include "OgreStringConverter.h"
#include "OgreHardwareBufferManager.h"
#include "OgreRoot.h"
#include "OgreRenderSystem.h"

namespace Ogre {
    //---------------------------------------------------------------------
    String LineStreamOverlayElement::msTypeName = "LineStream";
    //---------------------------------------------------------------------
    LineStreamOverlayElement::LineStreamOverlayElement(const String& name)
		: OverlayContainer(name),
		mNumberOfTraces(1),
		mNumberOfSamplesForTrace(100),
		mPosInStream(0),
		myTitle(" "),
		titleColour(ColourValue::Black),
		mDataChanged(false),
		moveMode(1),
		legendTop(0),
		legendBottom(0)
    {
		// Setup render op in advance
		mRenderOp.vertexData = new VertexData();
		mRenderOp.vertexData->vertexCount = 0;
	}
    //---------------------------------------------------------------------
    LineStreamOverlayElement::~LineStreamOverlayElement()
    {
        delete mRenderOp.vertexData;
    }
    //---------------------------------------------------------------------
    void LineStreamOverlayElement::initialise(void)
    {
		bool init = !mInitialised;

		OverlayContainer::initialise();
		if (init)
		{
			mInitialised = true;
		}
    }

	//---------------------------------------------------------------------
	void LineStreamOverlayElement::defaultStyle(void)
	{
    }

    //---------------------------------------------------------------------
    const String& LineStreamOverlayElement::getTypeName(void) const
    {
        return msTypeName;
    }
    //---------------------------------------------------------------------
    void LineStreamOverlayElement::getRenderOperation(RenderOperation& op)
    {
		updateVtxBuffer();
        op = mRenderOp;
    }
    //---------------------------------------------------------------------
    void LineStreamOverlayElement::setMaterialName(const String& matName)
    {
        OverlayContainer::setMaterialName(matName);
    }
    //---------------------------------------------------------------------
    void LineStreamOverlayElement::_updateRenderQueue(RenderQueue* queue)
    {
        if (mVisible)
        {

            if (!mpMaterial.isNull())
            {
                OverlayContainer::_updateRenderQueue(queue);
            }

            // Also add children
            ChildIterator it = getChildIterator();
            while (it.hasMoreElements())
            {
                // Give children ZOrder 1 higher than this
                it.getNext()->_updateRenderQueue(queue);
            }
        }
    }
    //---------------------------------------------------------------------
    void LineStreamOverlayElement::updatePositionGeometry(void)
    {
		mDataChanged = true;
		updateVtxBuffer();
    }
	//---------------------------------------------------------------------
	void LineStreamOverlayElement::updateTextureGeometry( void )
	{
	}
	//---------------------------------------------------------------------
	uint32 LineStreamOverlayElement::getNumberOfTraces() const
	{
		return mNumberOfTraces;
	}
	//---------------------------------------------------------------------
	void LineStreamOverlayElement::setNumberOfTraces( const uint32 val )
	{
		mNumberOfTraces = val;
	}
	//---------------------------------------------------------------------
	uint32 LineStreamOverlayElement::getNumberOfSamplesForTrace() const
	{
		return mNumberOfSamplesForTrace;
	}
	//---------------------------------------------------------------------
	void LineStreamOverlayElement::setNumberOfSamplesForTrace( const uint32 val )
	{
		mNumberOfSamplesForTrace = val;
	}
	//---------------------------------------------------------------------
	void LineStreamOverlayElement::createVertexBuffer()
	{

		uint32 numberOfVertexs = mNumberOfTraces * mNumberOfSamplesForTrace * 2;
		if (numberOfVertexs != mRenderOp.vertexData->vertexCount)
		{
			// Vertex declaration: 1 position, add texcoords later depending on #layers
			// Create as separate buffers so we can lock & discard separately
			VertexDeclaration* decl = mRenderOp.vertexData->vertexDeclaration;
			size_t offset = 0;
			offset += decl->addElement(0, 0, VET_FLOAT3, VES_POSITION).getSize();
			offset += decl->addElement(0, offset, VET_COLOUR_ARGB, VES_DIFFUSE).getSize();

			// Basic vertex data
			mRenderOp.vertexData->vertexStart = 0;
			// No indexes & issue as a strip
			mRenderOp.useIndexes = false;
			mRenderOp.operationType = RenderOperation::OT_LINE_LIST;


			mTraceSamples.resize(numberOfVertexs / 2);
			mTraceInfo.resize(mNumberOfTraces);

			mRenderOp.vertexData->vertexCount = numberOfVertexs;

			// Vertex buffer #1
			mCurrentVtxBuffer =
				HardwareBufferManager::getSingleton().createVertexBuffer(
				decl->getVertexSize(0), mRenderOp.vertexData->vertexCount,
				HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY// mostly static except during resizing
				);
			// Bind buffer
			mRenderOp.vertexData->vertexBufferBinding->setBinding(0, mCurrentVtxBuffer);

			// clear buffer
			memset(mCurrentVtxBuffer->lock(HardwareBuffer::HBL_DISCARD), 0, decl->getVertexSize(0) * mRenderOp.vertexData->vertexCount);
			memset(&mTraceSamples[0], 0, sizeof(Real) * mTraceSamples.size());
			mCurrentVtxBuffer->unlock();

			mPosInStream = 0;
		}
	}
	void LineStreamOverlayElement::setMoveMode(int value)
	{
		moveMode=value;
	}
	int LineStreamOverlayElement::getMoveMode()
	{
		return moveMode;
	}

	//---------------------------------------------------------------------
	void LineStreamOverlayElement::updateVtxBuffer()
	{

		if (!mDataChanged)
		{
			return;
		}
		mDataChanged = false;

		struct fTraceVertex
		{
			Vector3 pos;
			ARGB color;
		};





		// Use the furthest away depth value, since materials should have depth-check off
		// This initialised the depth buffer for any 3D objects in front
		Real zValue = Root::getSingleton().getRenderSystem()->getMaximumDepthInputValue();

		//find max value
		Real maxValue = 0.0f;
		for (uint32 trace = 0 ; trace < mNumberOfTraces ; trace++)
		{
			for (uint32 sample = 0 ; sample < mNumberOfSamplesForTrace ; sample++)
			{
				Real curSample = mTraceSamples[trace * mNumberOfSamplesForTrace + sample];
				if (maxValue < curSample)
				{
					maxValue = curSample;
				}

			}
		}

		if (fabs(maxValue) < 0.000001f)
		{
			return;
		}

		Real left, right, top, bottom, gHeight;
		left = _getDerivedLeft() * 2 - 1;
		right = left + (mWidth * 2);
		gHeight = mHeight * 0.6f;
		top = -((_getDerivedTop() * 2) - 1) - mHeight * 0.4f; // this creates a border at the top and bottom
		bottom =  top -  (gHeight * 2);

		fTraceVertex * currentVtxBufferData =
			static_cast<fTraceVertex *>(mCurrentVtxBuffer->lock(HardwareBuffer::HBL_DISCARD));

		// write vertexes
		for (uint32 trace = 0 ; trace < mNumberOfTraces ; trace++)
		{
			for (uint32 sample = 1 ; sample < mNumberOfSamplesForTrace ; sample++)
			{
				fTraceVertex & lineStartVtx = currentVtxBufferData[(trace * mNumberOfSamplesForTrace + sample) * 2];
				lineStartVtx.pos.x = left + (mWidth * 2) * ( (Real)sample  / ((Real)mNumberOfSamplesForTrace + 1) );
				Real startSampleQunt = ( mTraceSamples[trace * mNumberOfSamplesForTrace + (sample + mPosInStream) % mNumberOfSamplesForTrace] / maxValue );
				lineStartVtx.pos.y = bottom + (gHeight * 2) * startSampleQunt;
				lineStartVtx.pos.z = zValue;


				fTraceVertex & lineEndVtx = currentVtxBufferData[(trace * mNumberOfSamplesForTrace + sample) * 2 + 1];
				lineEndVtx.pos.x = left + (mWidth * 2) * (Real)(sample + 1) / (Real)(mNumberOfSamplesForTrace + 1) ;
				Real EndSampleQunt = ( mTraceSamples[trace * mNumberOfSamplesForTrace + (sample + mPosInStream + 1) % mNumberOfSamplesForTrace] / maxValue );
				lineEndVtx.pos.y = bottom +(gHeight * 2) * EndSampleQunt;
				lineEndVtx.pos.z = zValue;

				lineStartVtx.color = mTraceInfo[trace].colour.getAsARGB();
				lineEndVtx.color = mTraceInfo[trace].colour.getAsARGB();
			}

		}
		mCurrentVtxBuffer->unlock();

		if(legendTop) legendTop->setCaption(StringConverter::toString((int)maxValue));

		if(moveMode==1)
			mPosInStream++;
	}
	//---------------------------------------------------------------------
	void LineStreamOverlayElement::setExactValue( const uint32 traceIndex, const uint32 fieldIndex, const Real traceValue )
	{
		mTraceSamples[traceIndex * mNumberOfSamplesForTrace + fieldIndex % mNumberOfSamplesForTrace] = traceValue;
		mDataChanged = true;
	}

	//---------------------------------------------------------------------
	void LineStreamOverlayElement::moveForward()
	{
		mPosInStream++;
	}

	//---------------------------------------------------------------------
	void LineStreamOverlayElement::setTraceValue( const uint32 traceIndex, const Real traceValue )
	{
		mTraceSamples[traceIndex * mNumberOfSamplesForTrace + mPosInStream % mNumberOfSamplesForTrace] = traceValue;
		if(moveMode==2)
			mPosInStream++;
		mDataChanged = true;

	}
	//---------------------------------------------------------------------
	void LineStreamOverlayElement::setTitle(ColourValue c, String title)
	{
		myTitle = title;
		titleColour = c;
	}
	//---------------------------------------------------------------------
	void LineStreamOverlayElement::setTraceInfo( const uint32 traceIndex, const ColourValue & traceColour, const String &name )
	{
		String elName = "StreamTraceInfo"+mName+StringConverter::toString(traceIndex);
		bool existing = false;
		try
		{
			OverlayManager::getSingleton().getOverlayElement(elName);
			existing = true;
		} catch(...)
		{
			existing = false;
		}
		if(!existing)
		{
			// create
			mTraceInfo[traceIndex].legendText = (TextAreaOverlayElement*)OverlayManager::getSingleton().createOverlayElement("TextArea", elName);
			mTraceInfo[traceIndex].legendText->setMetricsMode(GMM_RELATIVE);
			mTraceInfo[traceIndex].legendText->setPosition(0.04f + (this->getWidth()/(float)mNumberOfTraces) * traceIndex, 0);
			mTraceInfo[traceIndex].legendText->setFontName("VeraMono");
			mTraceInfo[traceIndex].legendText->setDimensions(this->getWidth(), 0.02f);
			mTraceInfo[traceIndex].legendText->setCharHeight(0.02f);
			mTraceInfo[traceIndex].legendText->show();
			this->addChild(mTraceInfo[traceIndex].legendText);
		}
		mTraceInfo[traceIndex].legendText->setCaption(name);
		mTraceInfo[traceIndex].legendText->setColour(traceColour);
		mTraceInfo[traceIndex].colour = traceColour;
		mTraceInfo[traceIndex].name = name;

		if(!legendTop && !legendBottom)
		{
			legendTop = (TextAreaOverlayElement*)OverlayManager::getSingleton().createOverlayElement("TextArea", elName+"Top");
			legendTop->setMetricsMode(GMM_RELATIVE);
			legendTop->setPosition(0, 0);
			legendTop->setFontName("VeraMono");
			legendTop->setDimensions(0.06f, 0.02f);
			legendTop->setColour(ColourValue::Black);
			legendTop->setCharHeight(0.02f);
			legendTop->show();
			legendTop->setCaption("0");
			this->addChild(legendTop);

			// todo: fix top of the bottom element!
			legendBottom = (TextAreaOverlayElement*)OverlayManager::getSingleton().createOverlayElement("TextArea", elName+"Bottom");
			legendBottom->setMetricsMode(GMM_RELATIVE);
			legendBottom->setPosition(0, this->getHeight()-0.02f);
			legendBottom->setFontName("VeraMono");
			legendBottom->setDimensions(0.06f, 0.02f);
			legendBottom->setColour(ColourValue::Black);
			legendBottom->setCharHeight(0.02f);
			legendBottom->setCaption("0");
			legendBottom->show();
			this->addChild(legendBottom);

			// todo: fix top of the bottom element!
			title = (TextAreaOverlayElement*)OverlayManager::getSingleton().createOverlayElement("TextArea", elName+"Title");
			title->setMetricsMode(GMM_RELATIVE);
			title->setPosition(this->getWidth()*0.4f, this->getHeight()-0.02f);
			title->setFontName("VeraMono");
			title->setDimensions(0.06f, 0.02f);
			title->setColour(titleColour);
			title->setCharHeight(0.02f);
			title->setCaption(myTitle);
			title->show();
			this->addChild(title);
		}

	}
	//---------------------------------------------------------------------
	//---------------------------------------------------------------------
	//---------------------------------------------------------------------
	//---------------------------------------------------------------------
	const String& LineStreamOverlayElementFactory::getTypeName( void ) const
	{
		static String name = "LineStream";
		return name;
	}
	//---------------------------------------------------------------------
	OverlayElement* LineStreamOverlayElementFactory::createOverlayElement( const String& instanceName )
	{
		return new LineStreamOverlayElement(instanceName);
	}
	//---------------------------------------------------------------------
	void LineStreamOverlayElementFactory::destroyOverlayElement( OverlayElement* pElement )
	{
		delete pElement;
	}
}



