/**
 @file BeamSlideNode.cpp
 @author Christopher Ritchey
 @date 10/30/2009
 
This source file is part of Rigs of Rods
Copyright 2009 Christopher Ritchey

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

#include "Beam.h"
#include "Ogre.h"
#include <vector>

// Rail Group Parsing ********************************************************//
bool Beam::parseRailGroupLine(const Ogre::String& line)
{

	Ogre::vector<Ogre::String>::type options = Ogre::StringUtil::split(line, ",");
    
	if( options.size() < 3)
	{
		LogManager::getSingleton().logMessage("RAILGROUP: not enough nodes: " + String(line));
		return false;
	}
    
	unsigned int railID = StringConverter::parseInt(options[0]);
	options.erase( options.begin() );
    
	Rail* newRail = parseRailString( options );
	if( !newRail ) return false;
	RailGroup* newGroup = new RailGroup( newRail, railID );
        
	mRailGroups.push_back(newGroup); // keep track of all allocated Rails
	return true;
}
//****************************************************************************//

// Slide Node Methods ********************************************************//
bool Beam::parseSlideNodeLine(const Ogre::String& line)
{
	// add spaces as a separator to remove them 
	Ogre::vector<Ogre::String>::type options = Ogre::StringUtil::split(line, ", ");
    
	if( options.size() < 2)
	{
		LogManager::getSingleton().logMessage("SLIDENODE: not enough options provided: " + String(line));
		return false;
	}

	
	// find node ///////////////////////////////////////////////////////////////
	int nodeid = StringConverter::parseReal( options.front() );
	node_t* node = getNode( nodeid );
	options.erase(options.begin()); // remove front element

	if( !node )
	{
		LogManager::getSingleton().logMessage("SLIDENODE: invalid node id: " +
				StringConverter::toString( nodeid ) );
		return false;
	}
    
	RailGroup* rgGroup = NULL;
	// options,
	//if ends are open or closed ends, ie can node slide right off the end of a Rail
	// attachment constraints, none, self, foreign, all 

	LogManager::getSingleton().logMessage("SLIDENODE: making new SlideNode");
	SlideNode newSlideNode = SlideNode(node, NULL);
	//unsigned int quantity = 1;
	
	// check if tolerance value was provided ///////////////////////////////////
	for( bool moreOptions = true; moreOptions && options.size() > 0; )
	{
		std::pair<char, Ogre::String> option(options.back()[0], 
				Ogre::String( options.back().begin() + 1, options.back().end() ) );
		
		switch( option.first )
		{
		// tolerance in meters, default 0.0f
		case 't': case 'T':
		{			
			LogManager::getSingleton().logMessage("SLIDENODE: setting tolerance to : " +
					option.second );
			newSlideNode.setThreshold( StringConverter::parseReal( option.second ) );
		} 
		break;
		
		// specify a rail group
		case 'g': case 'G':
		{
			
			rgGroup = getRailFromID( StringConverter::parseReal( option.second ) );
			if( !rgGroup )
			{
				LogManager::getSingleton().logMessage("RAILGROUP: warning could not find "
						"a Railgroup with corresponding ID: " + 
						option.second + ". Will check for anonymous rail");
			}
			else
				newSlideNode.setDefaultRail( rgGroup );
			
			break;
		}
		
		// Spring Rate in N/m, default 9000000
		case 's': case 'S':
			newSlideNode.setSpringRate( StringConverter::parseReal( option.second ) );
			break;
		
		// Force when beam breaks in N, default 1000000
		case 'b': case 'B':
			newSlideNode.setBreakForce( StringConverter::parseReal( option.second ) );
			break;
		// Attachment Rate in seconds, default disabled
		case 'r': case 'R':
			newSlideNode.setAttachmentRate( StringConverter::parseReal( option.second ) );
			break;

		// Maximum attachment Distance in meters, default infinity
		case 'd': case 'D':
			newSlideNode.setAttachmentDistance( StringConverter::parseReal( option.second ) ); 
			break;
		
		// Attachment constraints, bit Flags, default none
		case 'c': case 'C':
		{
			switch( option.second[0] )
			{
			case 'a': newSlideNode.setAttachRule( ATTACH_ALL ); break;
			case 'f': newSlideNode.setAttachRule( ATTACH_FOREIGN ); break;
			case 's': newSlideNode.setAttachRule( ATTACH_SELF ); break;
			case 'n': newSlideNode.setAttachRule( ATTACH_NONE ); break;
			}
		}
		break;
		
		// quantity, how many rails this node can slide on
		case 'q': case'Q': break;
			
		
		// no more options
		default: moreOptions = false;
		
		}
		
		if( moreOptions ) options.pop_back();
	}

	// find beam ///////////////////////////////////////////////////////////////    
	// rail builder allocates the memory for each rail, it will not free it,
	if( !rgGroup && options.size() > 0 )
	{
		Rail* newRail = parseRailString( options );
		rgGroup = (newRail) ? new RailGroup(newRail) : NULL;

		newSlideNode.setDefaultRail( rgGroup );
		mRailGroups.push_back(rgGroup); // keep track of all allocated Rails
	}

	LogManager::getSingleton().logMessage("SLIDENODE: Adding Slide Rail");
	mSlideNodes.push_back( newSlideNode );
	return true;
}

// ug... BAD PERFORMNCE, BAD!! 
void Beam::toggleSlideNodeLock( Beam** trucks, int trucksnum, unsigned int curTruck )
{
	
	// for every slide node on this truck
	for(std::vector< SlideNode >::iterator itNode = mSlideNodes.begin(); itNode != mSlideNodes.end(); itNode++)
	{
		std::pair<RailGroup*, Ogre::Real> closest((RailGroup*)NULL, std::numeric_limits<Ogre::Real>::infinity());
		std::pair<RailGroup*, Ogre::Real> current((RailGroup*)NULL, std::numeric_limits<Ogre::Real>::infinity());
		
		// if neither foreign, nor self attach is set then we cannot change the
		// Rail attachments
		if( !itNode->getAttachRule( ATTACH_ALL ) ) continue;
		if( SlideNodesLocked )
		{
			itNode->attachToRail( NULL );
			continue;
		}
		
		// check all the slide rail on all the other trucks :(
		for( unsigned int i = 0; i < (unsigned int) trucksnum; ++i)
		{
			// make sure this truck is allowed
			if( !( ( curTruck != i && itNode->getAttachRule(ATTACH_FOREIGN) ) ||
				  ( curTruck == i && itNode->getAttachRule(ATTACH_SELF) ) ) )
				continue;
			
			current = getClosestRailOnTruck( trucks[i], (*itNode) );
			if( current.second < closest.second ) closest = current;
			
		} // this many
		
		itNode->attachToRail(closest.first);
	} // nests
	
	// flip boolean
	SlideNodesLocked = !SlideNodesLocked;
} // is ugly....

std::pair<RailGroup*, Ogre::Real> Beam::getClosestRailOnTruck( Beam* truck, const SlideNode& node)
{
	std::pair<RailGroup*, Ogre::Real> closest((RailGroup*)NULL, std::numeric_limits<Ogre::Real>::infinity());
	Rail* curRail = NULL;
	Ogre::Real lenToCurRail = std::numeric_limits<Ogre::Real>::infinity();

	for(std::vector< RailGroup* >::iterator itGroup = truck->mRailGroups.begin();
			itGroup != truck->mRailGroups.end();
			itGroup++)
	{
		// find the rail closest to the Node
		curRail = SlideNode::getClosestRailAll( (*itGroup), node.getNodePosition() );
		lenToCurRail = node.getLenTo( curRail );
#if 0
		// if this rail closer than the previous one keep, also check that this
		// slide node is not already attached to this rail, now how to do that
		// without looping through the entire slide node array AGAIN...
		// Only for use with a single slide node attaching to multiple rails,
		// which currently is not implemented
		// oh well git'r done... :P
		for(std::vector< SlideNode >::iterator itNode = mSlideNodes.begin(); itNode != mSlideNodes.end(); itNode++)
		{
			// check if node is already hooked up to 
			if( node.getNodeID() == itNode->getNodeID() &&
				node.getRailID() == (*itGroup)->getID() )
				continue;
		}
#endif
		if( lenToCurRail < node.getAttachmentDistance() && lenToCurRail < closest.second )
		{
			closest.first = (*itGroup);
			closest.second = lenToCurRail;
		}
	}
	
	return closest;
}

// SlideNode Utility functions /////////////////////////////////////////////////

void Beam::updateSlideNodeForces(const Ogre::Real dt)
{
	for(std::vector<SlideNode>::iterator it = mSlideNodes.begin(); it != mSlideNodes.end(); ++it)
	{
		it->UpdatePosition();
		it->UpdateForces(dt);
	}
}

void Beam::resetSlideNodePositions()
{
	if(mSlideNodes.empty()) return;
	for(std::vector<SlideNode>::iterator it = mSlideNodes.begin(); it != mSlideNodes.end(); ++it)
	{
		it->ResetPositions();
	}	
}

void Beam::resetSlideNodes()
{
	for(std::vector<SlideNode>::iterator it = mSlideNodes.begin(); it != mSlideNodes.end(); ++it)
	{
		it->reset();
	}
	
}
void Beam::updateSlideNodePositions()
{
	for(std::vector<SlideNode>::iterator it = mSlideNodes.begin(); it != mSlideNodes.end(); ++it)
	{
		it->UpdatePosition();
	}
}

RailGroup* Beam::getRailFromID(unsigned int id)
{
	for(std::vector< RailGroup* >::iterator it = mRailGroups.begin(); it != mRailGroups.end(); it++)
	{
		if( (*it)->getID() == id ) return (*it);
	}
	// Rail not found
	return NULL;
}

Rail* Beam::parseRailString( const Ogre::vector<Ogre::String>::type & railStrings)
{
	std::vector<int> nodeids;
    
	// convert all strings to integers
	for( unsigned int i = 0; i < railStrings.size(); ++i)
	{
		// check if value is a node range
		if( railStrings[i].find("-") != Ogre::String::npos )	
		{
			size_t pos = railStrings[i].find("-");
    		
			// from start to '-'
			const int start = StringConverter::parseInt(railStrings[i].substr(0, pos) );
			// from character after '-' to end 
			const int end = StringConverter::parseInt( railStrings[i].substr(++pos) );
    		 
			// if start is a lower value than the end, we need to start at the
			// higher value and decrement to the lower value. to avoid duplicate
			// logic (ie one loop for positive and one loop for negative
			// compare the negative values, and add a negative 1 to the counter.
    		
			int inc = ( start < end ? +1 : -1 );
    		
			// multiply by the increment value (1 or -1), if we are decrmenting
			// compare the negative value to essentially flip the <= operator to
			// a >= operator ie
			//assert( !!( start <= end ) == !!(-start >= -end );
    		
			for(int j = start ; inc * j <= inc * end; j += inc )
			{
				// if end is lower than start then 
				nodeids.push_back( j );
			}
    		
			continue;		
		}
    	
		nodeids.push_back(StringConverter::parseInt(railStrings[i]));
	}
    
	return getRails( nodeids );
}

Rail* Beam::getRails(const std::vector<int>& nodeids)
{
	// find beam ///////////////////////////////////////////////////////////////    
	// rail builder allocates the memory for each rail, it will not free it
	RailBuilder builder;
    
	if( nodeids.front() == nodeids.back() )
	{
		LogManager::getSingleton().logMessage("RAIL: Looping SlideRail");
			builder.loopRail();
	} 
    
	for( unsigned int i = 0; i < (nodeids.size() - 1); ++i)
	{
		beam_t* beam = getBeam( nodeids[i], nodeids[i+1] );
    
		// Verify Beam
		if( !beam )
		{
			LogManager::getSingleton().logMessage("RAIL: invalid beam: " +
					StringConverter::toString(nodeids[i]) + ", " +
					StringConverter::toString(nodeids[i + 1]) );
			return NULL;
		}

		builder.pushBack( beam );
	}
	// get completed Rail transfers memory ownership
	return builder.getCompletedRail();
}
