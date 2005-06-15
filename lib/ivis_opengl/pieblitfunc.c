/***************************************************************************/
/*
 * pieBlitFunc.c
 *
 * patch for exisitng ivis rectangle draw functions.
 *
 */
/***************************************************************************/

#include <time.h>
#ifdef _MSC_VER	
#include <windows.h>  //needed for gl.h!  --Qamly
#endif
#include <GL/gl.h>

#include "frame.h"
#include "pieblitfunc.h"
#include "bug.h"
#include "piedef.h"
#include "piemode.h"
#include "piestate.h"
#include "rendfunc.h"
#include "rendmode.h"
#include "pcx.h"
#include "pieclip.h"
#include "piefunc.h"
#include "piematrix.h"
#include "screen.h"

extern BOOL dtm_LoadRadarSurface(BYTE* radarBuffer);
extern SDWORD dtm_GetRadarTexImageSize(void);

/***************************************************************************/
/*
 *	Local Definitions
 */
/***************************************************************************/
UWORD	backDropBmp[BACKDROP_WIDTH * BACKDROP_HEIGHT * 4];
SDWORD gSurfaceOffsetX;
SDWORD gSurfaceOffsetY;
UWORD* pgSrcData = NULL;
SDWORD gSrcWidth;
SDWORD gSrcHeight;
SDWORD gSrcStride;

#define COLOURINTENSITY 0xffffffff
/***************************************************************************/
/*
 *	Local Variables
 */
/***************************************************************************/

PIESTYLE	rendStyle;
POINT		rectVerts[4];

/***************************************************************************/
/*
 *	Local ProtoTypes
 */
/***************************************************************************/

/***************************************************************************/
/*
 *	Source
 */
/***************************************************************************/
void pie_Line(int x0, int y0, int x1, int y1, uint32 colour)
{
//	PIELIGHT light;

	pie_SetRendMode(REND_FLAT);
	pie_SetColour(colour);
	pie_SetTexturePage(-1);
	pie_SetColourKeyedBlack(FALSE);

	glBegin(GL_LINE_STRIP);
	glVertex2f(x0, y0);
	glVertex2f(x1, y1);
	glEnd();
}
/***************************************************************************/

void pie_Box(int x0,int y0, int x1, int y1, uint32 colour)
{
//	PIELIGHT light;
//	iColour* psPalette;

	pie_SetRendMode(REND_FLAT);
	pie_SetColour(colour);
	pie_SetTexturePage(-1);
	pie_SetColourKeyedBlack(FALSE);

	if (x0>psRendSurface->clip.right || x1<psRendSurface->clip.left ||
		y0>psRendSurface->clip.bottom || y1<psRendSurface->clip.top)
	return;

	if (x0<psRendSurface->clip.left)
		x0 = psRendSurface->clip.left;
	if (x1>psRendSurface->clip.right)
		x1 = psRendSurface->clip.right;
	if (y0<psRendSurface->clip.top)
		y0 = psRendSurface->clip.top;
	if (y1>psRendSurface->clip.bottom)
		y1 = psRendSurface->clip.bottom;

	glBegin(GL_LINE_STRIP);
	glVertex2f(x0, y0);
	glVertex2f(x1, y0);
	glVertex2f(x1, y1);
	glVertex2f(x0, y1);
	glVertex2f(x0, y0);
	glEnd();
}

/***************************************************************************/

void pie_BoxFillIndex(int x0,int y0, int x1, int y1, UBYTE colour)
{
	PIELIGHT light;
	iColour* psPalette;

	pie_SetRendMode(REND_FLAT);
	pie_SetTexturePage(-1);

	if (x0>psRendSurface->clip.right || x1<psRendSurface->clip.left ||
		y0>psRendSurface->clip.bottom || y1<psRendSurface->clip.top)
	return;

	if (x0<psRendSurface->clip.left)
		x0 = psRendSurface->clip.left;
	if (x1>psRendSurface->clip.right)
		x1 = psRendSurface->clip.right;
	if (y0<psRendSurface->clip.top)
		y0 = psRendSurface->clip.top;
	if (y1>psRendSurface->clip.bottom)
		y1 = psRendSurface->clip.bottom;

	psPalette = pie_GetGamePal();
	light.byte.r = psPalette[colour].r;
	light.byte.g = psPalette[colour].g;
	light.byte.b = psPalette[colour].b;
	light.byte.a = MAX_UB_LIGHT;
	pie_DrawRect(x0, y0, x1, y1, light.argb, FALSE);
}

void pie_BoxFill(int x0,int y0, int x1, int y1, uint32 colour)
{
	pie_SetRendMode(REND_FLAT);
	pie_SetTexturePage(-1);

	if (x0>psRendSurface->clip.right || x1<psRendSurface->clip.left ||
		y0>psRendSurface->clip.bottom || y1<psRendSurface->clip.top)
	return;

	if (x0<psRendSurface->clip.left)
		x0 = psRendSurface->clip.left;
	if (x1>psRendSurface->clip.right)
		x1 = psRendSurface->clip.right;
	if (y0<psRendSurface->clip.top)
		y0 = psRendSurface->clip.top;
	if (y1>psRendSurface->clip.bottom)
		y1 = psRendSurface->clip.bottom;

	pie_DrawRect(x0, y0, x1, y1, colour, FALSE);

}
/***************************************************************************/

void pie_TransBoxFill(SDWORD x0, SDWORD y0, SDWORD x1, SDWORD y1)
{
	UDWORD rgb;
	UDWORD transparency;
	rgb = (pie_FILLRED<<16) | (pie_FILLGREEN<<8) | pie_FILLBLUE;//blue
	transparency = pie_FILLTRANS;
	pie_UniTransBoxFill(x0, y0, x1, y1, rgb, transparency);
//	pie_doWeirdBoxFX(x0,y0,x1,y1);

}

/***************************************************************************/
void pie_UniTransBoxFill(SDWORD x0,SDWORD y0, SDWORD x1, SDWORD y1, UDWORD rgb, UDWORD transparency)
{
	UDWORD light;

	if (x0>psRendSurface->clip.right || x1<psRendSurface->clip.left ||
		y0>psRendSurface->clip.bottom || y1<psRendSurface->clip.top)
	return;

	if (x0<psRendSurface->clip.left)
		x0 = psRendSurface->clip.left;
	if (x1>psRendSurface->clip.right)
		x1 = psRendSurface->clip.right;
	if (y0<psRendSurface->clip.top)
		y0 = psRendSurface->clip.top;
	if (y1>psRendSurface->clip.bottom)
		y1 = psRendSurface->clip.bottom;

	if (transparency == 0 ) {
		transparency = 127;
	}
	pie_SetTexturePage(-1);
	pie_SetRendMode(REND_ALPHA_FLAT);
	light = (rgb & 0x00ffffff) + (transparency << 24);
	pie_DrawRect(x0, y0, x1, y1, light, FALSE);
}

/***************************************************************************/

void pie_DrawImageFileID(IMAGEFILE *ImageFile, UWORD ID, int x, int y)
{
	IMAGEDEF *Image;
//	iBitmap *bmp;
//	UDWORD modulus;
	PIEIMAGE pieImage;
	PIERECT dest;
//	SDWORD width;
//	SDWORD height;
//	SDWORD delta;

	assert(ID < ImageFile->Header.NumImages);
	Image = &ImageFile->ImageDefs[ID];

	pieImage.texPage = ImageFile->TPageIDs[Image->TPageID];
	pieImage.tu = Image->Tu;
	pieImage.tv = Image->Tv;
	pieImage.tw = Image->Width;
	pieImage.th = Image->Height;
	dest.x = x+Image->XOffset;
	dest.y = y+Image->YOffset;
	dest.w = Image->Width;
	dest.h = Image->Height;
	pie_DrawImage(&pieImage, &dest, &rendStyle);
}

BOOL	bAddSprites = FALSE;
UDWORD	addSpriteLevel;

void	pie_SetAdditiveSprites(BOOL val) {
	bAddSprites = val;
}

void	pie_SetAdditiveSpriteLevel(UDWORD val) {
	addSpriteLevel = val;
}

BOOL	pie_GetAdditiveSprites( void ) {
	return bAddSprites;
}

void pie_ImageFileID(IMAGEFILE *ImageFile, UWORD ID, int x, int y)
{
	IMAGEDEF *Image;
//	iBitmap *bmp;
//	UDWORD modulus;
	PIEIMAGE pieImage;
	PIERECT dest;
//	SDWORD width;
//	SDWORD height;
//	SDWORD delta;
//	SDWORD div,wave;

	assert(ID < ImageFile->Header.NumImages);
	Image = &ImageFile->ImageDefs[ID];

   	if(pie_GetAdditiveSprites()) {
		pie_SetBilinear(TRUE);
		pie_SetRendMode(REND_ALPHA_TEX);
		pie_SetColour(addSpriteLevel);

	} else {
		pie_SetBilinear(FALSE);
		pie_SetRendMode(REND_GOURAUD_TEX);
		pie_SetColour(COLOURINTENSITY);
	}
	pie_SetColourKeyedBlack(TRUE);

	pieImage.texPage = ImageFile->TPageIDs[Image->TPageID];
	pieImage.tu = Image->Tu;
	pieImage.tv = Image->Tv;
	pieImage.tw = Image->Width;
	pieImage.th = Image->Height;
	dest.x = x+Image->XOffset;
	dest.y = y+Image->YOffset;
	dest.w = Image->Width;
	dest.h = Image->Height;
	pie_DrawImage(&pieImage, &dest, &rendStyle);
}


/***************************************************************************/

void pie_ImageFileIDTile(IMAGEFILE *ImageFile,UWORD ID,int x,int y,int x0,int y0,int Width,int Height)
{
	IMAGEDEF *Image;
	SDWORD hRep, hRemainder, vRep, vRemainder;
	PIEIMAGE pieImage;
	PIERECT dest;
	assert(ID < ImageFile->Header.NumImages);

	assert(x0 == 0);
	assert(y0 == 0);

	Image = &ImageFile->ImageDefs[ID];

	pie_SetBilinear(FALSE);
	pie_SetRendMode(REND_GOURAUD_TEX);
	pie_SetColour(COLOURINTENSITY);
	pie_SetColourKeyedBlack(TRUE);

	pieImage.texPage = ImageFile->TPageIDs[Image->TPageID];
	pieImage.tu = Image->Tu;
	pieImage.tv = Image->Tv;
	pieImage.tw = Image->Width;
	pieImage.th = Image->Height;

	dest.x = x+Image->XOffset;
	dest.y = y+Image->YOffset;
	dest.w = Image->Width;
	dest.h = Image->Height;

	vRep = Height/Image->Height;
	vRemainder = Height - (vRep * Image->Height);

	while(vRep > 0) {
		hRep = Width/Image->Width;
		hRemainder = Width - (hRep * Image->Width);
		pieImage.tw = Image->Width;
		dest.x = x+Image->XOffset;
		dest.w = Image->Width;
		while(hRep > 0) {
			pie_DrawImage(&pieImage, &dest, &rendStyle);
			hRep --;
			dest.x += Image->Width;
		}
		//draw remainder
		if (hRemainder > 0) {
			pieImage.tw = hRemainder;
			dest.w = hRemainder;
			pie_DrawImage(&pieImage, &dest, &rendStyle);
		}
		vRep --;
		dest.y += Image->Height;
	}
	//draw remainder
	if (vRemainder > 0) {
		hRep = Width/Image->Width;
		hRemainder = Width - (hRep * Image->Width);
		pieImage.th = vRemainder;
		dest.h = vRemainder;
		//as above
		{
			pieImage.tw = Image->Width;
			dest.x = x+Image->XOffset;
			dest.w = Image->Width;
			while(hRep > 0)
			{
				pie_DrawImage(&pieImage, &dest, &rendStyle);
				hRep --;
				dest.x += Image->Width;
			}
			//draw remainder
			if (hRemainder > 0)
			{
				pieImage.tw = hRemainder;
				dest.w = hRemainder;
				pie_DrawImage(&pieImage, &dest, &rendStyle);
			}
		}
	}
}

void pie_ImageFileIDStretch(IMAGEFILE *ImageFile,UWORD ID,int x,int y,int Width,int Height)
{
	IMAGEDEF *Image;
	PIEIMAGE pieImage;
	PIERECT dest;
	assert(ID < ImageFile->Header.NumImages);

	Image = &ImageFile->ImageDefs[ID];

	pie_SetBilinear(FALSE);
	pie_SetRendMode(REND_GOURAUD_TEX);
	pie_SetColour(COLOURINTENSITY);
	pie_SetColourKeyedBlack(TRUE);

	pieImage.texPage = ImageFile->TPageIDs[Image->TPageID];
	pieImage.tu = Image->Tu;
	pieImage.tv = Image->Tv;
	pieImage.tw = Image->Width;
	pieImage.th = Image->Height;

	dest.x = x+Image->XOffset;
	dest.y = y+Image->YOffset;
	dest.w = Width;
	dest.h = Height;
	pie_DrawImage(&pieImage, &dest, &rendStyle);
}

void pie_ImageDef(IMAGEDEF *Image,iBitmap *Bmp,UDWORD Modulus,int x,int y,BOOL bBilinear)
{
	PIEIMAGE pieImage;
	PIERECT dest;

	pie_SetBilinear(bBilinear);	//changed by alex 19 oct 98
	pie_SetRendMode(REND_GOURAUD_TEX);
	pie_SetColour(COLOURINTENSITY);
	pie_SetColourKeyedBlack(TRUE);

	pieImage.texPage = Image->TPageID;
	pieImage.tu = Image->Tu;
	pieImage.tv = Image->Tv;
	pieImage.tw = Image->Width;
	pieImage.th = Image->Height;
	dest.x = x+Image->XOffset;
	dest.y = y+Image->YOffset;
	dest.w = Image->Width;
	dest.h = Image->Height;
	pie_DrawImage(&pieImage, &dest, &rendStyle);

	pie_SetBilinear(FALSE);	//changed by alex 19 oct 98
}

void pie_ImageDefTrans(IMAGEDEF *Image,iBitmap *Bmp,UDWORD Modulus,int x,int y,int TransRate)
{
	pie_ImageDef(Image,Bmp,Modulus,x,y,FALSE);
}

void pie_UploadDisplayBuffer(UBYTE *DisplayBuffer)
{
	pie_GlobalRenderEnd(FALSE);
	screen_Upload((UWORD*)DisplayBuffer);
	pie_GlobalRenderBegin();
}

void pie_DownloadDisplayBuffer(UBYTE *DisplayBuffer)
{
	/*
	switch (pie_GetRenderEngine())
	{
	case ENGINE_4101:
	case ENGINE_SR:
		DownloadDisplayBuffer(DisplayBuffer);
		break;
	case ENGINE_D3D:
		//screen_SetBackDropFullWidth();//set when background sets
	default:
		break;
	}
	*/
}

void pie_ScaleBitmapRGB(UBYTE *DisplayBuffer,int Width,int Height,int ScaleR,int ScaleG,int ScaleB)
{
	switch (pie_GetRenderEngine())
	{
	case ENGINE_4101:
	case ENGINE_SR:
		ScaleBitmapRGB(DisplayBuffer, Width, Height, ScaleR, ScaleG, ScaleB);
		break;
	case ENGINE_D3D:
	case ENGINE_OPENGL:
	default:
		break;
	}
}

UDWORD radarTexture;
unsigned char radarBitmap[128*128*4];

BOOL pie_InitRadar(void)
{
	glGenTextures(1, &radarTexture);
	return TRUE;
}

BOOL pie_ShutdownRadar(void)
{
	glDeleteTextures(1, &radarTexture);
	return TRUE;
}


void pie_DownLoadRadar(unsigned char *buffer, UDWORD texPageID)
{
	unsigned int i, j;
	iColour* psPalette = pie_GetGamePal();

	for (i = 0, j = 0; i < 128*128; ++i) {
		radarBitmap[j++] = psPalette[buffer[i]].r;
		radarBitmap[j++] = psPalette[buffer[i]].g;
		radarBitmap[j++] = psPalette[buffer[i]].b;
		if (buffer[i] == 0) {
			radarBitmap[j++] = 0;
		} else {
			radarBitmap[j++] = 255;
		}
	}
	pie_SetTexturePage(radarTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 128, 128, 0,
		     GL_RGBA, GL_UNSIGNED_BYTE, radarBitmap);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
}

void pie_RenderRadar(IMAGEDEF *Image,iBitmap *Bmp,UDWORD Modulus,int x,int y)
{
	PIEIMAGE pieImage;
	PIERECT dest;

	pie_SetBilinear(TRUE);
	pie_SetRendMode(REND_GOURAUD_TEX);
	pie_SetColour(COLOURINTENSITY);
	pie_SetColourKeyedBlack(TRUE);
	//special case function because texture is held outside of texture list
	pieImage.texPage = radarTexture;
	pieImage.tu = 0;
	pieImage.tv = 0;
	pieImage.tw = 256;
	pieImage.th = 256;
	dest.x = x;
	dest.y = y;
	dest.w = 128;
	dest.h = 128;
	pie_DrawImage(&pieImage, &dest, &rendStyle);
}


void pie_RenderRadarRotated(IMAGEDEF *Image,iBitmap *Bmp,UDWORD Modulus,int x,int y,int angle)
{
	PIEIMAGE pieImage;
	PIERECT dest;

	pie_SetBilinear(TRUE);
	pie_SetRendMode(REND_GOURAUD_TEX);
	pie_SetColour(COLOURINTENSITY);
	pie_SetColourKeyedBlack(TRUE);
	//special case function because texture is held outside of texture list
	pieImage.texPage = radarTexture;
	pieImage.tu = 0;
	pieImage.tv = 0;
	pieImage.tw = 256;
	pieImage.th = 256;
	dest.x = x;
	dest.y = y;
	dest.w = 128;
	dest.h = 128;
	pie_DrawImage(&pieImage, &dest, &rendStyle);
}


/*	Converts an 8 bit raw (palettised) source image to
	a 16 bit argb destination image 
*/
void	bufferTo16Bit(UBYTE *origBuffer,UWORD *newBuffer, BOOL b3DFX)
{
UBYTE	paletteIndex;
UWORD	newColour;
UWORD	gun;
UDWORD	i;
ULONG			mask, amask, rmask, gmask, bmask;
BYTE			ap = 0,	ac = 0, rp = 0,	rc = 0, gp = 0,	gc = 0, bp = 0, bc = 0;
iColour*		psPalette;
UDWORD			size;

	if (b3DFX)
	{
		// 565 RGB
		ap = 16;
		ac = 0;
		rp = 11;
		rc = 5;
		gp = 5;
		gc = 6;
		bp = 0;
		bc = 5;
	}
	else
	{
		/*
		// Cannot playback if not 16bit mode 
		*/
		if( screenGetBackBufferBitDepth() == 16 )
		{
			screenGetBackBufferPixelFormatMasks(&amask, &rmask, &gmask, &bmask);
			/*
			// Find out the RGB type of the surface and tell the codec...
			*/
			mask = amask;
			if(mask!=0)
			{
				while(!(mask & 1))
				{
					mask>>=1;
					ap++;
				}
			}
			while((mask & 1))
			{
				mask>>=1;
				ac++;
			}

			mask = rmask;
			if(mask!=0)
			{
				while(!(mask & 1))
				{
					mask>>=1;
					rp++;
				}
			}
			while((mask & 1))
			{
				mask>>=1;
				rc++;
			}

			mask = gmask;
			if(mask!=0)
			{
				while(!(mask & 1))
				{
					mask>>=1;
					gp++;
				}
			}
			while((mask & 1))
			{
				mask>>=1;
				gc++;
			}

			mask = bmask;
			if(mask!=0)
			{
				while(!(mask & 1))
				{
					mask>>=1;
					bp++;
				}
			}
			while((mask & 1))
			{
				mask>>=1;
				bc++;
			}
		}
	}

	/*
		640*480, 8 bit colour source image 
		640*480, 16 bit colour destination image
	*/
	size = BACKDROP_WIDTH*BACKDROP_HEIGHT;//pie_GetVideoBufferWidth()*pie_GetVideoBufferHeight();
	for(i=0; i<size; i++)
	{
		psPalette = pie_GetGamePal();
		/* Get the next colour */
		paletteIndex = *origBuffer++;
		/* Flush out destination word (and alpha bits) */
		newColour = 0;
		/* Get red bits - 5 */
		gun = (UWORD)(psPalette[paletteIndex].r>>(8-rc));
		gun = gun << rp;
		newColour += gun;
		/* Get green bits - 6 */
		gun = (UWORD)(psPalette[paletteIndex].g>>(8-gc));
		gun = gun << gp;
		newColour += gun;
		/* Get blue bits - 5 */
		gun = (UWORD)(psPalette[paletteIndex].b>>(8-bc));
		gun = gun << bp;
		newColour += gun;
		/* Copy over */
		*newBuffer++ = newColour;
	}
}


void pie_ResetBackDrop(void)
{
	//screen_SetBackDrop(backDropBmp, BACKDROP_WIDTH, BACKDROP_HEIGHT);
	return;
}
	
	
void pie_LoadBackDrop(SCREENTYPE screenType, BOOL b3DFX) {
	UDWORD	chooser0,chooser1;
	CHAR	backd[128];

	//randomly load in a backdrop piccy.
	srand((unsigned)time( NULL ) );

	chooser0 = 0;
	chooser1 = rand()%7;

	switch (screenType)
	{
	case SCREEN_RANDOMBDROP:
		sprintf(backd,"texpages/bdrops/%d%d-bdrop.jpg", chooser0, chooser1);
		break;
	case SCREEN_COVERMOUNT:
		sprintf(backd,"texpages/bdrops/demo-bdrop.jpg");
		break;
	case SCREEN_MISSIONEND:
		sprintf(backd,"texpages/bdrops/missionend.jpg");
		break;
	case SCREEN_SLIDE1:
		sprintf(backd,"texpages/slides/slide1.jpg");
		break;
	case SCREEN_SLIDE2:
		sprintf(backd,"texpages/slides/slide2.jpg");
		break;
	case SCREEN_SLIDE3:
		sprintf(backd,"texpages/slides/slide3.jpg");
		break;
	case SCREEN_SLIDE4:
		sprintf(backd,"texpages/slides/slide4.jpg");
		break;
	case SCREEN_SLIDE5:
		sprintf(backd,"texpages/slides/slide5.jpg");
		break;

	default:
		sprintf(backd,"texpages/bdrops/credits.jpg");
		break;
	}

	screen_SetBackDropFromFile(backd);
}

void pie_D3DSetupRenderForFlip(SDWORD surfaceOffsetX, SDWORD surfaceOffsetY, UWORD* pSrcData, SDWORD srcWidth, SDWORD srcHeight, SDWORD srcStride)
{

	gSurfaceOffsetX = surfaceOffsetX;
	gSurfaceOffsetY = surfaceOffsetY;
	pgSrcData		=	pSrcData;
	gSrcWidth		=	srcWidth;
	gSrcHeight		= srcHeight;
	gSrcStride		= srcStride;
	return;
}

void pie_D3DRenderForFlip(void)
{
	if (pgSrcData != NULL)
	{
		pie_RenderImageToSurface(screenGetSurface(), gSurfaceOffsetX, gSurfaceOffsetY, pgSrcData, gSrcWidth, gSrcHeight, gSrcStride);
		pgSrcData = NULL;
	}
}

