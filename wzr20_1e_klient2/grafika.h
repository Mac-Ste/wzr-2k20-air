#include <gl\gl.h>
#include <gl\glu.h>
#ifndef _WEKTOR__H
  #include "wektor.h"
#endif

struct ParametryWidoku
{
	// Parametry widoku:
	Wektor3 kierunek_kamery_1,                  // kierunek patrzenia
		pol_kamery_1,                           // po³o¿enie kamery
		pion_kamery_1,                          // kierunek pionu kamery        
		kierunek_kamery_2,                      // to samo dla widoku z góry
		pol_kamery_2,
		pion_kamery_2,
		kierunek_kamery, pol_kamery, pion_kamery;
	bool sledzenie;                             // tryb œledzenia obiektu przez kamerê
	bool widok_z_gory;                          // tryb widoku z góry
	float oddalenie;                            // oddalenie widoku z kamery
	float kat_kam_z;                            // obrót kamery góra-dó³
	float oddalenie_1, kat_kam_z_1, oddalenie_2, kat_kam_z_2,
		oddalenie_3, kat_kam_z_3;
};

enum GLDisplayListNames
{
	Wall1 = 1,
	Wall2 = 2,
	Floor = 3,
	Cube = 4,
  Auto = 5,
	PowierzchniaTerenu = 10
};


int InicjujGrafike(HDC g_context);
void RysujScene();
void ZmianaRozmiaruOkna(int cx,int cy);
Wektor3 WspolrzedneKursora3D(int x, int y);
void WspolrzedneEkranu(float *xx, float *yy, float *zz, Wektor3 Punkt3D);
void ZakonczenieGrafiki();

BOOL SetWindowPixelFormat(HDC hDC);
BOOL CreateViewGLContext(HDC hDC);
GLvoid BuildFont(HDC hDC);
GLvoid glPrint(const char *fmt, ...);