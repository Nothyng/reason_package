////////////////////////////////////////////////////////
//
// GEM - Graphics Environment for Multimedia
//
// Implementation file
//
// Copyright (c) 2002 IOhannes m zmoelnig. forum::f�r::uml�ute. IEM
//	zmoelnig@iem.kug.ac.at
//  For information on usage and redistribution, and for a DISCLAIMER
//  *  OF ALL WARRANTIES, see the file, "GEM.LICENSE.TERMS"
//
//  this file has been generated...
////////////////////////////////////////////////////////

#include "GEMglFrontFace.h"
#include "Base/GemException.h"

CPPEXTERN_NEW_WITH_GIMME ( GEMglFrontFace )

/////////////////////////////////////////////////////////
//
// GEMglViewport
//
/////////////////////////////////////////////////////////
// Constructor
//
GEMglFrontFace :: GEMglFrontFace (int argc, t_atom*argv) :
		mode(0)
{
  if(1==argc)modeMess(argv[0]); else if(argc) throw(GemException("invalid number of arguments"));
	m_inlet[0] = inlet_new(this->x_obj, &this->x_obj->ob_pd, &s_float, gensym("mode"));
}
/////////////////////////////////////////////////////////
// Destructor
//
GEMglFrontFace :: ~GEMglFrontFace () {
inlet_free(m_inlet[0]);
}

/////////////////////////////////////////////////////////
// Render
//
void GEMglFrontFace :: render(GemState *state) {
	glFrontFace (mode);
}

/////////////////////////////////////////////////////////
// Variables
//
void GEMglFrontFace :: modeMess (t_atom arg) {	// FUN
	mode = static_cast<GLenum>(getGLdefine(&arg));
	setModified();
}


/////////////////////////////////////////////////////////
// static member functions
//

void GEMglFrontFace :: obj_setupCallback(t_class *classPtr) {
	 class_addmethod(classPtr, reinterpret_cast<t_method>(&GEMglFrontFace::modeMessCallback),  	gensym("mode"), A_GIMME, A_NULL);
};

void GEMglFrontFace :: modeMessCallback (void* data, t_symbol*, int argc, t_atom*argv){
	if(argc==1)GetMyClass(data)->modeMess ( argv[0]);
}