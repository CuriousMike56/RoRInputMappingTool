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

#ifndef PANEL_MESHTREE_H__
#define PANEL_MESHTREE_H__

#include <wx/wx.h>

#include <wx/treectrl.h> // for treectrl

#include "RoRViewer.h"

#include "panel_meshprop.h"


class TreeItemData : public wxTreeItemData
{
public:
	TreeItemData(int _value) : value(_value)
	{
	}

	~TreeItemData()
	{
	}

	int value;
};

class PanelMeshTree : public wxPanel
{
protected:
	wxTreeCtrl *tree;
	RoRViewer *viewer;
	PanelMeshProp *meshProp;
	Ogre::MeshPtr current_mesh;
	enum { 
		MESHTREE,
	};

public:
	void setViewer(RoRViewer *_viewer)
	{
		viewer = _viewer;
	}
	
	void setPropGrid(PanelMeshProp *_meshProp)
	{
		meshProp = _meshProp;
	}

    PanelMeshTree(RoRViewer *_viewer, wxWindow *parent,
            wxWindowID winid = wxID_ANY,
            const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxDefaultSize,
            long style = wxTAB_TRAVERSAL | wxNO_BORDER,
            const wxString& name = wxPanelNameStr) :
		wxPanel(parent, winid, pos, size, style, name), viewer(_viewer), current_mesh(0)
    {
		// create sizer and tree
		wxBoxSizer *vsizer = new wxBoxSizer ( wxVERTICAL );
		this->SetSizer(vsizer);
		vsizer->SetSizeHints(this);
		
		// add the treeview
		tree = new wxTreeCtrl(this, MESHTREE);
		vsizer->Add( tree, 1, wxEXPAND );
    }
	
	void updateData()
	{
		if(!viewer) return;
		Ogre::MeshPtr mesh = viewer->GetMesh();
		if(mesh.isNull())
			return;

		// only update if mesh changed
		if(mesh == current_mesh)
			return;

		current_mesh = mesh;

		// mesh valid, lets do the update work
		
		// tree first
		tree->DeleteAllItems();
		wxTreeItemId root = tree->AddRoot(mesh->getName(), -1, -1, new TreeItemData(0));

		// add submeshes
		for(int i = 0; i < mesh->getNumSubMeshes(); i++)
		{
			Ogre::SubMesh *sm = mesh->getSubMesh(i);
			if(!sm) continue;
			tree->AppendItem(root, "SubMesh " + StringConverter::toString(i), -1, -1, new TreeItemData(i+1));
		}

		// select mesh item
		tree->SelectItem(root);

		tree->ExpandAll();


		// update PG
		if(meshProp)
			meshProp->updatePG(0);
	}

	void OnTreeItemChange(wxTreeEvent &event)
	{
		wxTreeItemId id = (wxTreeItemId) event.GetItem();
		TreeItemData *data = (TreeItemData *)tree->GetItemData(id);
		if(!data)
			return;
		if(!meshProp)
			return;
		
		// update 3d viewport :)
		Ogre::Entity *e = viewer->GetEntity();
		if(e && data->value == 0)
		{
			// show full mesh
			for(int i = 0; i < (int)e->getNumSubEntities(); i++)
			{
				SubEntity *se = e->getSubEntity(i);
				if(se) se->setVisible(true);
			}
		} else if (e && data->value != 0)
		{
			// show only one specific submesh
			for(int i = 0; i < (int)e->getNumSubEntities(); i++)
			{
				SubEntity *se = e->getSubEntity(i);
				if(se) se->setVisible( (i == (data->value - 1)) );
			}
		}
		
		// update PG
		meshProp->updatePG(data->value);
	}

private:
    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(PanelMeshTree, wxPanel)
	EVT_TREE_SEL_CHANGED(MESHTREE, PanelMeshTree::OnTreeItemChange)
END_EVENT_TABLE()

#endif //PANEL_MESHTREE_H__
