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

#include "utils.h"
#include "Ogre.h"

using namespace Ogre;

String hexdump(void *pAddressIn, long  lSize)
{
	char szBuf[100];
	long lIndent = 1;
	long lOutLen, lIndex, lIndex2, lOutLen2;
	long lRelPos;
	struct { char *pData; unsigned long lSize; } buf;
	unsigned char *pTmp,ucTmp;
	unsigned char *pAddress = (unsigned char *)pAddressIn;

	buf.pData   = (char *)pAddress;
	buf.lSize   = lSize;

	String result = "";
	
	while (buf.lSize > 0)
	{
		pTmp     = (unsigned char *)buf.pData;
		lOutLen  = (int)buf.lSize;
		if (lOutLen > 16)
		  lOutLen = 16;

		// create a 64-character formatted output line:
		sprintf(szBuf, " >                            "
					 "                      "
					 "    %08lX", (long unsigned int)(pTmp-pAddress));
		lOutLen2 = lOutLen;

		for(lIndex = 1+lIndent, lIndex2 = 53-15+lIndent, lRelPos = 0;
		  lOutLen2;
		  lOutLen2--, lIndex += 2, lIndex2++
			)
		{
			ucTmp = *pTmp++;

			sprintf(szBuf + lIndex, "%02X ", (unsigned short)ucTmp);
			if(!isprint(ucTmp))  ucTmp = '.'; // nonprintable char
			szBuf[lIndex2] = ucTmp;

			if (!(++lRelPos & 3))     // extra blank after 4 bytes
			{  lIndex++; szBuf[lIndex+2] = ' '; }
		}

		if (!(lRelPos & 3)) lIndex--;

		szBuf[lIndex  ]   = '<';
		szBuf[lIndex+1]   = ' ';

		result += String(szBuf) + "\n";

		buf.pData   += lOutLen;
		buf.lSize   -= lOutLen;
	}
	return result;
}

UTFString tryConvertUTF(char *buffer)
{
	try
	{
		UTFString s = UTFString(buffer);
		if(s.empty())
			s = UTFString("(conversion error)");
		return s;

	} catch(...)
	{
		return UTFString();
	}
	return UTFString();
}

Ogre::String formatBytes(double bytes)
{
	char tmp[128]="";
	if(bytes <= 1024)
		sprintf(tmp, "%0.2f B", bytes);
	else if(bytes > 1024 && bytes <= 1048576)
		sprintf(tmp, "%0.2f KB", bytes / 1024.0f);
	else if(bytes > 1048576 && bytes <= 1073741824)
		sprintf(tmp, "%0.2f MB", bytes / 1024.0f / 1024.0f);
	else //if(bytes > 1073741824 && bytes <= 1099511627776)
		sprintf(tmp, "%0.2f GB", bytes / 1024.0f / 1024.0f / 1024.0f);
	//else if(bytes > 1099511627776)
	//	sprintf(res, "%0.2f TB", bytes / 1024.0f / 1024.0f / 1024.0f / 1024.0f);
	return Ogre::String(tmp);
}