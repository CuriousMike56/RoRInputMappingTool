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
#include "Airfoil.h"

Airfoil::Airfoil(char* fname)
{
	//FILE *fd;
	for (int i=0; i<3601; i++) //init in case of bad things
	{
		cl[i]=0;
		cd[i]=0;
		cm[i]=0;
	}
	char line[1024];
	//we load directly X-Plane AFL file format!!!
	bool process=false;
	bool neg=true;
	int lastia=-1;
	//fd=fopen(rfname, "r");
	ResourceGroupManager& rgm = ResourceGroupManager::getSingleton();
	
	String group="";
	try
	{
		group = rgm.findGroupContainingResource(fname);
	}catch(...)
	{
	}
	if(group == "")
	{
		LogManager::getSingleton().logMessage(String("Airfoil error: could not load airfoil ")+fname);
		return;
	}

	DataStreamPtr ds=rgm.openResource(fname, group);

	//if (!fd)
	//{
	//	exit(1);
	//};
	//while (!feof(fd))
	while (!ds->eof())
	{
		size_t ll=ds->readLine(line, 1023);
		if (ll==0) continue;
		//		fscanf(fd," %[^\n\r]",line);
		if (!strncmp("alpha",line, 5)) {process=true; continue;};
		if (process)
		{
			float l, d, m;
			int a,b;
			sscanf(line,"%i.%i %f %f %f",&a, &b,&l,&d,&m);
			if (neg) b=-b;
			if (a==0 && b==0) neg=false;
			int ia=(a*10+b)+1800;
			if (ia==3600) {process=false;};
			cl[ia]=l;
			cd[ia]=d;
			cm[ia]=m;
			if (lastia!=-1 && ia-lastia>1)
			{
				//we have to interpolate previous elements (linear interpolation)
				int i;
				for (i=0; i<ia-lastia-1; i++)
				{
					cl[lastia+1+i]=cl[lastia]+(float)(i+1)*(cl[ia]-cl[lastia])/(float)(ia-lastia);
					cd[lastia+1+i]=cd[lastia]+(float)(i+1)*(cd[ia]-cd[lastia])/(float)(ia-lastia);
					cm[lastia+1+i]=cm[lastia]+(float)(i+1)*(cm[ia]-cm[lastia])/(float)(ia-lastia);
				}
			}
			lastia=ia;
		}
	}
	//fclose(fd);

	// ds closes automatically, so do not close it explicitly here:
	//ds->close();

}

void Airfoil::getparams(float a, float cratio, float cdef, float* ocl, float* ocd, float* ocm)
{
	int ta=(int)(a/360.0);
	//		float va=360.0f*fmod(a, 360.0f); FMOD IS TOTALLY UNRELIABLE HERE : fmod(-180.0f, 360.0f)=-180.0f!!!!!
	float va=a-(float)(ta*360);
	if (va>180.0f) va-=360.0f;
	if (va<-180.0f) va+=360.0f;
	int ia=(int)((va+180.0f)*10.0f);
	//drag shift
	float dva=va+1.15*(1.0-cratio)*cdef;
	if (dva>180.0f) dva-=360.0f;
	if (dva<-180.0f) dva+=360.0f;
	int dia=(int)((dva+180.0f)*10.0f);
	float sign=1.0;
	if (cdef<0) sign=-1.0;
	*ocl=cl[ia]-0.66*sign*(1.0-cratio)*sqrt(fabs(cdef));
	*ocd=cd[dia]+0.00015*(1.0-cratio)*cdef*cdef;
	*ocm=cm[ia]+0.20*sign*(1.0-cratio)*sqrt(fabs(cdef));
	/*
	float ea=va-((float)ia/10.0)-180.0;
	float res=0.0;
	if (ea<0)
	{
	int pia=ia-1;
	if (pia<0) pia+=3600;
	res=cl[pia]+(1.0+ea*10.0)*(cl[ia]-cl[pia]);
	}
	else
	{
	int nia=ia+1;
	if (nia==3600) nia=0;
	res=cl[ia]+(ea*10.0)*(cl[nia]-cl[ia]);
	}
	return res;
	*/
}

Airfoil::~Airfoil()
{
}

/*
void Airfoil::dumpcl()
{
FILE *fd;
int i;
fd=fopen("cldump.raw", "wb");
short w;

for (i=0; i<36000; i++)
{
float val=getcl((float)i/100.0-180.0, 0, 0);
w=(short)(val*16000.0);
fwrite((unsigned char*)(&w), 1, 2, fd);
}
fclose(fd);
}
*/

