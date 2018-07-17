/* zlib/libpng license below:
Copyright (c) 2004-2009 Pierre-Michel Ricordel (pricorde{AT}rigsofrods{DOT}com), Thomas Fischer (thomas{AT}rigsofrods{DOT}com)

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
    claim that you wrote the original software. If you use this software
    in a product, an acknowledgment in the product documentation would be
    appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
    misrepresented as being the original software.

    3. This notice may not be removed or altered from any source
    distribution.
*/
// created: 12th of January 2009, thomas fischer thomas{AT}thomasfischer{DOT}biz

#ifndef LANGUAGE_H_
#define LANGUAGE_H_

#ifdef NOLANG
// no language mode
// used when building with wxwidgets for example (as they ship their own i18n)

#define _L(str) Ogre::String(str)

#else // NOLANG

#include "Ogre.h"
#ifdef USE_MOFILEREADER
#include "moFileReader.h"
#endif //MOFILEREADER
#include "rormemory.h"

#define _L(str) LanguageEngine::Instance().lookUp(str)
#define MOFILENAME "ror"

class LanguageEngine : public MemoryAllocatedObject
{
public:
    static LanguageEngine & Instance();
    void setup();
	Ogre::String lookUp(Ogre::String name);
    
protected:
	LanguageEngine();
	~LanguageEngine();
	LanguageEngine(const LanguageEngine&);
	LanguageEngine& operator= (const LanguageEngine&);
	static LanguageEngine* myInstance;
	bool working;
#ifdef USE_MOFILEREADER
	moFileLib::moFileReader *reader;
#endif // MOFILEREADER
	void setupCodeRanges(Ogre::String codeRangesFilename, Ogre::String codeRangesGroupname);
};
#endif //NOLANG
#endif //LANGUAGE_H_