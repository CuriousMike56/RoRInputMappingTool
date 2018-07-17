#ifndef SCOPELOG_H
#define SCOPELOG_H

#include "OgrePrerequisites.h"

/**
 * @brief: this class will change the default log with the scope of its creation.
 * Upon scope leaving, the previous default Log is restored.
 */
class ScopeLog
{
protected:
	Ogre::Log *orgLog;
	Ogre::String logFileName;
	int getFileSize(Ogre::String filename);
public:
	ScopeLog(Ogre::String filename);
	~ScopeLog();
};

#endif
