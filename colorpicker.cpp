/* Raw Graphics Demonstrator Main Program
 * Computer Graphics Group "Chobits"
 *
 * NOTES:
 * http://www.ummon.eu/Linux/API/Devices/framebuffer.html
 *
 * TODOS:
 * - make dedicated canvas frame handler (currently the canvas frame is actually screen-sized)
 *
 */

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <vector>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <pthread.h>
#include <termios.h>
 using namespace std;

/* SETTINGS ------------------------------------------------------------------------------------------------------------------------------------------------------------ */
#define screenXstart 0
#define screenX 1366
#define screenY 700
#define mouseSensitivity 1

/* TYPEDEFS ------------------------------------------------------------------------------------------------------------------------------------------------------------ */

//RGB color
typedef struct s_rgb {
	unsigned char r;
	unsigned char g;
	unsigned char b;
	unsigned char a;
} RGB;

//Frame of RGBs
typedef struct s_frame {
	RGB **px;
} Frame;

//Coordinate System
typedef struct s_coord {
	int x;
	int y;
} Coord;

//The integrated frame buffer plus info struct.
typedef struct s_frameBuffer {
	char* ptr;
	int smemLen;
	int lineLen;
	int bpp;
} FrameBuffer;

void createPixelsArray(Frame *frm){
	RGB **px;
	px = (RGB **) malloc(sizeof(RGB *) * screenX);

	int i;
	for(i = 0; i < 1366; i++){
		px[i] = (RGB *) malloc(sizeof(RGB) * screenY);
	}

	frm->px = px;
}


/* MATH STUFF ---------------------------------------------------------------------------------------------------------------------------------------------------------- */

// construct coord
Coord coord(int x, int y) {
	Coord retval;
	retval.x = x;
	retval.y = y;
	return retval;
}

unsigned char isInBound(Coord position, Coord corner1, Coord corner2) {
	unsigned char xInBound = 0;
	unsigned char yInBound = 0;
	if (corner1.x < corner2.x) {
		xInBound = (position.x>corner1.x) && (position.x<corner2.x);
	} else if (corner1.x > corner2.x) {
		xInBound = (position.x>corner2.x) && (position.x<corner1.x);
	} else {
		return 0;
	}
	if (corner1.y < corner2.y) {
		yInBound = (position.y>corner1.y) && (position.y<corner2.y);
	} else if (corner1.y > corner2.y) {
		yInBound = (position.y>corner2.y) && (position.y<corner1.y);
	} else {
		return 0;
	}
	return xInBound&&yInBound;
}

/* MOUSE OPERATIONS ---------------------------------------------------------------------------------------------------------------------------------------------------- */

// get mouse coord, with integrated screen-space bounding
Coord getCursorCoord(Coord* mc) {
	Coord xy;
	if (mc->x < 0) {
		mc->x = 0;
		xy.x = 0;
	} else if (mc->x >= screenX*mouseSensitivity) {
		mc->x = screenX*mouseSensitivity-1;
		xy.x = screenX-1;
	} else {
		xy.x = (int) mc->x / mouseSensitivity;
	}
	if (mc->y < 0) {
		mc->y = 0;
		xy.y = 0;
	} else if (mc->y >= screenY*mouseSensitivity) {
		mc->y = screenY*mouseSensitivity-1;
		xy.y = screenY-1;
	} else {
		xy.y = (int) mc->y / mouseSensitivity;
	}
	return xy;
}

/* VIDEO OPERATIONS ---------------------------------------------------------------------------------------------------------------------------------------------------- */

// construct RGB
RGB rgb(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
	RGB retval;
	retval.r = r;
	retval.g = g;
	retval.b = b;
	retval.a = a;
	return retval;
}

// insert pixel to composition frame, with bounds filter
void insertPixel(Frame* frm, Coord loc, RGB col) {
	// do bounding check:
	if (!(loc.x >= screenX || loc.x < 0 || loc.y >= screenY || loc.y < 0)) {
		frm->px[loc.x][loc.y].r = col.r;
		frm->px[loc.x][loc.y].g = col.g;
		frm->px[loc.x][loc.y].b = col.b;
		frm->px[loc.x][loc.y].a = col.a;
	}
}

// Sprite Controller
void insertSprite(Frame* frm, Coord loc, unsigned short type) {
	switch (type) {
		case 1 : { // the mouse sprite
			insertPixel(frm, loc, rgb(255,255,255,255));
			int i;
			for (i=5; i<10; i++) {
				insertPixel(frm, coord(loc.x-i, loc.y), rgb(0,0,0,255));
				insertPixel(frm, coord(loc.x+i, loc.y), rgb(0,0,0,255));
				insertPixel(frm, coord(loc.x, loc.y-i), rgb(0,0,0,255));
				insertPixel(frm, coord(loc.x, loc.y+i), rgb(0,0,0,255));

				insertPixel(frm, coord(loc.x-i, loc.y+1), rgb(255,255,255,255));
				insertPixel(frm, coord(loc.x-i, loc.y-1), rgb(255,255,255,255));
				insertPixel(frm, coord(loc.x+i, loc.y+1), rgb(255,255,255,255));
				insertPixel(frm, coord(loc.x+i, loc.y-1), rgb(255,255,255,255));
				insertPixel(frm, coord(loc.x+1, loc.y-i), rgb(255,255,255,255));
				insertPixel(frm, coord(loc.x-1, loc.y-i), rgb(255,255,255,255));
				insertPixel(frm, coord(loc.x+1, loc.y+i), rgb(255,255,255,255));
				insertPixel(frm, coord(loc.x-1, loc.y+i), rgb(255,255,255,255));
			}
		} break;
		case 2 : { // something?

		} break;
	}
}

//get RGB from HSL
RGB getColorValue(unsigned short hue, unsigned char saturation, unsigned char luminosity){
	RGB curH;
	float s,l;

	//get RGB from selected Hue
	if (hue < 128) {
		curH = rgb(255,2*hue,0,255);
	} else if (hue < 256) {
		curH = rgb(255-(2*hue),255,0,255);
	} else if (hue < 384) {
		curH = rgb(0,255,2*hue,255);
	} else if (hue < 512) {
		curH = rgb(0,255-(2*hue),255,255);
	} else if (hue < 640) {
		curH = rgb(2*hue,0,255,255);
	} else if (hue < 768) {
		curH = rgb(255,0,255-(2*hue),255);
	}

	//show RGB from SL selector
	s = (float)saturation/255;
	RGB curHS = rgb(curH.r+(255-curH.r)*s,
					curH.g+(255-curH.g)*s,
					curH.b+(255-curH.b)*s,255
					);
	l = (float)luminosity/255;
	RGB curHSL = rgb(curHS.r-(curHS.r)*l,
					 curHS.g-(curHS.g)*l,
					 curHS.b-(curHS.b)*l,255
					 );

	return curHSL;
}

//show canvas
void showCanvas(Frame* frm, Frame* cnvs, int width, int height, Coord loc) {
	int x, y;

	for (y=0; y < height;y++) {
		for (x=0; x < width; x++) {
			if(cnvs->px[x][y].a != 0){
				insertPixel(frm, coord(loc.x+x, loc.y+y), cnvs->px[x][y]);
			}
		}
	}

	//show border
	for (y=0; y < height; y++) {
		if(cnvs->px[x][y].a != 0){
			insertPixel(frm, coord(loc.x-1, loc.y+y), rgb(255,255,255,255));
			insertPixel(frm, coord(loc.x+width, loc.y+y), rgb(255,255,255,255));
		}
	}
	for (x=0; x < width; x++) {
		if(cnvs->px[x][y].a != 0){
			insertPixel(frm, coord(loc.x+x, loc.y-1), rgb(255,255,255,255));
			insertPixel(frm, coord(loc.x+x, loc.y+height), rgb(255,255,255,255));
		}
	}
}

// delete contents of composition frame
void flushFrame (Frame* frm, RGB color) {
	int x;
	int y;
	for (y=0; y<screenY; y++) {
		for (x=0; x<screenX; x++) {
			frm->px[x][y] = color;
		}
	}
}

// copy composition Frame to FrameBuffer
void showFrame (Frame* frm, FrameBuffer* fb) {
	int x;
	int y;
	for (y=0; y<screenY; y++) {
		for (x=screenXstart; x<screenX; x++) {
			int location = x * (fb->bpp/8) + y * fb->lineLen;
			*(fb->ptr + location    ) = frm->px[x][y].b; // blue
			*(fb->ptr + location + 1) = frm->px[x][y].g; // green
			*(fb->ptr + location + 2) = frm->px[x][y].r; // red
			*(fb->ptr + location + 3) = frm->px[x][y].a; // transparency
		}
	}
}

/* Basic Drawing Functions --------------------------------------------------------------------------------------------------------------------------------------------------------- */

/* Fungsi membuat garis */
void plotLine(Frame* frm, int x0, int y0, int x1, int y1, RGB lineColor)
{
	int dx =  abs(x1-x0), sx = x0<x1 ? 1 : -1;
	int dy = -abs(y1-y0), sy = y0<y1 ? 1 : -1;
	int err = dx+dy, e2; /* error value e_xy */
	int loop = 1;
	while(loop){  /* loop */
		insertPixel(frm, coord(x0, y0), rgb(lineColor.r, lineColor.g, lineColor.b,255));
		if (x0==x1 && y0==y1) loop = 0;
		e2 = 2*err;
		if (e2 >= dy) { err += dy; x0 += sx; } /* e_xy+e_x > 0 */
		if (e2 <= dx) { err += dx; y0 += sy; } /* e_xy+e_y < 0 */
	}
}

int isColorEqual(RGB color1, RGB color2){
	if (color1.r == color2.r && color1.g == color2.g && color1.b == color2.b){
		return 1;
	}else{
		return 0;
	}
}

//colorFlood
void colorFlood(Frame* frm,int x, int y,RGB color){
	if (isColorEqual(frm->px[x][y],color) == 1){
		//do nothing
	}else{
		insertPixel(frm,coord(x,y),color);
		colorFlood(frm,x+1,y,color);
		colorFlood(frm,x,y+1,color);
		colorFlood(frm,x-1,y,color);
		colorFlood(frm,x,y-1,color);
	}
}

// addBlob
void addBlob(Frame* cnvs, Coord loc, RGB color) {
	int x,y;
	for (y=-2; y<3;y++) {
		for (x=-2; x<3; x++) {
			if (!(abs(x)==2 && abs(y)==2)) {
				insertPixel(cnvs, coord(loc.x+x, loc.y+y), color);
			}
		}
	}
}

// Plot Circle
void plotCircle(Frame* frm,int xm, int ym, int r,RGB col)
{
   int x = -r, y = 0, err = 2-2*r; /* II. Quadrant */
   do {
	  insertPixel(frm,coord(xm-x, ym+y),col); /*   I. Quadrant */
	  insertPixel(frm,coord(xm-y, ym-x),col); /*  II. Quadrant */
	  insertPixel(frm,coord(xm+x, ym-y),col); /* III. Quadrant */
	  insertPixel(frm,coord(xm+y, ym+x),col); /*  IV. Quadrant */
	  r = err;
	  if (r <= y) err += ++y*2+1;           /* e_xy+e_y < 0 */
	  if (r > x || err > y) err += ++x*2+1; /* e_xy+e_x > 0 or no 2nd y-step */
   } while (x < 0);
}


/* Menu Displayer --------------------------------------------------------------------------------------------------------------------------------------------------------- */

// Hue Selector
void showHueSelector(Frame* frm, Coord loc, unsigned short hueLoc) {
	int x,y;
	for ( y = 5; y < 45; y++ ) {
		for ( x = 0; x < 128; x++ ) {
			insertPixel(frm, coord(loc.x+x, loc.y+y), rgb(255,2*x,0,255));
		}
		for ( x = 128; x < 256; x++ ) {
			insertPixel(frm, coord(loc.x+x, loc.y+y), rgb(255-(2*x),255,0,255));
		}
		for ( x = 256; x < 384; x++ ) {
			insertPixel(frm, coord(loc.x+x, loc.y+y), rgb(0,255,2*x,255));
		}
		for ( x = 384; x < 512; x++ ) {
			insertPixel(frm, coord(loc.x+x, loc.y+y), rgb(0,255-(2*x),255,255));
		}
		for ( x = 512; x < 640; x++ ) {
			insertPixel(frm, coord(loc.x+x, loc.y+y), rgb(2*x,0,255,255));
		}
		for ( x = 640; x < 768; x++ ) {
			insertPixel(frm, coord(loc.x+x, loc.y+y), rgb(255,0,255-(2*x),255));
		}
	}

	//show border
	for (y=5; y<45; y++) {
		insertPixel(frm, coord(loc.x-1, loc.y+y), rgb(255,255,255,255));
		insertPixel(frm, coord(loc.x+768, loc.y+y), rgb(255,255,255,255));
	}
	for (x=0; x<768; x++) {
		insertPixel(frm, coord(loc.x+x, loc.y+4), rgb(255,255,255,255));
		insertPixel(frm, coord(loc.x+x, loc.y+45), rgb(255,255,255,255));
	}

	//show selected hue
	for (y=0; y<5;y++) {
		insertPixel(frm, coord(loc.x+hueLoc, loc.y+y), rgb(255,255,255,255));
		for (x=0; x<5-y; x++) {
			insertPixel(frm, coord(loc.x+hueLoc-x, loc.y+y), rgb(255,255,255,255));
			insertPixel(frm, coord(loc.x+hueLoc+x, loc.y+y), rgb(255,255,255,255));
		}
	}
	for (y=5; y<15;y++) {
		insertPixel(frm, coord(loc.x+hueLoc, loc.y+y), rgb(0,0,0,255));
	}
	for (y=45; y<50;y++) {
		insertPixel(frm, coord(loc.x+hueLoc, loc.y+y), rgb(255,255,255,255));
		for (x=0; x<y-44; x++) {
			insertPixel(frm, coord(loc.x+hueLoc-x, loc.y+y), rgb(255,255,255,255));
			insertPixel(frm, coord(loc.x+hueLoc+x, loc.y+y), rgb(255,255,255,255));
		}
	}
	for (y=35; y<45;y++) {
		insertPixel(frm, coord(loc.x+hueLoc, loc.y+y), rgb(0,0,0,255));
	}
}

// Saturation and Lightness Selector
void showSlSelector(Frame* frm, Coord loc, unsigned short hue, unsigned char sat, unsigned char lum) {
	RGB curH;
	int x, y;
	float s,l;

	//get RGB from selected Hue
	if (hue < 128) {
		curH = rgb(255,2*hue,0,255);
	} else if (hue < 256) {
		curH = rgb(255-(2*hue),255,0,255);
	} else if (hue < 384) {
		curH = rgb(0,255,2*hue,255);
	} else if (hue < 512) {
		curH = rgb(0,255-(2*hue),255,255);
	} else if (hue < 640) {
		curH = rgb(2*hue,0,255,255);
	} else if (hue < 768) {
		curH = rgb(255,0,255-(2*hue),255);
	}

	//show SL selector
	for ( y = 0; y < 256; y++ ) {
		s = (float)y/255;
		RGB curHS = rgb(curH.r+(255-curH.r)*s,
						curH.g+(255-curH.g)*s,
						curH.b+(255-curH.b)*s,255
						);
		for ( x = 0; x < 256; x++ ) {
			l = (float)x/255;
			RGB curHSL = rgb(curHS.r-(curHS.r)*l,
							 curHS.g-(curHS.g)*l,
							 curHS.b-(curHS.b)*l,255
							 );
			insertPixel(frm, coord(loc.x+x, loc.y+y), curHSL);
		}
	}

	//show border
	for (y=0; y<256; y++) {
		insertPixel(frm, coord(loc.x-1, loc.y+y), rgb(255,255,255,255));
		insertPixel(frm, coord(loc.x+256, loc.y+y), rgb(255,255,255,255));
	}
	for (x=0; x<256; x++) {
		insertPixel(frm, coord(loc.x+x, loc.y-1), rgb(255,255,255,255));
		insertPixel(frm, coord(loc.x+x, loc.y+256), rgb(255,255,255,255));
	}

	//show selected SL
	for (y=0; y<10;y++) {
		for (x=0; x<10; x++) {
			float euclidDistance = sqrt(pow(x,2)+pow(y,2));
			if((int)euclidDistance == 9){
				insertPixel(frm, coord(loc.x+lum-x, loc.y+sat+y), rgb(255,255,255,255));
				insertPixel(frm, coord(loc.x+lum+x, loc.y+sat+y), rgb(255,255,255,255));
			}
			if((int)euclidDistance == 8){
				insertPixel(frm, coord(loc.x+lum-x, loc.y+sat+y), rgb(0,0,0,255));
				insertPixel(frm, coord(loc.x+lum+x, loc.y+sat+y), rgb(0,0,0,255));
			}
		}
	}
	for (y=-10; y<0;y++) {
		for (x=0; x<10; x++) {
			float euclidDistance = sqrt(pow(x,2)+pow(y,2));
			insertPixel(frm, coord(loc.x+lum-2, loc.y+sat), rgb(0,0,0,255));
			insertPixel(frm, coord(loc.x+lum, loc.y+sat-2), rgb(0,0,0,255));
			insertPixel(frm, coord(loc.x+lum, loc.y+sat+2), rgb(0,0,0,255));
			insertPixel(frm, coord(loc.x+lum+2, loc.y+sat), rgb(0,0,0,255));
			insertPixel(frm, coord(loc.x+lum-3, loc.y+sat), rgb(255,255,255,255));
			insertPixel(frm, coord(loc.x+lum, loc.y+sat-3), rgb(255,255,255,255));
			insertPixel(frm, coord(loc.x+lum, loc.y+sat+3), rgb(255,255,255,255));
			insertPixel(frm, coord(loc.x+lum+3, loc.y+sat), rgb(255,255,255,255));
			if((int)euclidDistance == 9){
				insertPixel(frm, coord(loc.x+lum-x, loc.y+sat+y), rgb(255,255,255,255));
				insertPixel(frm, coord(loc.x+lum+x, loc.y+sat+y), rgb(255,255,255,255));
			}
			if((int)euclidDistance == 8){
				insertPixel(frm, coord(loc.x+lum-x, loc.y+sat+y), rgb(0,0,0,255));
				insertPixel(frm, coord(loc.x+lum+x, loc.y+sat+y), rgb(0,0,0,255));
			}
		}
	}
}

//show selected color
void showSelectedColor(Frame* frm, Coord loc, RGB color) {
	int x, y;
	for (y=0; y<50;y++) {
		for (x=0; x<256; x++) {
			insertPixel(frm, coord(loc.x+x, loc.y+y), color);
		}
	}
	//show border
	for (y=0; y<50; y++) {
		insertPixel(frm, coord(loc.x-1, loc.y+y), rgb(255,255,255,255));
		insertPixel(frm, coord(loc.x+256, loc.y+y), rgb(255,255,255,255));
	}
	for (x=0; x<256; x++) {
		insertPixel(frm, coord(loc.x+x, loc.y-1), rgb(255,255,255,255));
		insertPixel(frm, coord(loc.x+x, loc.y+50), rgb(255,255,255,255));
	}
}

//Draw Button
void drawButton(Frame* canvas, int originX, int originY, int code, RGB color)
{
	int left = originX;
	int right = originX + 30;
	int top = originY;
	int bottom = originY + 30;
	int centerX = originX + 15;
	int centerY = originY + 15;

	plotLine(canvas, left, top, right, top, color);
	plotLine(canvas, right, top, right, bottom, color);
	plotLine(canvas, right, bottom, left, bottom, color);
	plotLine(canvas, left, bottom, left, top, color);
	colorFlood(canvas, originX + 1, originY + 1, color);

	if(code == 1)
	{
		plotLine(canvas, left + 5, top + 5, right - 5, bottom - 5, rgb(0,0,0,0));
	}

	else if(code == 2)
	{
		plotLine(canvas, left + 5, top + 5, right - 5, top + 5, rgb(0,0,0,0));
		plotLine(canvas, right - 5, top + 5, right - 5, bottom - 5, rgb(0,0,0,0));
		plotLine(canvas, right - 5, bottom - 5, left + 5, bottom - 5, rgb(0,0,0,0));
		plotLine(canvas, left + 5, bottom - 5, left + 5, top + 5, rgb(0,0,0,0));
	}

	else if(code == 4)
	{
		plotLine(canvas, left + 10, top + 5, left + 10, bottom - 5, rgb(0,0,0,0));
		plotLine(canvas, left + 10, top + 5, right - 10, top + 5, rgb(0,0,0,0));
		plotLine(canvas, left + 10, centerY - 3, right - 10, centerY -3, rgb(0,0,0,0));
	}

	else if(code == 3)
	{
		plotLine(canvas, left + 5, top + 10, centerX, top + 5, rgb(0,0,0,0));
		plotLine(canvas, centerX, top + 5, right - 5, top + 10, rgb(0,0,0,0));
		plotLine(canvas, right - 5, top + 10, right - 10, bottom - 5, rgb(0,0,0,0));
		plotLine(canvas, right - 10, bottom - 5, left +10 , bottom - 5, rgb(0,0,0,0));
		plotLine(canvas, left +10 , bottom - 5, left + 5, top + 10, rgb(0,0,0,0));
	}

	else if(code == 5)
	{
		plotCircle(canvas, centerX, centerY, 10, rgb(0,0,0,0));
	}
}

/* FUNCTIONS FOR SCANLINE ALGORITHM ---------------------------------------------------------------------------------------------------------------------------------------------------- */

bool isSlopeEqualsZero(int y0, int y1){
	if(y0 == y1){
		return true;
	}else{
		return false;
	}
}

bool isInBetween(int y0, int y1, int yTest){
	if((yTest >= y0 && yTest <= y1 || yTest >= y1 && yTest <= y0) && !isSlopeEqualsZero(y0, y1)){
		return true;
	}else{
		return false;
	}
}

/* Function to calculate intersection between line (a,b) and line with slope 0 */
Coord intersection(Coord a, Coord b, int y){
	int x;
	double slope;

	if(b.x == a.x){
		x = a.x;
	}else{
		slope = (double)(b.y - a.y) / (double)(b.x - a.x);
		x = round(((double)(y - a.y) / slope) + (double)a.x);
	}

	return coord(x, y);
}

bool compareByAxis(const s_coord &a, const s_coord &b){
	return a.x <= b.x;
}

bool compareSameAxis(const s_coord &a, const s_coord &b){
	return a.x == b.x;
}

bool operator==(const Coord& lhs, const Coord& rhs) {
	if(lhs.x==rhs.x && lhs.y==rhs.y)
		return true;
	return false;
}

bool isLocalMaxima(const Coord& a, const Coord& b, const Coord& titikPotong) {
	return ((titikPotong.y<a.y && titikPotong.y<b.y) || (titikPotong.y>a.y && titikPotong.y>b.y));
}

vector<Coord> intersectionGenerator(int y, vector<Coord> polygon){
	vector<Coord> intersectionPoint;
	Coord prevTipot = coord(-9999,-9999);
	for(int i = 0; i < polygon.size(); i++){
		if(i == polygon.size() - 1){
			if(isInBetween(polygon.at(i).y, polygon.at(0).y, y)){
				Coord a = coord(polygon.at(i).x, polygon.at(i).y);
				Coord b = coord(polygon.at(0).x, polygon.at(0).y);

				Coord titikPotong = intersection(a, b, y);

				if(titikPotong==b){
					if(isLocalMaxima(polygon.at(i), polygon.at(1), titikPotong))
						intersectionPoint.push_back(titikPotong);
				}
				else {
					if(prevTipot==titikPotong){
						if(isLocalMaxima(polygon.at(i-1), polygon.at(0), titikPotong))
							intersectionPoint.push_back(titikPotong);
					}
					else
						intersectionPoint.push_back(titikPotong);
				}
			}
		}else{
			if(isInBetween(polygon.at(i).y, polygon.at(i + 1).y, y)){
				Coord a = coord(polygon.at(i).x, polygon.at(i).y);
				Coord b = coord(polygon.at(i + 1).x, polygon.at(i + 1).y);

				Coord titikPotong = intersection(a, b, y);

				// Jika sama dgn tipot sebelumnya, cek apakah local minima/maxima
				if(titikPotong==prevTipot) {
					Coord z = coord(polygon.at(i-1).x, polygon.at(i-1).y);
					if(isLocalMaxima(z, b, titikPotong)) {
						intersectionPoint.push_back(titikPotong);
					}
				}
				else {
					intersectionPoint.push_back(titikPotong);
				}
				prevTipot = intersectionPoint.back();
			}
		}
	}

	sort(intersectionPoint.begin(), intersectionPoint.end(), compareByAxis);

	return intersectionPoint;
}
vector<Coord> combineIntersection(vector<Coord> a, vector<Coord> b){
	for(int i = 0; i < b.size(); i++){
		a.push_back(b.at(i));
	}

	sort(a.begin(), a.end(), compareByAxis);

	return a;
}

void scanlineFill(Frame *canvas, vector<Coord> vertex, RGB color) {
	for(int i = 1; i <= screenY; i++){
		vector<Coord> shapeIntersectionPoint = intersectionGenerator(i, vertex);

		// if(shapeIntersectionPoint.size() % 2 != 0){
		// 	unique(shapeIntersectionPoint.begin(), shapeIntersectionPoint.end(), compareSameAxis);
		// 	shapeIntersectionPoint.erase(shapeIntersectionPoint.end() - 1);
		// }

		int shapeSize = shapeIntersectionPoint.size()-1;

		for(int j = 0; j<shapeSize; j++){
			if(j % 2 == 0){
				int x0 = shapeIntersectionPoint.at(j).x-580;
				int y0 = shapeIntersectionPoint.at(j).y-120;
				int x1 = shapeIntersectionPoint.at(j + 1).x-580;
				int y1 = shapeIntersectionPoint.at(j + 1).y-120;

				plotLine(canvas, x0, y0, x1, y1, color);
			}
		}
	}
}

/* Paint-App Functions ---------------------------------------------------------------------------------------------------------------------------------------- */

//Draw Polygon
void drawPolygon(Frame* canvas, vector<Coord> vertex, int originX, int originY, RGB color) {
	for(int i=0;i<vertex.size();++i) {
		if(i==(vertex.size()-1))
			plotLine(canvas, vertex.at(i).x-originX, vertex.at(i).y-originY, vertex.at(0).x-originX, vertex.at(0).y-originY, color);
		else
			plotLine(canvas, vertex.at(i).x-originX, vertex.at(i).y-originY, vertex.at(i+1).x-originX, vertex.at(i+1).y-originY, color);
	}

	scanlineFill(canvas, vertex, color);
}

//Draw Square
void drawSquare(Frame* canvas, Coord mousePosition, int mouseState, int originX, int originY, RGB color, RGB canvasColor){
	static Coord initialPosition;
	static int isReleased = 1;
	Coord currentPosition;
	int x, y;

	if(mouseState && isReleased){
		initialPosition = coord(mousePosition.x - originX, mousePosition.y - originY);
		isReleased = 0;
	}

	if(mouseState && !isReleased){
		currentPosition = coord(mousePosition.x - originX, mousePosition.y - originY);

		// draw square lines
		plotLine(canvas, initialPosition.x, initialPosition.y, initialPosition.x, currentPosition.y, color);
		plotLine(canvas, initialPosition.x, initialPosition.y, currentPosition.x, initialPosition.y, color);
		plotLine(canvas, currentPosition.x, initialPosition.y, currentPosition.x, currentPosition.y, color);
		plotLine(canvas, initialPosition.x, currentPosition.y, currentPosition.x, currentPosition.y, color);
	}

	if(!mouseState && !isReleased){
		isReleased = 1;
		currentPosition = coord(mousePosition.x - originX, mousePosition.y - originY);

		// draw square lines
		plotLine(canvas, initialPosition.x, initialPosition.y, initialPosition.x, currentPosition.y, color);
		plotLine(canvas, initialPosition.x, initialPosition.y, currentPosition.x, initialPosition.y, color);
		plotLine(canvas, currentPosition.x, initialPosition.y, currentPosition.x, currentPosition.y, color);
		plotLine(canvas, initialPosition.x, currentPosition.y, currentPosition.x, currentPosition.y, color);

		colorFlood(canvas, (currentPosition.x + initialPosition.x) / 2, (currentPosition.y + initialPosition.y) / 2, color);
	}
}

//Draw Circle
void drawCircle(Frame* canvas, Coord mousePosition, int mouseState, int originX, int originY, RGB color, RGB canvasColor){
	static Coord initialPosition;
	static int isReleased = 1;
	Coord currentPosition;
	int x, y;
	int len;

	if(mouseState && isReleased){
		initialPosition = coord(mousePosition.x - originX, mousePosition.y - originY);
		isReleased = 0;
	}

	if(mouseState && !isReleased){
		currentPosition = coord(mousePosition.x - originX, mousePosition.y - originY);
		len = sqrt(pow(abs(initialPosition.x - currentPosition.x),2)+pow(abs(initialPosition.y - currentPosition.y),2));
		//len = coord(abs(initialPosition.x - currentPosition.x), abs(initialPosition.y - currentPosition.y));

		plotCircle(canvas, initialPosition.x, initialPosition.y, len, color);
	}

	if(!mouseState && !isReleased){
		isReleased = 1;
		currentPosition = coord(mousePosition.x - originX, mousePosition.y - originY);
		len = sqrt(pow(abs(initialPosition.x - currentPosition.x),2)+pow(abs(initialPosition.y - currentPosition.y),2));
		//len = coord(abs(initialPosition.x - currentPosition.x), abs(initialPosition.y - currentPosition.y));

		plotCircle(canvas, initialPosition.x, initialPosition.y, len, color);

		colorFlood(canvas, initialPosition.x, initialPosition.y, color);
	}
}

//draw Line
void drawLine(Frame* canvas, Coord mousePosition, int originX, int originY, RGB color, RGB canvasColor){
	static Coord initialPosition;
	static int isInitiated = 0;
	static int secondClick = 1;
	Coord currentPosition;
	int x, y;

	if(isInitiated && !secondClick){
		currentPosition = coord(mousePosition.x - originX, mousePosition.y - originY);
		isInitiated = 0;
		secondClick = 1;

		// draw line
		plotLine(canvas, initialPosition.x, initialPosition.y, currentPosition.x, currentPosition.y, color);
	}else{
		if(!isInitiated && secondClick){
			initialPosition = coord(mousePosition.x - originX, mousePosition.y - originY);
			isInitiated = 1;
			secondClick = 0;
		}
	}
}

/* GLOBALVAR DECLARATIONS ----------------------------------------------- */
static struct termios old, new1;
// set termios to not display pressed KBD
void initTermios(int echo) {
	tcgetattr(0, &old); /* grab old terminal i/o settings */
	new1 = old; /* make new settings same as old settings */
	new1.c_lflag &= ~ICANON; /* disable buffered i/o */
	new1.c_lflag &= echo ? ECHO : ~ECHO; /* set echo mode */
	tcsetattr(0, TCSANOW, &new1); /* use these new terminal i/o settings now */
}

/* Restore old terminal i/o settings */
void resetTermios(void) {
	tcsetattr(0, TCSANOW, &old);
}

// KBD Controller
unsigned char mloop = 1; // frame loop controller
int tool = 4; // selected tool
void *threadFuncKeyboard(void *arg)
{
	char c;
	initTermios(0);

	while(mloop){
		read(0, &c, 1);
		if(c == 49){
			tool = 1;
		}

		if(c == 50){
			tool = 2;
		}

		if(c == 51){
			tool = 3;
		}
		if(c == 52){
			tool = 4;
		}
		if(c == 53){
			tool = 5;
		}
		if(c == 27){
			mloop = 0;
		}
	}

	resetTermios();

	return NULL;
}

/* MAIN FUNCTION ------------------------------------------------------------------------------------------------------------------------------------------------------- */
int main() {
	/* Preparations ---------------------------------------------------- */
	
	for (int i = 0; i<500; i++) {
		cout << "\n";
	}
	cout << "Init... PaintApp v1.00 - (c) 2015 Chobits Group\n";

	// get fb and screenInfos
	struct fb_var_screeninfo vInfo; // variable screen info
	struct fb_fix_screeninfo sInfo; // static screen info
	int fbFile;	 // frame buffer file descriptor
	fbFile = open("/dev/fb0",O_RDWR);
	if (!fbFile) {
		printf("Error: cannot open framebuffer device.\n");
		exit(1);
	}
	if (ioctl (fbFile, FBIOGET_FSCREENINFO, &sInfo)) {
		printf("Error reading fixed information.\n");
		exit(2);
	}
	if (ioctl (fbFile, FBIOGET_VSCREENINFO, &vInfo)) {
		printf("Error reading variable information.\n");
		exit(3);
	}

	//create the FrameBuffer struct with its important infos.
	FrameBuffer fb;
	fb.smemLen = sInfo.smem_len;
	fb.lineLen = sInfo.line_length;
	fb.bpp = vInfo.bits_per_pixel;

	//and map the framebuffer to the FB struct.
	fb.ptr = (char*)mmap(0, sInfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fbFile, 0);
	if ((long int)fb.ptr == -1) {
		printf ("Error: failed to map framebuffer device to memory.\n");
		exit(4);
	}

	//prepare mouse controller
	FILE *fmouse;
	char mouseRaw[3];
	fmouse = fopen("/dev/input/mice","r");
	Coord mouse; // mouse internal counter
	mouse.x = 0;
	mouse.y = 0;

	//prepare environment controller
	Frame cFrame; // composition frame (Video RAM)
	createPixelsArray(&cFrame);

	Frame canvas; // persistence canvas frame
	createPixelsArray(&canvas);
	flushFrame(&canvas, rgb(255,255,255,255)); // prepare canvas

	Frame drawingCanvas;
	createPixelsArray(&drawingCanvas);
	flushFrame(&drawingCanvas, rgb(255,255,255,0));

	unsigned short hue = 0; //the hue location, 0..768
	unsigned char sat = 0; //saturation, 0..255
	unsigned char lum = 0; //luminosity, 0..255
	RGB colorValue;
	float trigonoLen;
	int i; //for drawing.

	pthread_t pth_keyboard;
	pthread_create(&pth_keyboard,NULL,threadFuncKeyboard,NULL);

	vector<Coord> polygonVertex;

	/* Main Loop ------------------------------------------------------------------------------------------------------------------------------------------------------- */
	while (mloop) {
		//calc color value
		colorValue = getColorValue(hue, sat, lum);

		//clean
		flushFrame(&cFrame, rgb(0,0,0,255));

		//hue selector
		showHueSelector(&cFrame, coord(299,50), hue);

		//saturation and lightness selector
		showSlSelector(&cFrame, coord(299,120), hue, sat, lum);

		//show selected
		showSelectedColor(&cFrame, coord(299,400), colorValue);

		//show canvas
		showCanvas(&cFrame, &canvas, 487, 500, coord(580,120));

		//show buttons, koordinat = titik kiri atas, sisi = 30
		int buttonLeft = 310;
		int buttonRight = 340;
		int buttonUp = 470;
		int buttonBottom = 500;
		drawButton(&cFrame, buttonLeft, 		buttonUp, 1, rgb(255,255,255,255));
		drawButton(&cFrame, buttonLeft + 50, 	buttonUp, 2, rgb(255,255,255,255));
		drawButton(&cFrame, buttonLeft + 100, 	buttonUp, 3, rgb(255,255,255,255));
		drawButton(&cFrame, buttonLeft + 150, 	buttonUp, 4, rgb(255,255,255,255));
		drawButton(&cFrame, buttonLeft + 200, 	buttonUp, 5, rgb(255,255,255,255));

		//Draw Tool
		switch(tool){
			case 1: //LINE
				drawButton(&cFrame, buttonLeft,		buttonUp, 1, rgb(200,200,200,255));
				if((mouseRaw[0]&1)){
					if (isInBound(getCursorCoord(&mouse),coord(580,120), coord(1067,620))) {
						drawLine(&canvas, coord(getCursorCoord(&mouse).x, getCursorCoord(&mouse).y), 580, 120, colorValue, rgb(255,255,255,255));
						showCanvas(&cFrame, &canvas, 487, 500, coord(580,120));
					}
				}
				break;

			case 2: //square
				drawButton(&cFrame, buttonLeft + 50, 	buttonUp, 2, rgb(200,200,200,255));
				if((mouseRaw[0]&1)){
					if (isInBound(getCursorCoord(&mouse),coord(580,120), coord(1067,620))) {
						flushFrame(&drawingCanvas, rgb(255,255,255,0));
						drawSquare(&drawingCanvas, coord(getCursorCoord(&mouse).x, getCursorCoord(&mouse).y), mouseRaw[0]&1, 580, 120, colorValue, rgb(255,255,255,255));
						showCanvas(&cFrame, &drawingCanvas, 487, 500, coord(580,120));
					}
				}else{
					if (isInBound(getCursorCoord(&mouse),coord(580,120), coord(1067,620))) {
						drawSquare(&canvas, coord(getCursorCoord(&mouse).x, getCursorCoord(&mouse).y), mouseRaw[0]&1, 580, 120, colorValue, rgb(255,255,255,255));
						showCanvas(&cFrame, &canvas, 487, 500, coord(580,120));
					}
				}
				break;
			case 3: //poly
				drawButton(&cFrame, buttonLeft + 100, 	buttonUp, 3, rgb(200,200,200,255));
				if(mouseRaw[0]&1) {
					if (isInBound(getCursorCoord(&mouse),coord(580,120), coord(1067,620))) {
						polygonVertex.push_back(getCursorCoord(&mouse));
					}
				}
				if(polygonVertex.size()>2) {
					flushFrame(&drawingCanvas, rgb(255,255,255,0));
					drawPolygon(&drawingCanvas, polygonVertex, 580, 120, colorValue);
					showCanvas(&cFrame, &drawingCanvas, 487, 500, coord(580,120));
				}
				
				if(mouseRaw[0]&2) {
					polygonVertex.push_back(getCursorCoord(&mouse));
					drawPolygon(&canvas, polygonVertex, 580, 120, colorValue);
					showCanvas(&cFrame, &canvas, 487, 500, coord(580,120));
					polygonVertex.clear();
				}
				break;
			case 4: //freehand
				drawButton(&cFrame, buttonLeft + 150, 	buttonUp, 4, rgb(200,200,200,255));
				if((mouseRaw[0]&1)){
					if (isInBound(getCursorCoord(&mouse),coord(580,120), coord(1067,620))) {
						trigonoLen = sqrt((float)pow(mouseRaw[1],2)+(float)pow(mouseRaw[2],2));
						for (i=0; i<=trigonoLen; i++) {
							addBlob(&canvas, coord(getCursorCoord(&mouse).x-580-(mouseRaw[1]*i/trigonoLen), getCursorCoord(&mouse).y-120+(mouseRaw[2]*i/trigonoLen)), colorValue);
						}
					}
				}
				break;
			case 5: //circle
				drawButton(&cFrame, buttonLeft + 200, 	buttonUp, 5, rgb(200,200,200,255));
				if((mouseRaw[0]&1)){
					if (isInBound(getCursorCoord(&mouse),coord(580,120), coord(1067,620))) {
						flushFrame(&drawingCanvas, rgb(255,255,255,0));
						drawCircle(&drawingCanvas, coord(getCursorCoord(&mouse).x, getCursorCoord(&mouse).y), mouseRaw[0]&1, 580, 120, colorValue, rgb(255,255,255,255));
						showCanvas(&cFrame, &drawingCanvas, 487, 500, coord(580,120));
					}
				}else{
					if (isInBound(getCursorCoord(&mouse),coord(580,120), coord(1067,620))) {
						drawCircle(&canvas, coord(getCursorCoord(&mouse).x, getCursorCoord(&mouse).y), mouseRaw[0]&1, 580, 120, colorValue, rgb(255,255,255,255));
						showCanvas(&cFrame, &canvas, 487, 500, coord(580,120));
					}
				}
				break;
		}

		//fill mouse LAST
		insertSprite(&cFrame, getCursorCoord(&mouse), 1);

		//show frame
		showFrame(&cFrame,&fb);

		//read next mouse
		fread(mouseRaw,sizeof(char),3,fmouse);
		mouse.x += mouseRaw[1];
		mouse.y -= mouseRaw[2];

		//menu press controller
		if ((mouseRaw[0]&1)>0) {

			//in hue selector
			if (isInBound(getCursorCoord(&mouse),coord(299,50), coord(1066,100))) {
				hue = getCursorCoord(&mouse).x-299;
			} else

			//in sl selector
			if (isInBound(getCursorCoord(&mouse),coord(299,120), coord(555,376))) {
				sat = getCursorCoord(&mouse).y-120;
				lum = getCursorCoord(&mouse).x-299;
			} else

			// in tools
			if (isInBound(getCursorCoord(&mouse),coord(buttonLeft,buttonUp), coord(buttonLeft + 30,buttonBottom))) {
				tool = 1;
			} else if (isInBound(getCursorCoord(&mouse),coord(buttonLeft + 50,buttonUp), coord(buttonLeft + 80,buttonBottom))) {
				tool = 2;
			} else if (isInBound(getCursorCoord(&mouse),coord(buttonLeft + 100,buttonUp), coord(buttonLeft + 130,buttonBottom))) {
				tool = 3;
			} else if (isInBound(getCursorCoord(&mouse),coord(buttonLeft + 150,buttonUp), coord(buttonLeft + 180,buttonBottom))) {
				tool = 4;
			} else if (isInBound(getCursorCoord(&mouse),coord(buttonLeft + 200,buttonUp), coord(buttonLeft + 230,buttonBottom))) {
				tool = 5;
			}
		}
	}

	/* Cleanup --------------------------------------------------------------------------------------------------------------------------------------------------------- */
	pthread_join(pth_keyboard,NULL);
	munmap(fb.ptr, sInfo.smem_len);
	close(fbFile);
	fclose(fmouse);
	
	cout << "Graceful Exit successful.\n";

	return 0;
}

