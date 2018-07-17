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
#ifdef USE_MYGUI 

#ifndef __SELECTOR_WINDOW_H__
#define __SELECTOR_WINDOW_H__

#include "Singleton.h"
#include <BaseLayout.h>
#include <Ogre.h>
#include "skin.h"

class Cache_Entry;

ATTRIBUTE_CLASS_LAYOUT(SelectorWindow, "SelectorWindow.layout");
class SelectorWindow :
	public wraps::BaseLayout,
	public Singleton2<SelectorWindow>
{
	friend class Singleton2<SelectorWindow>;
	SelectorWindow();
	~SelectorWindow();
public:

	void setupCamera(Ogre::Camera* _camera) { mCamera = _camera; }

	bool isFinishedSelecting();
	enum LoaderType {LT_None, LT_Terrain, LT_Vehicle, LT_Truck, LT_Car, LT_Boat, LT_Airplane, LT_Trailer, LT_Load, LT_Extension, LT_Network, LT_NetworkWithBoat, LT_Heli, LT_SKIN, LT_AllBeam};
	void show(LoaderType type);
	void hide();

	Cache_Entry *getSelection() { return mSelectedTruck; }
	Skin *getSelectedSkin() { return mSelectedSkin; }
	std::vector<Ogre::String> getTruckConfig() { return mTruckConfigs; }
	void setEnableCancel(bool enabled);
private:
	// gui events
	void eventKeyButtonPressed_Main(MyGUI::WidgetPtr _sender, MyGUI::KeyCode _key, MyGUI::Char _char);
	void eventComboChangePositionTypeComboBox(MyGUI::ComboBoxPtr _sender, size_t _index);
	void eventListChangePositionModelList(MyGUI::ListPtr _sender, size_t _index);
	void eventComboAcceptConfigComboBox(MyGUI::ComboBoxPtr _sender, size_t _index);
	void eventMouseButtonClickOkButton(MyGUI::WidgetPtr _sender);
	void eventMouseButtonClickCancelButton(MyGUI::WidgetPtr _sender);
	void eventSearchTextChange(MyGUI::WidgetPtr _sender);
	void eventSearchTextGotFocus(MyGUI::WidgetPtr _sender, MyGUI::WidgetPtr oldWidget);
	

	// other functions
	void getData();
	void onCategorySelected(int categoryID);
	void onEntrySelected(int entryID);
	void selectionDone();
	
	void updateControls(Cache_Entry *entry);
	void setPreviewImage(Ogre::String texture);

	std::vector<Cache_Entry> mEntries;
	std::map<int, int> mCategoryUsage;
	std::vector<Skin *> mCurrentSkins;
	LoaderType mLoaderType;
	bool mSelectionDone;
	std::vector<Ogre::String> mTruckConfigs;
	Ogre::Camera *mCamera;
	Cache_Entry *mSelectedTruck;
	int visibleCounter;
	Skin *mSelectedSkin;
private:
	ATTRIBUTE_FIELD_WIDGET_NAME(SelectorWindow, mCacheSheet, "Cache");
	MyGUI::Sheet* mCacheSheet;
	ATTRIBUTE_FIELD_WIDGET_NAME(SelectorWindow, mTypeComboBox, "Type");
	MyGUI::ComboBox* mTypeComboBox;
	ATTRIBUTE_FIELD_WIDGET_NAME(SelectorWindow, mModelList, "Model");
	MyGUI::List* mModelList;
	ATTRIBUTE_FIELD_WIDGET_NAME(SelectorWindow, mEntryNameStaticText, "EntryName");
	MyGUI::StaticText* mEntryNameStaticText;
	ATTRIBUTE_FIELD_WIDGET_NAME(SelectorWindow, mEntryDescriptionStaticText, "EntryDescription");
	MyGUI::StaticText* mEntryDescriptionStaticText;
	ATTRIBUTE_FIELD_WIDGET_NAME(SelectorWindow, mConfigComboBox, "Config");
	MyGUI::ComboBox* mConfigComboBox;
	ATTRIBUTE_FIELD_WIDGET_NAME(SelectorWindow, mPreviewStaticImage, "Preview");
	MyGUI::StaticImage* mPreviewStaticImage;
	ATTRIBUTE_FIELD_WIDGET_NAME(SelectorWindow, mOkButton, "Ok");
	MyGUI::Button* mOkButton;
	ATTRIBUTE_FIELD_WIDGET_NAME(SelectorWindow, mCancelButton, "Cancel");
	MyGUI::Button* mCancelButton;
	ATTRIBUTE_FIELD_WIDGET_NAME(SelectorWindow, mSearchSheet, "Search");
	MyGUI::Sheet* mSearchSheet;
	ATTRIBUTE_FIELD_WIDGET_NAME(SelectorWindow, mSearchLineEdit, "SearchLine");
	MyGUI::Edit* mSearchLineEdit;
};

#endif // __SELECTOR_WINDOW_H__

#endif //MYGUI

