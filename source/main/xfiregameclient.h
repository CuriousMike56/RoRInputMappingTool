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

#ifdef WIN32
#ifdef USE_XFIRE

#ifndef __XFIREGAMECLIENT_H__
#define __XFIREGAMECLIENT_H__

#ifdef __cplusplus
extern "C" {
#endif

/*
**  XfireIsLoaded()
**
**  returns 1 if application can talk to Xfire, 0 otherwise
*/
int XfireIsLoaded();

/*
**  XfireSetCustomGameDataA()
**
**  ANSI version to tell xfire of custom game data
*/
int XfireSetCustomGameDataA(int num_keys, const char **keys, const char **values);

/*
**  XfireSetCustomGameDataA()
**
**  UNICODE version to tell xfire of custom game data
*/
int XfireSetCustomGameDataW(int num_keys, const wchar_t **keys, const wchar_t **values);

/*
**  XfireSetCustomGameDataUTF8()
**
**  UTF8 version to tell xfire of custom game data
*/
int XfireSetCustomGameDataUTF8(int num_keys, const char **keys, const char **values);

#ifdef UNICODE
#define XfireSetCustomGameData XfireSetCustomGameDataW
#else
#define XfireSetCustomGameData XfireSetCustomGameDataA
#endif


#ifdef __cplusplus
}
#endif

#endif /* __XFIREGAMECLIENT_H__ */

#endif //XFIRE
#endif //__WIN32__
