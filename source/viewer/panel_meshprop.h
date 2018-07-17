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

#ifndef PANEL_MESHPROP_H__
#define PANEL_MESHPROP_H__

#include <wx/wx.h>

// Main propertygrid header.
#include <wx/propgrid/propgrid.h>

// Extra property classes.
#include <wx/propgrid/advprops.h>

// This defines wxPropertyGridManager.
#include <wx/propgrid/manager.h>

#include "RoRViewer.h"

class PanelMeshProp : public wxPanel
{
protected:
	RoRViewer *viewer;
	Ogre::MeshPtr current_mesh;
	int current_item;
	wxPropertyGrid  *pg;
	wxPropertyGridManager *pgman;
	enum { 
		PGID1,
		PGID2,
	};

public:
	void setViewer(RoRViewer *_viewer)
	{
		viewer = _viewer;
	}

    PanelMeshProp(RoRViewer *_viewer, wxWindow *parent,
            wxWindowID winid = wxID_ANY,
            const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxDefaultSize,
            long style = wxTAB_TRAVERSAL | wxNO_BORDER,
            const wxString& name = wxPanelNameStr) :
		wxPanel(parent, winid, pos, size, style, name), viewer(_viewer), current_mesh(0), current_item(-1)
    {
		// create sizer and PG
		wxBoxSizer *vsizer = new wxBoxSizer ( wxVERTICAL );
		this->SetSizer(vsizer);
		vsizer->SetSizeHints(this);
		
		// then add the propertygrid
		int pgstyle = wxPG_BOLD_MODIFIED | wxPG_SPLITTER_AUTO_CENTER | wxPG_AUTO_SORT | wxPG_TOOLBAR | wxPG_DESCRIPTION;
		int extrastyle = wxPG_EX_MODE_BUTTONS;
		pg = new wxPropertyGrid(this,
									PGID1,
									wxDefaultPosition,
									wxDefaultSize,
									pgstyle);
		pgman = new wxPropertyGridManager(this,
                                  PGID2,
                                  wxDefaultPosition,
                                  wxDefaultSize,
                                  pgstyle );
		pgman->SetExtraStyle(extrastyle);
		vsizer->Add( pgman, 1, wxEXPAND );
		//pg->Bind(wxEVT_PG_CHANGED, &PanelTruckProp::onPropertyGridChange, this, PGID1);
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
	}

	void OnPropertyChange(wxPropertyGridEvent &ev)
	{
		wxPGProperty *p = ev.GetProperty();
	}

	void updatePG(int item)
	{
		wxPGProperty *p=0;

		// check if update is needed
		if(item == current_item)
			return;
		current_item = item;

		pgman->Clear();
		if(item == 0)
		{
			// main mesh
			wxPropertyGridPage *pgp = pgman->AddPage(wxT("Mesh"));

			// bounds
			pgp->Append(new wxPropertyCategory("Bounding Box"));

			AxisAlignedBox bounds = current_mesh->getBounds();
			Vector3 bounds_min = bounds.getMinimum();
			Vector3 bounds_max = bounds.getMaximum();
			wxPGProperty *boundsProp    = pgp->Append(new wxStringProperty("Bounds", wxPG_LABEL, "<composed>"));
			pgp->SetPropertyReadOnly(boundsProp);
			pgp->SetPropertyHelpString(boundsProp, "axis-aligned bounding box for this mesh."); 
			wxPGProperty *boundsMinProp = pgp->AppendIn(boundsProp, new wxStringProperty("Minimum", wxPG_LABEL, "<composed>"));
			boundsMinProp->SetExpanded(false);
			wxPGProperty *boundsMinX    = pgp->AppendIn(boundsMinProp, new wxIntProperty("X", wxPG_LABEL, bounds_min.x));
			wxPGProperty *boundsMinY    = pgp->AppendIn(boundsMinProp, new wxIntProperty("Y", wxPG_LABEL, bounds_min.y));
			wxPGProperty *boundsMinZ    = pgp->AppendIn(boundsMinProp, new wxIntProperty("Z", wxPG_LABEL, bounds_min.z));
			wxPGProperty *boundsMaxProp = pgp->AppendIn(boundsProp, new wxStringProperty("Maximum", wxPG_LABEL, "<composed>"));
			
			boundsMaxProp->SetExpanded(false);
			wxPGProperty *boundsMaxX    = pgp->AppendIn(boundsMaxProp, new wxIntProperty("X", wxPG_LABEL, bounds_max.x));
			wxPGProperty *boundsMaxY    = pgp->AppendIn(boundsMaxProp, new wxIntProperty("Y", wxPG_LABEL, bounds_max.y));
			wxPGProperty *boundsMaxZ    = pgp->AppendIn(boundsMaxProp, new wxIntProperty("Z", wxPG_LABEL, bounds_max.z));

			pgp->Append(p = new wxFloatProperty("Bounding Sphere Radius", wxPG_LABEL, current_mesh->getBoundingSphereRadius()));
			pgp->SetPropertyHelpString(p, "radius of the bounding sphere surrounding this mesh"); 
			pgp->SetPropertyReadOnly(p);

			
			pgp->Append(new wxPropertyCategory("LODs"));
			pgp->Append(p = new wxIntProperty("Number of LOD Levels", wxPG_LABEL, current_mesh->getNumLodLevels()));
			pgp->SetPropertyHelpString(p, "number of levels of detail that this mesh supports");
			pgp->SetPropertyReadOnly(p);
			
			for(int i=0; i<current_mesh->getNumLodLevels(); i++)
			{
				MeshLodUsage lu = current_mesh->getLodLevel(i);
				pgp->Append(p = new wxFloatProperty("value", wxPG_LABEL, lu.value));
				pgp->SetPropertyHelpString(p, "Value used to determine when this lod applies");
				pgp->SetPropertyReadOnly(p);
				pgp->Append(p = new wxFloatProperty("userValue", wxPG_LABEL, lu.userValue));
				pgp->SetPropertyHelpString(p, "User-supplied values used to determine when th is lod applies");
				pgp->SetPropertyReadOnly(p);
				pgp->Append(p = new wxStringProperty("manualName", wxPG_LABEL, lu.manualName));
				pgp->SetPropertyHelpString(p, "name of the alternative mesh to use");
				pgp->SetPropertyReadOnly(p);
				pgp->Append(p = new wxStringProperty("manualGroup", wxPG_LABEL, lu.manualGroup));
				pgp->SetPropertyHelpString(p, "name of the group of the alternative mesh");
				pgp->SetPropertyReadOnly(p);
				EdgeData *edgeData = lu.edgeData;
				if(edgeData)
				{
					pgp->Append(p = new wxIntProperty("TriangleCount", wxPG_LABEL, edgeData->triangles.size()));
					pgp->SetPropertyReadOnly(p);
				}
			}
			pgp->Append(p = new wxBoolProperty("isLodManual", wxPG_LABEL, current_mesh->isLodManual()));
			pgp->SetPropertyHelpString(p, "true if this mesh is using manual LOD");
			pgp->SetPropertyReadOnly(p);

			
			pgp->Append(new wxPropertyCategory("Unsorted"));
			pgp->Append(p = new wxIntProperty("Number of Submeshes", wxPG_LABEL, current_mesh->getNumSubMeshes()));
			pgp->SetPropertyHelpString(p, "number of sub meshes which comprise this mesh");
			pgp->SetPropertyReadOnly(p);

			wxArrayString arrBufferUsage;
			arrBufferUsage.Add("HBU_STATIC");
			arrBufferUsage.Add("HBU_DYNAMIC");
			arrBufferUsage.Add("HBU_WRITE_ONLY");
			arrBufferUsage.Add("HBU_DISCARDABLE");
			arrBufferUsage.Add("HBU_STATIC_WRITE_ONLY");
			arrBufferUsage.Add("HBU_DYNAMIC_WRITE_ONLY");
			arrBufferUsage.Add("HBU_DYNAMIC_WRITE_ONLY_DISCARDABLE");

			pgp->Append(p = new wxEnumProperty("getVertexBufferUsage", wxPG_LABEL, arrBufferUsage, wxArrayInt(), current_mesh->getVertexBufferUsage()));
			pgp->SetPropertyHelpString(p, "usage setting for this meshes vertex buffers");
			pgp->SetPropertyReadOnly(p);
			pgp->Append(p = new wxEnumProperty("getIndexBufferUsage",  wxPG_LABEL, arrBufferUsage, wxArrayInt(), current_mesh->getIndexBufferUsage()));
			pgp->SetPropertyHelpString(p, "usage setting for this meshes index buffers");
			pgp->SetPropertyReadOnly(p);

			pgp->Append(p = new wxBoolProperty("isVertexBufferShadowed", wxPG_LABEL, current_mesh->isVertexBufferShadowed()));
			pgp->SetPropertyHelpString(p, "whether or not this meshes vertex buffers are shadowed");
			pgp->SetPropertyReadOnly(p);
			pgp->Append(p = new wxBoolProperty("isIndexBufferShadowed", wxPG_LABEL, current_mesh->isIndexBufferShadowed()));
			pgp->SetPropertyHelpString(p, "whether or not this meshes index buffers are shadowed");
			pgp->SetPropertyReadOnly(p);
			 
			pgp->Append(new wxPropertyCategory("Animations"));
			pgp->Append(p = new wxIntProperty("Animations Count", wxPG_LABEL, current_mesh->getNumAnimations()));
			pgp->SetPropertyHelpString(p, "number of morph animations in this mesh");
			pgp->SetPropertyReadOnly(p);

			pgp->Append(p = new wxIntProperty("Pose Count", wxPG_LABEL, current_mesh->getPoseCount()));
			pgp->SetPropertyHelpString(p, "number of poses");
			pgp->SetPropertyReadOnly(p);

			pgp->Append(p = new wxBoolProperty("Has Skeleton", wxPG_LABEL, current_mesh->hasSkeleton()));
			pgp->SetPropertyHelpString(p, "true if this Mesh has a linked Skeleton");
			pgp->SetPropertyReadOnly(p);
			
			if(current_mesh->hasSkeleton())
			{
				pgp->Append(p = new wxStringProperty("Skeleton Name", wxPG_LABEL, current_mesh->getSkeletonName()));
				pgp->SetPropertyHelpString(p, "name of any linked Skeleton");
				pgp->SetPropertyReadOnly(p);
			}
			pgp->Append(p = new wxBoolProperty("Has Vertex Animation", wxPG_LABEL, current_mesh->hasVertexAnimation()));
			pgp->SetPropertyHelpString(p, "whether or not this mesh has some kind of vertex animation");
			pgp->SetPropertyReadOnly(p);

			pgp->Append(new wxPropertyCategory("Parameters"));
			ParameterList pl = current_mesh->getParameters();
			pgp->Append(p = new wxIntProperty("Parameter Count", wxPG_LABEL, pl.size()));
			pgp->SetPropertyReadOnly(p);
			for(int i=0; i < (int)pl.size(); i++)
			{
				wxPGProperty *parameterProp = pgp->Append(new wxIntProperty("Parameter " + StringConverter::toString(i), wxPG_LABEL, pl.size()));
				pgp->AppendIn(parameterProp, p = new wxStringProperty("name", wxPG_LABEL, pl[i].name));
				pgp->SetPropertyReadOnly(p);
				pgp->AppendIn(parameterProp, p = new wxStringProperty("description", wxPG_LABEL, pl[i].description));
				pgp->SetPropertyReadOnly(p);
				pgp->AppendIn(parameterProp, p = new wxIntProperty("paramType", wxPG_LABEL, pl[i].paramType));
				pgp->SetPropertyReadOnly(p);
				pgp->AppendIn(parameterProp, p = new wxStringProperty("value", wxPG_LABEL, current_mesh->getParameter(pl[i].name)));
				pgp->SetPropertyReadOnly(p);
			}
		} else
		{
			// a submesh
			// reduce item number by one, so it fits again
			item--;

			wxPropertyGridPage *pgp = pgman->AddPage(wxT("SubMesh"));
			SubMesh *sm = current_mesh->getSubMesh(item);
			if(!sm) return;
			pgp->Append(new wxPropertyCategory("General"));
			pgp->Append(p = new wxStringProperty("Material Name", wxPG_LABEL, sm->getMaterialName()));
			pgp->SetPropertyReadOnly(p);

			pgp->Append(p = new wxBoolProperty("isMatInitialised", wxPG_LABEL, sm->isMatInitialised()));
			pgp->SetPropertyHelpString(p, "true if a material has been assigned to the submesh, otherwise false");
			pgp->SetPropertyReadOnly(p);

			pgp->Append(p = new wxBoolProperty("hasTextureAliases", wxPG_LABEL, sm->hasTextureAliases()));
			pgp->SetPropertyHelpString(p, "true if the sub mesh has texture aliases");
			pgp->SetPropertyReadOnly(p);
			
			if(sm->hasTextureAliases())
			{
				pgp->Append(p = new wxIntProperty("textureAliasCount", wxPG_LABEL, sm->getTextureAliasCount()));
				pgp->SetPropertyHelpString(p, "number of texture aliases assigned to the sub mesh");
				pgp->SetPropertyReadOnly(p);
			}

			wxArrayString arrVertAnimType;
			arrVertAnimType.Add("VAT_NONE");
			arrVertAnimType.Add("VAT_MORPH");
			arrVertAnimType.Add("VAT_POSE");

			pgp->Append(p = new wxEnumProperty("vertexAnimationType", wxPG_LABEL, arrVertAnimType, wxArrayInt(), sm->getVertexAnimationType()));
			pgp->SetPropertyHelpString(p, "type of any vertex animation used by dedicated geometry");
			pgp->SetPropertyReadOnly(p);
			
			pgp->Append(p = new wxBoolProperty("buildEdgesEnabled", wxPG_LABEL, sm->isBuildEdgesEnabled()));
			pgp->SetPropertyHelpString(p, "true(by default) if the submesh should be included in the mesh EdgeList, otherwise returns false");
			pgp->SetPropertyReadOnly(p);

			pgp->Append(p = new wxBoolProperty("useSharedVertices", wxPG_LABEL, sm->useSharedVertices));
			pgp->SetPropertyHelpString(p, "Indicates if this submesh shares vertex data with other meshes or whether it has it's own vertices");
			pgp->SetPropertyReadOnly(p);

			if(sm->vertexData)
			{
				pgp->Append(p = new wxIntProperty("vertexCount", wxPG_LABEL, sm->vertexData->vertexCount));
				pgp->SetPropertyHelpString(p, "The number of vertices used in this operation");
				pgp->SetPropertyReadOnly(p);
			}

			if(sm->indexData)
			{
				pgp->Append(p = new wxIntProperty("indexCount", wxPG_LABEL, sm->indexData->indexCount));
				pgp->SetPropertyHelpString(p, "The number of indexes to use from the buffer");
				pgp->SetPropertyReadOnly(p);
			}

			// maybe add sm->extremityPoints ?
		}
	}

private:
    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(PanelMeshProp, wxPanel)
    EVT_PG_CHANGED( wxID_ANY, PanelMeshProp::OnPropertyChange )
END_EVENT_TABLE()

#endif //PANEL_MESHPROP_H__
