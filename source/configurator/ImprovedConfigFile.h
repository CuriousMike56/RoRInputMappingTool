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

#ifndef IMPROVEDCONFIGFILE_H_
#define IMPROVEDCONFIGFILE_H_

#include <OgrePrerequisites.h>

namespace Ogre
{

class ImprovedConfigFile : public ConfigFile
{
public:
	ImprovedConfigFile() : separators(), filename()
	{
		ConfigFile();
	}
	
	~ImprovedConfigFile()
	{
	}
    
	// note: saving is only supported for direct loaded files atm!
	void load(const String& filename, const String& separators, bool trimWhitespace)
    {
		this->separators = separators;
		this->filename = filename;
		ConfigFile::load(filename, separators, trimWhitespace);
    }
	
	bool save()
	{
		if(!filename.length())
		{
			OGRE_EXCEPT(Exception::ERR_INTERNAL_ERROR, "Saving of the configuration File is only allowed when the configuration was not loaded using the resource system!", "ImprovedConfigFile::save");
			return false;
		}
		FILE *f = fopen(filename.c_str(), "w");
		if(!f)
		{
			OGRE_EXCEPT(Exception::ERR_FILE_NOT_FOUND, "Cannot open File '"+filename+"' for writing.", "ImprovedConfigFile::save");
			return false;
		}

		SettingsBySection::iterator secIt;
		for(secIt = mSettings.begin(); secIt!=mSettings.end(); secIt++)
		{
			if(secIt->first.size() > 0)
				fprintf(f, "[%s]\n", secIt->first.c_str());
			SettingsMultiMap::iterator setIt;
			for(setIt = secIt->second->begin(); setIt!=secIt->second->end(); setIt++)
			{
				fprintf(f, "%s%c%s\n", setIt->first.c_str(), separators[0], setIt->second.c_str());
			}
			
		}
		fclose(f);
		return true;
	}

	void setSetting(String &key, String &value, String section = StringUtil::BLANK)
	{
		SettingsMultiMap *set = mSettings[section];
		if(!set)
		{
			// new section
			set = new SettingsMultiMap();
			mSettings[section] = set;
		}
		if(set->count(key))
			// known key, delete old first
			set->erase(key);
        // add key
		set->insert(std::multimap<String, String>::value_type(key, value));
	}

	// type specific implementations
	Radian getSettingRadian(String key, String section = StringUtil::BLANK)
	{
		return StringConverter::parseAngle(getSetting(key, section));
	}
	void setSetting(String key, Radian value, String section = StringUtil::BLANK)
	{
		String set = StringConverter::toString(value);
		setSetting(key, set, section);
	}

	bool getSettingBool(String key, String section = StringUtil::BLANK)
	{
		return StringConverter::parseBool(getSetting(key, section));
	}
	void setSetting(String key, bool value, String section = StringUtil::BLANK)
	{
		String set = StringConverter::toString(value);
		setSetting(key, set, section);
	}

	Real getSettingReal(String key, String section = StringUtil::BLANK)
	{
		return StringConverter::parseReal(getSetting(key, section));
	}
	void setSetting(String key, Real value, String section = StringUtil::BLANK)
	{
		String set = StringConverter::toString(value);
		setSetting(key, set, section);
	}

	int getSettingInt(String key, String section = StringUtil::BLANK)
	{
		return StringConverter::parseInt(getSetting(key, section));
	}
	void setSetting(String key, int value, String section = StringUtil::BLANK)
	{
		String set = StringConverter::toString(value);
		setSetting(key, set, section);
	}

	unsigned int getSettingUnsignedInt(String key, String section = StringUtil::BLANK)
	{
		return StringConverter::parseUnsignedInt(getSetting(key, section));
	}
	void setSetting(String key, unsigned int value, String section = StringUtil::BLANK)
	{
		String set = StringConverter::toString(value);
		setSetting(key, set, section);
	}

	long getSettingLong(String key, String section = StringUtil::BLANK)
	{
		return StringConverter::parseLong(getSetting(key, section));
	}
	void setSetting(String key, long value, String section = StringUtil::BLANK)
	{
		String set = StringConverter::toString(value);
		setSetting(key, set, section);
	}

	unsigned long getSettingUnsignedLong(String key, String section = StringUtil::BLANK)
	{
		return StringConverter::parseUnsignedLong(getSetting(key, section));
	}
	void setSetting(String key, unsigned long value, String section = StringUtil::BLANK)
	{
		String set = StringConverter::toString(value);
		setSetting(key, set, section);
	}

	Vector3 getSettingVector3(String key, String section = StringUtil::BLANK)
	{
		return StringConverter::parseVector3(getSetting(key, section));
	}
	void setSetting(String key, Vector3 value, String section = StringUtil::BLANK)
	{
		String set = StringConverter::toString(value);
		setSetting(key, set, section);
	}

	Matrix3 getSettingMatrix3(String key, String section = StringUtil::BLANK)
	{
		return StringConverter::parseMatrix3(getSetting(key, section));
	}
	void setSetting(String key, Matrix3 value, String section = StringUtil::BLANK)
	{
		String set = StringConverter::toString(value);
		setSetting(key, set, section);
	}

	Matrix4 getSettingMatrix4(String key, String section = StringUtil::BLANK)
	{
		return StringConverter::parseMatrix4(getSetting(key, section));
	}
	void setSetting(String key, Matrix4 value, String section = StringUtil::BLANK)
	{
		String set = StringConverter::toString(value);
		setSetting(key, set, section);
	}

	Quaternion getSettingQuaternion(String key, String section = StringUtil::BLANK)
	{
		return StringConverter::parseQuaternion(getSetting(key, section));
	}
	void setSetting(String key, Quaternion value, String section = StringUtil::BLANK)
	{
		String set = StringConverter::toString(value);
		setSetting(key, set, section);
	}

	ColourValue getSettingColorValue(String key, String section = StringUtil::BLANK)
	{
		return StringConverter::parseColourValue(getSetting(key, section));
	}
	void setSetting(String key, ColourValue value, String section = StringUtil::BLANK)
	{
		String set = StringConverter::toString(value);
		setSetting(key, set, section);
	}

	StringVector getSettingStringVector(String key, String section = StringUtil::BLANK)
	{
		return StringConverter::parseStringVector(getSetting(key, section));
	}
	void setSetting(String key, StringVector value, String section = StringUtil::BLANK)
	{
		String set = StringConverter::toString(value);
		setSetting(key, set, section);
	}

protected:
	String separators;
	String filename;
};

};
#endif
