////////////////////////////////////////////////////////
//
// GEM - Graphics Environment for Multimedia
//
// tigital AT mac DOT com
//
// Implementation file
//
//    Copyright (c) 1997-1999 Mark Danks.
//    Copyright (c) G�nther Geiger.
//    Copyright (c) 2001-2011 IOhannes m zmoelnig. forum::f�r::uml�ute. IEM
//    Copyright (c) 2005-2006 James Tittle II, tigital At mac DoT com
//    For information on usage and redistribution, and for a DISCLAIMER OF ALL
//    WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.
//
/////////////////////////////////////////////////////////

#include "gemframebuffer.h"
#include <string.h>
#include "Base/GemState.h"
#include "Base/GLStack.h"

CPPEXTERN_NEW_WITH_TWO_ARGS(gemframebuffer, t_symbol *, A_DEFSYMBOL, t_symbol *, A_DEFSYMBOL)

/////////////////////////////////////////////////////////
//
// gemframebuffer
//
/////////////////////////////////////////////////////////
// Constructor
//
/////////////////////////////////////////////////////////
gemframebuffer :: gemframebuffer(t_symbol *format, t_symbol *type)
  : m_haveinit(false), m_wantinit(false), m_frameBufferIndex(0), m_depthBufferIndex(0),
    m_offScreenID(0), m_texTarget(GL_TEXTURE_2D), m_texunit(0),
    m_width(256), m_height(256),
    m_mode(0), m_internalformat(GL_RGB8), m_format(GL_RGB), m_type(GL_UNSIGNED_BYTE),
    m_outTexInfo(NULL)
{
  // create an outlet to send out texture info:
  //  - ID
  //  - width & height
  //  - format/type (ie. GL_TEXTURE_RECTANGLE or GL_TEXTURE_2D)
  //  - anything else?
  if(!m_outTexInfo)
    m_outTexInfo = outlet_new(this->x_obj, 0);

  m_FBOcolor[0] = 0.f;
  m_FBOcolor[1] = 0.f;
  m_FBOcolor[2] = 0.f;
  m_FBOcolor[3] = 0.f;

  m_perspect[0] = -1.f;
  m_perspect[1] = 1.f;	
  m_perspect[2] = -1.f;
  m_perspect[3] = 1.f;	
  m_perspect[4] = 1.f;
  m_perspect[5] = 20.f;	

  
  if(format && format->s_name && format!=gensym(""))
    formatMess(format->s_name);
  if(type   && type->s_name   && type  !=gensym(""))
    typeMess(type->s_name);
}

////////////////////////////////////////////////////////
// Destructor
//
/////////////////////////////////////////////////////////
gemframebuffer :: ~gemframebuffer()
{
  destroyFBO();
  outlet_free(m_outTexInfo);
}

////////////////////////////////////////////////////////
// extension check
//
/////////////////////////////////////////////////////////
bool gemframebuffer :: isRunnable() {
  if(!GLEW_VERSION_1_3) {
    error("openGL version 1.3 needed");
    return false;
  }

  if(GLEW_EXT_framebuffer_object)
    return true;

  error("openGL framebuffer extension is not supported by this system");

  return false;
}

////////////////////////////////////////////////////////
// renderGL
//
/////////////////////////////////////////////////////////
void gemframebuffer :: render(GemState *state)
{
  gem::GLStack*stacks=NULL;
  if(state) {
    state->get("gl.stacks", stacks);
  }

  if(!m_width || !m_height) {
    error("width and height must be present!");
  }
 
  glActiveTexture(GL_TEXTURE0_ARB + m_texunit);

  if (m_wantinit)
    initFBO();
  
  // store the window viewport dimensions so we can reset them,
  // and set the viewport to the dimensions of our texture
  glGetIntegerv(GL_VIEWPORT, m_vp);
  glGetFloatv( GL_COLOR_CLEAR_VALUE, m_color );
	
  glBindTexture( m_texTarget, 0 );
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_frameBufferIndex);
  // Bind the texture to the frame buffer.
  glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                            m_texTarget, m_offScreenID, 0);
  glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
                               GL_RENDERBUFFER_EXT, m_depthBufferIndex);
  
  // debug yellow color
  // glClearColor( 1,1,0,0);
  glClearColor( m_FBOcolor[0], m_FBOcolor[1], m_FBOcolor[2], m_FBOcolor[3] );
  
  // Clear the buffers and reset the model view matrix.
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // We need a one-to-one mapping of pixels to texels in order to
  // ensure every element of our texture is processed. By setting our
  // viewport to the dimensions of our destination texture and drawing
  // a screen-sized quad (see below), we ensure that every pixel of our
  // texel is generated and processed in the fragment program.		
  glViewport(0,0, m_width, m_height);

  if(stacks) stacks->push(gem::GLStack::PROJECTION);
  glLoadIdentity();
  glFrustum( m_perspect[0],  m_perspect[1],  m_perspect[2],  m_perspect[3], m_perspect[4], m_perspect[5]);

  if(stacks) stacks->push(gem::GLStack::MODELVIEW);
  glLoadIdentity();
}

////////////////////////////////////////////////////////
// postRender
//
/////////////////////////////////////////////////////////
void gemframebuffer :: postrender(GemState *state)
{
  t_float w, h;
  gem::GLStack*stacks=NULL;
  if(state) {
    state->get("gl.stacks", stacks);
  }

  glActiveTexture(GL_TEXTURE0_ARB + m_texunit);

  if(m_texTarget== GL_TEXTURE_2D) {
    w=1.f;
    h=1.f;
  } else {
    w=static_cast<t_float>(m_width);
    h=static_cast<t_float>(m_height);
  }

  // GPGPU CONCEPT 4: Viewport-Sized Quad = Data Stream Generator.
  // In order to execute fragment programs, we need to generate pixels.
  // Drawing a quad the size of our viewport (see above) generates a
  // fragment for every pixel of our destination texture. Each fragment
  // is processed identically by the fragment program. Notice that in
  // the reshape() function, below, we have set the frustum to
  // orthographic, and the frustum dimensions to [-1,1].  Thus, our
  // viewport-sized quad vertices are at [-1,-1], [1,-1], [1,1], and
  // [-1,1]: the corners of the viewport.

  glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 );
  glBindTexture( m_texTarget, m_offScreenID );

  if(stacks) stacks->pop(gem::GLStack::PROJECTION);
  if(stacks) stacks->pop(gem::GLStack::MODELVIEW);

  // reset to visible window's clear color
  glClearColor( m_color[0], m_color[1], m_color[2], m_color[3] );
  // reset to original viewport dimensions
  glViewport( m_vp[0], m_vp[1], m_vp[2], m_vp[3] );
  // now that the render is done,

  // send textureID, w, h, textureTarget to outlet
  t_atom ap[5];
  SETFLOAT(ap+0, static_cast<t_float>(m_offScreenID));
  SETFLOAT(ap+1, w);
  SETFLOAT(ap+2, h);
  SETFLOAT(ap+3, m_texTarget);
  SETFLOAT(ap+4, static_cast<t_float>(0.));
  outlet_list(m_outTexInfo, 0, 5, ap);
}

void gemframebuffer :: printInfo()
{
  if(m_mode)
    post("using rectmode 1:GL_TEXTURE_RECTANGLE");
  else post("using rectmode 0:GL_TEXTURE_2D");

  switch(m_type) {
  case GL_UNSIGNED_BYTE: post("using type: BYTE"); break;
  case GL_FLOAT: post("using type: FLOAT"); break;
  default: post("using type: unknown(%d)", m_type);
  }

  switch(m_format) {
  case GL_YUV422_GEM: post("using color: YUV"); break;
  case GL_RGB: post("using color: RGB"); break;
  case GL_RGBA: post("using color: RGBA"); break;
  case GL_BGRA: post("using color: BGRA"); break;
  }

  post("using texunit: %d", m_texunit);
}

/////////////////////////////////////////////////////////
// initFBO
//
/////////////////////////////////////////////////////////
void gemframebuffer :: initFBO()
{
  // clean up any existing FBO before creating a new one
  if(m_haveinit)
    destroyFBO();
	
  if ( !m_mode )
    m_texTarget = GL_TEXTURE_2D;
  else
    m_texTarget = GL_TEXTURE_RECTANGLE_EXT;

  // Generate frame buffer object then bind it.
  glGenFramebuffersEXT(1, &m_frameBufferIndex);
  glGenRenderbuffersEXT(1, &m_depthBufferIndex);

  // Create the texture we will be using to render to.
  glGenTextures(1, &m_offScreenID);
  glBindTexture(m_texTarget, m_offScreenID);

  glTexImage2D( m_texTarget, 0, m_internalformat, m_width, m_height, 0, m_format, m_type, NULL );
  // 2.13.2006
  // GL_LINEAR causes fallback to software shader
  // so switching back to GL_NEAREST
  glTexParameteri(m_texTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(m_texTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf(m_texTarget, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameterf(m_texTarget, GL_TEXTURE_WRAP_T, GL_CLAMP);
  
  // Initialize the render buffer.
  glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, m_depthBufferIndex);
  glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, m_width, m_height);

  // Make sure we have not errors.
  GLenum status = glCheckFramebufferStatusEXT( GL_FRAMEBUFFER_EXT) ;
  if( status != GL_FRAMEBUFFER_COMPLETE_EXT )
    {
      switch(status) {                                          
      case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
        error("GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT");
        break;
      case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
        error("GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT");
        break;
      case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
        error("GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT");
        break;
      case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
        error("GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT");
        break;
      case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
        error("GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT");
        break;
      case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
        error("GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT");
        break;
      case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
        error("GL_FRAMEBUFFER_UNSUPPORTED_EXT");
        break;
      case GL_INVALID_FRAMEBUFFER_OPERATION_EXT:
        error("GL_INVALID_FRAMEBUFFER_OPERATION_EXT");
        break;
      default:
        error("Unknown ERROR %d", status);
      }
      return;
    }

  // Return out of the frame buffer.
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

  m_haveinit = true;
  m_wantinit = false;

  printInfo();
}

////////////////////////////////////////////////////////
// destroyFBO
//
/////////////////////////////////////////////////////////
void gemframebuffer :: destroyFBO()
{
  if(!GLEW_EXT_framebuffer_object)
    return;
  // Release all resources.
  if(m_depthBufferIndex) glDeleteRenderbuffersEXT(1, &m_depthBufferIndex);
  if(m_frameBufferIndex) glDeleteFramebuffersEXT(1, &m_frameBufferIndex);
  if(m_offScreenID) glDeleteTextures(1, &m_offScreenID);

  m_haveinit = false;
}

////////////////////////////////////////////////////////
// bangMess
//
/////////////////////////////////////////////////////////
void gemframebuffer :: bangMess()
{

}

////////////////////////////////////////////////////////
// startRendering
//
/////////////////////////////////////////////////////////
void gemframebuffer :: startRendering()
{
  m_wantinit = true;
}

////////////////////////////////////////////////////////
// stopRendering
//
/////////////////////////////////////////////////////////
void gemframebuffer :: stopRendering()
{
  destroyFBO();
}
////////////////////////////////////////////////////////
// dimMess
//
/////////////////////////////////////////////////////////
void gemframebuffer :: dimMess(int width, int height)
{
  if (width != m_width || height != m_height)
    {
      m_width = width;
      m_height = height;
      m_wantinit=true;
      setModified();
    }
}

void gemframebuffer :: colorMess(float red, float green, float blue, float alpha)
{
  m_FBOcolor[0] = red;
  m_FBOcolor[1] = green;
  m_FBOcolor[2] = blue;
  m_FBOcolor[3] = alpha;
    
  setModified();
}

void gemframebuffer :: perspectiveMess(float f_left, float f_right, 
                                       float f_bottom, float f_top,
                                       float f_near, float f_far)
{
  m_perspect[0] = f_left;
  m_perspect[1] = f_right;
  m_perspect[2] = f_bottom;
  m_perspect[3] = f_top;
  m_perspect[4] = f_near;
  m_perspect[5] = f_far;
    
  setModified();
}

void gemframebuffer :: formatMess(const char* fmt)
{
  std::string format=fmt;
  GLenum tmp_format=0;
  if("YUV"==format) {
    tmp_format = GL_YUV422_GEM;
  } else if ("RGB"==format) {
    tmp_format = GL_RGB;
  } else if ("RGBA"==format) {
    tmp_format = GL_RGBA;
  } else if ("RGB32"==format) {
    if(GLEW_ATI_texture_float) {
      tmp_format =  GL_RGB_FLOAT32_ATI;
    }
  }

  m_type = GL_UNSIGNED_BYTE;
  switch(tmp_format) {
  default:
    post("using default format");
    format="RGB";
  case GL_RGB:
    m_internalformat=m_format=GL_RGB;
    break;
  case  GL_RGB_FLOAT32_ATI:
  m_internalformat = GL_RGB_FLOAT32_ATI;
  m_format = GL_RGB;
  format="RGB_FLOAT32_ATI";
  break;
  case GL_RGBA:
    m_internalformat = GL_RGBA;
    m_format = GL_RGBA;
    break;
  case  GL_YUV422_GEM:
    m_format=GL_YUV422_GEM;
    m_internalformat=GL_RGB8;
    break;
  }

#ifdef __APPLE__
  switch(tmp_format) {
  case  GL_RGB_FLOAT32_ATI;
  m_format = GL_BGR;
  break;
  case GL_RGBA:
    m_format = GL_BGRA;
    break;
  case GL_YUV422_GEM:
    m_type = GL_UNSIGNED_SHORT_8_8_REV_APPLE;
    break;
  default:
    break;
  }
#endif

  post("format is '%s'(%d)", format.c_str(), m_format);

  // changed format, so we need to rebuild the FBO
  m_wantinit=true;
}

void gemframebuffer :: typeMess(const char* typ)
{
  std::string type=typ;
  if("FLOAT"==type) {
    m_type = GL_FLOAT;
  } else {
    type="BYTE";
    m_type=GL_UNSIGNED_BYTE;
  }
  post("type is '%s'(%d)", type.c_str(), m_type);

  // changed type, so we need to rebuild the FBO
  m_wantinit=true;
}

////////////////////////////////////////////////////////
// static member function
//
////////////////////////////////////////////////////////
void gemframebuffer :: obj_setupCallback(t_class *classPtr)
{
  class_addbang(classPtr, reinterpret_cast<t_method>(&gemframebuffer::bangMessCallback));
  class_addmethod(classPtr, reinterpret_cast<t_method>(&gemframebuffer::modeCallback),
                  gensym("mode"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, reinterpret_cast<t_method>(&gemframebuffer::modeCallback),
                  gensym("rectangle"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, reinterpret_cast<t_method>(&gemframebuffer::dimMessCallback),
                  gensym("dimen"), A_FLOAT, A_FLOAT, A_NULL);
  class_addmethod(classPtr, reinterpret_cast<t_method>(&gemframebuffer::dimMessCallback),
                  gensym("dim"), A_FLOAT, A_FLOAT, A_NULL);
  class_addmethod(classPtr, reinterpret_cast<t_method>(&gemframebuffer::formatMessCallback),
                  gensym("format"), A_DEFSYMBOL, A_NULL);
  class_addmethod(classPtr, reinterpret_cast<t_method>(&gemframebuffer::typeMessCallback),
                  gensym("type"), A_DEFSYMBOL, A_NULL);
  class_addmethod(classPtr, reinterpret_cast<t_method>(&gemframebuffer::colorMessCallback),
                  gensym("color"), A_GIMME, A_NULL);
  class_addmethod(classPtr, reinterpret_cast<t_method>(&gemframebuffer::texunitCallback),
                  gensym("texunit"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, reinterpret_cast<t_method>(&gemframebuffer::perspectiveMessCallback),
		  gensym("perspec"), A_GIMME, A_NULL);
}
void gemframebuffer :: bangMessCallback(void *data)
{
  GetMyClass(data)->bangMess();
}
void gemframebuffer :: modeCallback(void *data, t_floatarg quality)
{
  GetMyClass(data)->m_mode=(static_cast<int>(quality));
  // changed mode, so we need to rebuild the FBO
  GetMyClass(data)->m_wantinit=true;
}
void gemframebuffer :: dimMessCallback(void *data, t_floatarg width, t_floatarg height)
{
  GetMyClass(data)->dimMess(static_cast<int>(width), static_cast<int>(height));
}
void gemframebuffer :: formatMessCallback (void *data, t_symbol *format)
{
  GetMyClass(data)->formatMess(format->s_name);
}
void gemframebuffer :: typeMessCallback (void *data, t_symbol *type)
{
  GetMyClass(data)->typeMess(type->s_name);
}

void gemframebuffer :: colorMessCallback(void *data, t_symbol*s, int argc, t_atom*argv)
{
  float red=1., green=1., blue=1., alpha=1.;
  switch(argc) {
  case (4):
    alpha=atom_getfloat(argv+3);
  case (3):
    red =atom_getfloat(argv+0);
    green=atom_getfloat(argv+1);
    blue =atom_getfloat(argv+2);
    break;
  default:
    GetMyClass(data)->error("'color' message takes 3 (RGB) or 4 (RGBA) values");
    return;
  }

  GetMyClass(data)->colorMess(red, green, blue, alpha);
}

void gemframebuffer :: texunitCallback(void *data, t_floatarg unit)
{
  GetMyClass(data)->m_texunit=static_cast<GLuint>(unit);
}

void gemframebuffer :: perspectiveMessCallback(void *data, t_symbol*s,int argc, t_atom*argv)
{
  t_float f_left, f_right, f_bottom, f_top, f_near, f_far;
  switch(argc){
  case 6:
    f_left=  atom_getfloat(argv);
    f_right=atom_getfloat(argv+1);
    f_bottom= atom_getfloat(argv+2);
    f_top=  atom_getfloat(argv+3);
    f_near=atom_getfloat(argv+4);
    f_far= atom_getfloat(argv+5);
    GetMyClass(data)->perspectiveMess(
				      static_cast<float>(f_left), 
				      static_cast<float>(f_right), 
				      static_cast<float>(f_bottom), 
				      static_cast<float>(f_top), 
				      static_cast<float>(f_near),
				      static_cast<float>(f_far));
    break;
  default:
    GetMyClass(data)->error("\"perspec\" expects 6 values for frustum - left, right, bottom, top, near, far");
  }
}
