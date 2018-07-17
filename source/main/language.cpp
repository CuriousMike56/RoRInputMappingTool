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

#ifndef NOLANG

#include "OgreFontManager.h"
#include "language.h"
#include "Settings.h"

//#include "MyGUI_Font.h"
//#include "MyGUI_FontManager.h"

#include "fontTextureHelper.h"

using namespace std;
using namespace Ogre;
#ifdef USE_MOFILEREADER
using namespace moFileLib;
#endif //MOFILEREADER

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
#define strnlen(str,len) strlen(str)
#endif

// singleton pattern
LanguageEngine* LanguageEngine::myInstance = 0;

LanguageEngine &LanguageEngine::Instance()
{
	if (myInstance == 0)
		myInstance = new LanguageEngine;
	return *myInstance;
}

LanguageEngine::LanguageEngine() : working(false)
{
}

LanguageEngine::~LanguageEngine()
{
}

void LanguageEngine::setup()
{
#ifdef USE_MOFILEREADER
	reader = new moFileReader();

	String language = SETTINGS.getSetting("Language");
	String language_short = SETTINGS.getSetting("Language Short").substr(0, 2); // only first two characters are important

	// Load a .mo-File.
	Ogre::LogManager::getSingleton().logMessage("*** Loading Language ***");
	String langfile = SETTINGS.getSetting("Program Path") + String("languages/") + language_short + String("/") + String(MOFILENAME) + String(".mo");
	if (reader->ReadFile(langfile.c_str()) != moFileLib::moFileReader::EC_SUCCESS )
	{
			Ogre::LogManager::getSingleton().logMessage("* error loading language file " + langfile);
			return;
	}
	working=true;

	// add resource path
	ResourceGroupManager::getSingleton().addResourceLocation(SETTINGS.getSetting("Program Path") + "languages/" + language_short, "FileSystem", "LanguageRanges");

	// now load the code ranges
	// be aware, that this approach only works if we load just one language, and not multiple
	setupCodeRanges("codes.txt", "LanguageRanges");

	Ogre::LogManager::getSingleton().logMessage("* Language successfully loaded");
#endif //MOFILEREADER
}

Ogre::String LanguageEngine::lookUp(Ogre::String name)
{
#ifdef USE_MOFILEREADER
	if(working)
		return reader->Lookup(name.c_str());
#endif // MOFILEREADER
	return name;
}

void LanguageEngine::setupCodeRanges(String codeRangesFilename, String codeRangesGroupname)
{
	DataStreamPtr ds = ResourceGroupManager::getSingleton().openResource(codeRangesFilename, codeRangesGroupname);
	if(ds.isNull())
	{
		Ogre::LogManager::getSingleton().logMessage("unable to load language code points file: " + codeRangesFilename);
		return;
	}
	Ogre::LogManager::getSingleton().logMessage("loading code_range file: " + codeRangesFilename);

	char line[1024] = "";
	while (!ds->eof())
	{
		size_t ll = ds->readLine(line, 1023);
		// only process valid lines
		if(strncmp(line, "code_points ", 12) && strnlen(line, 50) > 13)
			continue;
		Ogre::vector<String>::type args = StringUtil::split(line + 12, " ");
		for(Ogre::vector<String>::type::iterator it=args.begin(); it!=args.end(); it++)
		{
			Font::CodePointRange range;
			StringVector itemVec = StringUtil::split(*it, "-");
			if (itemVec.size() == 2)
				range = Font::CodePointRange(StringConverter::parseLong(itemVec[0]), StringConverter::parseLong(itemVec[1]));

			// add this code range to all available fonts
			ResourceManager::ResourceMapIterator itf = FontManager::getSingleton().getResourceIterator();
			while (itf.hasMoreElements())
				((FontPtr)itf.getNext())->addCodePointRange(range);

			// add code points to all MyGUI fonts
			// XXX: TOFIX: CRASH!
			//MyGUI::Font *fp = (MyGUI::Font *)MyGUI::FontManager::getInstance().getByName("Default");
			//if(fp) fp->addCodePointRange(range.first, range.second);
			//fp = (MyGUI::Font *)MyGUI::FontManager::getInstance().getByName("DefaultBig");
			//if(fp) fp->addCodePointRange(range.first, range.second);
		}
	}

	// reload all ttf fonts
	ResourceManager::ResourceMapIterator itf = FontManager::getSingleton().getResourceIterator();
	while (itf.hasMoreElements())
	{
		FontPtr font = itf.getNext();
		// if the font is a ttf font and loaded, then reload it in order to regenenerate the glyphs with corrected code_points
		if(font->getType() == Ogre::FT_TRUETYPE && font->isLoaded())
			font->reload();
	}

	//fontCacheInit("Cyberbit");
	//generateAllFontTextures();
}
#endif //NOLANG