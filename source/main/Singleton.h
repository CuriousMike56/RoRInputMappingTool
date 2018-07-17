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
#ifndef __SINGLETON_H__
#define __SINGLETON_H__

// damn you "using namespace Ogre" that cause name conflict with Singleton

template <class T>
class Singleton2
{
	static T* _self;
protected:
	Singleton2(){}
	virtual ~Singleton2() { _self = 0; }
	void _free() { delete this; }
public:
	static T& getInstance()
	{
		if (!_self) _self = new T;
		return *_self;
	}
	static T* getInstancePtr()
	{
		if (!_self) _self = new T;
		return _self;
	}
	// just shorter name
	static T* get() { return getInstancePtr(); }
	static void FreeInstance() { getInstancePtr()->_free(); }
};

template <class T>
T* Singleton2<T>::_self = 0;

#endif // __SINGLETON_H__
