////////////////////////////////////////////////////////
//
// GEM - Graphics Environment for Multimedia
//
// Created by tigital on 10/04/2005.
// Copyright 2005 James Tittle
//
// Implementation file
//
//    Copyright (c) 1997-1999 Mark Danks.
//    Copyright (c) G¸nther Geiger.
//    Copyright (c) 2001-2002 IOhannes m zmoelnig. forum::f¸r::uml‰ute. IEM
//    For information on usage and redistribution, and for a DISCLAIMER OF ALL
//    WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.
//
/////////////////////////////////////////////////////////

#include "glsl_vertex.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef __APPLE__
#include <AGL/agl.h>
extern bool HaveValidContext (void);
#endif

CPPEXTERN_NEW_WITH_ONE_ARG(glsl_vertex, t_symbol *, A_DEFSYM)

/////////////////////////////////////////////////////////
//
// glsl_vertex
//
/////////////////////////////////////////////////////////
// Constructor
//
/////////////////////////////////////////////////////////
glsl_vertex :: glsl_vertex() :
  m_shaderType(GEM_shader_none), 
  m_shader(0), m_compiled(0), m_size(0), 
  m_shaderString(NULL), m_shaderID(0)
{
#ifdef GL_ARB_shader_objects
  m_shaderTarget = GL_VERTEX_SHADER_ARB;
#endif
}
glsl_vertex :: glsl_vertex(t_symbol *filename) :
  m_shaderType(GEM_shader_none), 
  m_shader(0), m_compiled(0), m_size(0), 
  m_shaderString(NULL), m_shaderID(0)
{
#ifdef GL_ARB_shader_objects
  m_shaderTarget = GL_VERTEX_SHADER_ARB;
  openMess(filename);
  
  // create an outlet to send shader object ID
  m_outShaderID = outlet_new(this->x_obj, &s_float);
#endif
}

/////////////////////////////////////////////////////////
// Destructor
//
/////////////////////////////////////////////////////////
glsl_vertex :: ~glsl_vertex()
{
  glDeleteObjectARB( m_shader );
  closeMess();
}

/////////////////////////////////////////////////////////
// closeMess
//
/////////////////////////////////////////////////////////
void glsl_vertex :: closeMess(void)
{
  if(m_shaderString)delete [] m_shaderString;
  m_shaderString=NULL;
  m_size=0;
  if(m_shader){
#ifdef GL_ARB_shader_objects
	glDeleteObjectARB( m_shader );
#endif
  }
  m_shader=0;
  m_compiled=0;
  m_shaderType=GEM_shader_none;
}

/////////////////////////////////////////////////////////
// openMess
//
/////////////////////////////////////////////////////////
void glsl_vertex :: openMess(t_symbol *filename)
{
  if(NULL==filename || NULL==filename->s_name)return;
#ifdef __APPLE__
  if (!HaveValidContext ()) {
    post("GEM: [%s] - need window/context to load shader", m_objectname->s_name);
    return;
  }
#endif

  char buf[MAXPDSTRING];
  canvas_makefilename(getCanvas(), filename->s_name, buf, MAXPDSTRING);

  // Clean up any open files
  closeMess();

  FILE *file = fopen(buf,"r");
  if(file) {
    fseek(file,0,SEEK_END);
    int size = ftell(file);
    m_shaderString = new char[size + 1];
    memset(m_shaderString,0,size + 1);
    fseek(file,0,SEEK_SET);
    fread(m_shaderString,1,size,file);
    fclose(file);
  } else {
    m_shaderString = new char[strlen(buf) + 1];
    strcpy(m_shaderString,buf);
  }
  m_size=strlen(m_shaderString);
#ifdef GL_ARB_shader_objects
  if (!m_shader) m_shader = glCreateShaderObjectARB( m_shaderTarget );
  else
  {
    glDeleteObjectARB( m_shader );
	m_shader = glCreateShaderObjectARB( m_shaderTarget );
  }
  if (!m_shader)
  {
	post("[%s]: could not create shader object", m_objectname->s_name);
	return;
  }
  const char * vs = m_shaderString;
  glShaderSourceARB( m_shader, 1, &vs, NULL );
  glCompileShaderARB( m_shader );
  glGetObjectParameterivARB( m_shader, GL_OBJECT_COMPILE_STATUS_ARB, &m_compiled );
  if (!m_compiled) {
	GLint	length;
	GLcharARB* log;
	glGetObjectParameterivARB( m_shader, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length );
	log = (GLcharARB*)malloc( length * sizeof(GLcharARB) );
	glGetInfoLogARB( m_shader, length, NULL, log );
	post("[%s]: compile Info_log:", m_objectname->s_name );
	post("%s", log );
	post("[%s]: shader not loaded", m_objectname->s_name );
	free(log);
	return;
  }
  post("[%s]: Loaded file: %s", m_objectname->s_name, buf);
#endif
}

/////////////////////////////////////////////////////////
// startRendering
//
/////////////////////////////////////////////////////////
void glsl_vertex :: startRendering()
{
  if (m_shaderString == NULL)
    {
      error("[%s]: need to load a shader", m_objectname->s_name);
      return;
    }
}

/////////////////////////////////////////////////////////
// render
//
/////////////////////////////////////////////////////////
void glsl_vertex :: render(GemState *state)
{
  if (m_shader)
  {
    // send textureID to outlet
	outlet_float(m_outShaderID, (t_float)(unsigned int)m_shader);
  }
}

/////////////////////////////////////////////////////////
// postrender
//
/////////////////////////////////////////////////////////
void glsl_vertex :: postrender(GemState *state)
{
}

/////////////////////////////////////////////////////////
// printInfo
//
/////////////////////////////////////////////////////////
void glsl_vertex :: printInfo()
{
#ifdef __APPLE__
	if (!HaveValidContext ()) {
		post("GEM: vertex_shader - need window/context to load shader");
		return;
	}
#endif
#ifdef GL_ARB_vertex_shader
	GLint bitnum = 0;
	post("Vertex_shader Hardware Info");
	post("============================");
	glGetIntegerv( GL_MAX_VERTEX_ATTRIBS_ARB, &bitnum );
	post("MAX_VERTEX_ATTRIBS: %d", bitnum);
	glGetIntegerv( GL_MAX_VERTEX_UNIFORM_COMPONENTS_ARB, &bitnum );
	post("MAX_VERTEX_UNIFORM_COMPONENTS_ARB: %d", bitnum);
	glGetIntegerv( GL_MAX_VARYING_FLOATS_ARB, &bitnum );
	post("MAX_VARYING_FLOATS: %d", bitnum);
	glGetIntegerv( GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS_ARB, &bitnum );
	post("MAX_COMBINED_TEXTURE_IMAGE_UNITS: %d", bitnum);
	glGetIntegerv( GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS_ARB, &bitnum );
	post("MAX_VERTEX_TEXTURE_IMAGE_UNITS: %d", bitnum);
	glGetIntegerv( GL_MAX_TEXTURE_IMAGE_UNITS_ARB, &bitnum );
	post("MAX_TEXTURE_IMAGE_UNITS: %d", bitnum);
	glGetIntegerv( GL_MAX_TEXTURE_COORDS_ARB, &bitnum );
	post("MAX_TEXTURE_COORDS: %d", bitnum);
#endif /* GL_ARB_vertex_shader */
}

/////////////////////////////////////////////////////////
// static member function
//
/////////////////////////////////////////////////////////
void glsl_vertex :: obj_setupCallback(t_class *classPtr)
{
  class_addmethod(classPtr, (t_method)&glsl_vertex::openMessCallback,
		  gensym("open"), A_SYMBOL, A_NULL);
  class_addmethod(classPtr, (t_method)&glsl_vertex::printMessCallback,
		  gensym("print"), A_NULL);
}
void glsl_vertex :: openMessCallback(void *data, t_symbol *filename)
{
	    GetMyClass(data)->openMess(filename);
}
void glsl_vertex :: printMessCallback(void *data)
{
	GetMyClass(data)->printInfo();
}
