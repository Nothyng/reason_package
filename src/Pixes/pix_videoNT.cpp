////////////////////////////////////////////////////////
//
// GEM - Graphics Environment for Multimedia
//
// zmoelnig@iem.kug.ac.at
//
// Implementation file 
//
//    Copyright (c) 1997-1999 Mark Danks.
//    Copyright (c) G�nther Geiger.
//    Copyright (c) 2001-2002 IOhannes m zmoelnig. forum::f�r::uml�ute. IEM
//    For information on usage and redistribution, and for a DISCLAIMER OF ALL
//    WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.
//
/////////////////////////////////////////////////////////

#ifdef _WINDOWS

#include "pix_videoNT.h"

CPPEXTERN_NEW_WITH_ONE_ARG(pix_videoNT, t_floatarg, A_DEFFLOAT)

  /////////////////////////////////////////////////////////
//
// pix_videoNT
//
/////////////////////////////////////////////////////////
// Constructor
//
/////////////////////////////////////////////////////////
pix_videoNT :: pix_videoNT(t_floatarg num)
  : m_haveVideo(0), m_swap(1), m_colorSwap(0),
    m_hWndC(NULL), m_newFrame(0)
{
  char driverName[64];
  char driverDesc[64];
  if (capGetDriverDescription(0, driverName, 64, driverDesc, 0))
    post("pix_video: driver '%s'", driverName);
  else post("couldn't determine video-driver name");
  
  m_pixBlock.image.data = NULL;
    
  // set to half PAL
  if (num)
    {
      m_vidXSize = 320;
      m_vidYSize = 240;
    }
  // set to half NTSC
  else
    {
      m_vidXSize = 320;
      m_vidYSize = 240;
    }

  // Connect to the daemon
  m_hWndC = capCreateCaptureWindow ((LPSTR) "GEM video",	// window name if pop-up 
				    0,				// window style (not visible)
				    0, 0, m_vidXSize, m_vidYSize,// window position and dimensions
				    GetDesktopWindow(), 0);
  if (!m_hWndC) {
    error("GEM: pix_videoNT: Unable to create capture window");
    return;
  } 
#if 1
int index=0;
boolean result=false;
while(!result && index<10){
	post("driver %d", index);
	capDriverConnect(m_hWndC, index);
	index++;
}
if (!result){
#else
  if (!capDriverConnect(m_hWndC, 0)) {
#endif
	  error("GEM: pix_videoNT: Unable to connect to video driver");
    return;
  }


  CAPTUREPARMS params;
  if (!capCaptureGetSetup(m_hWndC, &params, sizeof(CAPTUREPARMS))) {
    error("GEM: pix_videoNT: Unable to get capture parameters");
    return;
  }
  params.fYield = TRUE;
  params.fCaptureAudio = FALSE;
  params.wPercentDropForError = 100;
  params.fLimitEnabled = FALSE;
  params.AVStreamMaster = AVSTREAMMASTER_NONE;
  params.fStepCaptureAt2x = FALSE;
  params.fAbortLeftMouse = FALSE;
  params.fAbortRightMouse = FALSE;
  if (!capCaptureSetSetup(m_hWndC, &params, sizeof(CAPTUREPARMS)))
    {
      error("GEM: pix_videoNT: Unable to set capture parameters");
      return;
    }

  if (!capSetCallbackOnVideoStream(m_hWndC, pix_videoNT::videoFrameCallback))
    {
      error("GEM: pix_videoNT: Unable to set frame callback");
      return;
    }
  if (!capSetUserData(m_hWndC, this))
    {
      error("GEM: pix_videoNT: Unable to set user data");
      return;
    }
  DWORD formSize = capGetVideoFormat(m_hWndC, NULL, 0);
  BITMAPINFO *videoFormat = (BITMAPINFO *)(new char[formSize]);
  if (!capGetVideoFormat(m_hWndC, videoFormat, formSize))
    {
      error("GEM: pix_videoNT: Unable to get video format");
      return;
    }

  videoFormat->bmiHeader.biWidth = m_vidXSize;
  videoFormat->bmiHeader.biHeight = m_vidYSize;
  videoFormat->bmiHeader.biBitCount = 24;
  videoFormat->bmiHeader.biCompression = BI_RGB;
  videoFormat->bmiHeader.biClrUsed = 0;
  videoFormat->bmiHeader.biClrImportant = 0;
  videoFormat->bmiHeader.biSizeImage = 0;
  if (!capSetVideoFormat(m_hWndC, videoFormat, formSize))
    {
      error("GEM: pix_video: Unable to set video format");
      return;
    }
  post("GEM: pix_video: Connected at x: %d, y: %d, c: %d",
       (int)(videoFormat->bmiHeader.biWidth),
       (int)(videoFormat->bmiHeader.biHeight),
       (int)(videoFormat->bmiHeader.biBitCount));
  delete videoFormat;

  m_pixBlock.image.xsize = m_vidXSize;  // was defaulted to 128 ???
  m_pixBlock.image.ysize = m_vidYSize;  // was defaulted to 128 ???
  m_pixBlock.image.csize = 4;
  m_pixBlock.image.format = GL_RGBA;
  m_pixBlock.image.type = GL_UNSIGNED_BYTE;
  const int dataSize = m_pixBlock.image.xsize * m_pixBlock.image.ysize
    * m_pixBlock.image.csize * sizeof(unsigned char);
  m_pixBlock.image.data = new unsigned char[dataSize];
  memset(m_pixBlock.image.data, 255, dataSize);

  m_haveVideo = 1;
}

/////////////////////////////////////////////////////////
// Destructor
//
/////////////////////////////////////////////////////////
pix_videoNT :: ~pix_videoNT()
{
  if (!m_hWndC)
    return;

  stopTransfer();
  capDriverDisconnect(m_hWndC);
  DestroyWindow(m_hWndC);

  // Clean up the video
  if (m_haveVideo)
    {
      post("GEM: pix_video: Closed video connection");
    }
  cleanPixBlock();
}

/////////////////////////////////////////////////////////
// videoFrame
//
/////////////////////////////////////////////////////////
void pix_videoNT :: videoFrame(LPVIDEOHDR lpVHdr)
{
  int count = lpVHdr->dwBytesUsed;
  // notice that it is times 3 for the color!
  const int dataSize = m_pixBlock.image.xsize * m_pixBlock.image.ysize * 3;
	
  if (count < dataSize)
    {
      error("GEM: pix_videoNT: not enough pixels captured");
      return;
    }

  const unsigned char *srcLine = lpVHdr->lpData;
  unsigned char *dstLine = m_pixBlock.image.data;
  const int srcXSize = m_vidXSize * 3;
  const int dstXSize = m_pixBlock.image.xsize * 4;
  const int xSize = m_pixBlock.image.xsize;
  const int ySize = m_pixBlock.image.ysize;

  for (int i = 0; i < ySize; ++i)
    {
      const unsigned char *srcPix = srcLine;
      unsigned char *dstPix = dstLine;
      for (int j = 0; j < xSize; ++j)
	{
	  dstPix[chRed] = srcPix[2];
	  dstPix[chGreen] = srcPix[1];
	  dstPix[chBlue] = srcPix[0];
	  // remember that we memset the data to 255
	  //			dstPix[chAlpha] = 255;
	  dstPix += 4;
	  srcPix += 3;
	}
      srcLine += srcXSize;
      dstLine += dstXSize;
    }
  m_newFrame = 1;
}

/////////////////////////////////////////////////////////
// render
//
/////////////////////////////////////////////////////////
void pix_videoNT :: render(GemState *state)
{
  if (!m_haveVideo)
    {
      error("GEM: pix_videoNT: Video not active");
      return;
    }

  m_pixBlock.newimage = m_newFrame;
  state->image = &m_pixBlock;
  m_newFrame = 0;
}

/////////////////////////////////////////////////////////
// startRendering
//
/////////////////////////////////////////////////////////
void pix_videoNT :: startRendering()
{
  if (!capCaptureSequenceNoFile(m_hWndC))
    {
      error("GEM: pix_videoNT: Unable to start capture");
      return;
    }

  m_pixBlock.newimage = 1;
}

/////////////////////////////////////////////////////////
// stopRendering
//
/////////////////////////////////////////////////////////
void pix_videoNT :: stopRendering()
{
  capCaptureStop(m_hWndC);
}

/////////////////////////////////////////////////////////
// postrender
//
/////////////////////////////////////////////////////////
void pix_videoNT :: postrender(GemState *state)
{
  m_pixBlock.newimage = 0;
  state->image = NULL;
}

/////////////////////////////////////////////////////////
// startTransfer
//
/////////////////////////////////////////////////////////
int pix_videoNT :: startTransfer()
{
  if (!m_haveVideo)
    return(0);
  return(1);
}

/////////////////////////////////////////////////////////
// stopTransfer
//
/////////////////////////////////////////////////////////
int pix_videoNT :: stopTransfer()
{
  if ( !m_haveVideo )
    return(0);

  // Clean up the buffer
    
  return(1);
}

/////////////////////////////////////////////////////////
// offsetMess
//
/////////////////////////////////////////////////////////
void pix_videoNT :: offsetMess(int x, int y)
{
  post("GEM: pix_video: offset message not supported");
  return;

  if (!m_haveVideo)
    {
      error("GEM: pix_video: Connect to video first");
      return;
    }
    
  // stop the transfer and destroy the buffer
  if ( !stopTransfer() ) 
    {
      error("GEM: pix_video: error stopping transfer");
      return;
    }

  // start the transfer and rebuild the buffer
  if ( !startTransfer() ) 
    {
      error("GEM: pix_video: error starting transfer");
      return;
    }
}

/////////////////////////////////////////////////////////
// dimenMess
//
/////////////////////////////////////////////////////////
void pix_videoNT :: dimenMess(int x, int y)
{
  if (!m_haveVideo)
    {
      error("GEM: pix_videoNT: Connect to video first");
      return;
    }
    
  // stop the transfer and destroy the buffer
  if ( !stopTransfer() ) 
    {
      error("GEM: pix_videoNT: error stopping transfer");
      return;
    }

  m_pixBlock.image.xsize = x;
  m_pixBlock.image.ysize = y;

  cleanPixBlock();
  int dataSize = m_pixBlock.image.xsize * m_pixBlock.image.ysize
    * 4 * sizeof(unsigned char);
  m_pixBlock.image.data = new unsigned char[dataSize];
  memset(m_pixBlock.image.data, 255, dataSize);

  // start the transfer and rebuild the buffer
  if ( !startTransfer() ) 
    {
      error("GEM: pix_videoNT: error starting transfer");
      return;
    }
}

/////////////////////////////////////////////////////////
// cleanPixBlock
//
/////////////////////////////////////////////////////////
void pix_videoNT :: cleanPixBlock()
{
  delete [] m_pixBlock.image.data;
  m_pixBlock.image.data = NULL;
}

/////////////////////////////////////////////////////////
// swapMess
//
/////////////////////////////////////////////////////////
void pix_videoNT :: swapMess(int state)
{
  post("GEM: pix_video: swap message not supported");
  return;

  if (state)
    m_swap = 1;
  else
    m_swap = 0;
}

/////////////////////////////////////////////////////////
// static member function
//
/////////////////////////////////////////////////////////
void pix_videoNT :: obj_setupCallback(t_class *classPtr)
{
  class_addcreator((t_newmethod)_classpix_videoNT, gensym("pix_video"), A_DEFFLOAT, A_NULL);
  pix_video::real_obj_setupCallback(classPtr);

  class_addmethod(classPtr, (t_method)&pix_videoNT::dimenMessCallback,
		  gensym("dimen"), A_FLOAT, A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_videoNT::offsetMessCallback,
		  gensym("offset"), A_FLOAT, A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_videoNT::swapMessCallback,
		  gensym("swap"), A_FLOAT, A_NULL);
}
void pix_videoNT :: dimenMessCallback(void *data, t_floatarg x, t_floatarg y)
{
  GetMyClass(data)->dimenMess((int)x, (int)y);
}
void pix_videoNT :: offsetMessCallback(void *data, t_floatarg x, t_floatarg y)
{
  GetMyClass(data)->offsetMess((int)x, (int)y);
}
void pix_videoNT :: swapMessCallback(void *data, t_floatarg state)
{
  GetMyClass(data)->swapMess((int)state);
}

void pix_videoNT :: videoFrameCallback(HWND hWnd, LPVIDEOHDR lpVHdr) 
{
  void *ptr = (void *)(capGetUserData(hWnd));
  ((pix_videoNT *)ptr)->videoFrame(lpVHdr);
}

#endif
