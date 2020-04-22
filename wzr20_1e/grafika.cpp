/************************************************************
					 Grafika OpenGL
					 *************************************************************/
#include <windows.h>
#include <time.h>
#include <gl\gl.h>
#include <gl\glu.h>
#include <iterator> 
#include <map>
using namespace std;

#include "grafika.h"
//#include "wektor.h"
//#include "kwaternion.h"
#include "obiekty.h"

ParametryWidoku parawid;

extern long czas_start;                // czas od poczatku dzialania aplikacji  
extern long dlugosc_dnia ;         // czas trwania dnia w [s]

extern FILE *f;


extern ObiektRuchomy *moj_pojazd;               // obiekt przypisany do tej aplikacji
extern map<int, ObiektRuchomy*> obiekty_ruchome;
extern Teren teren;
extern CRITICAL_SECTION m_cs;              // do synchronizacji w¹tków

int g_GLPixelIndex = 0;
HGLRC g_hGLContext = NULL;
unsigned int font_base;


extern void TworzListyWyswietlania();		// definiujemy listy tworz¹ce labirynt
extern void RysujGlobalnyUkladWsp();


int InicjujGrafike(HDC g_context)
{

	if (SetWindowPixelFormat(g_context) == FALSE)
		return FALSE;

	if (CreateViewGLContext(g_context) == FALSE)
		return 0;
	BuildFont(g_context);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	// Parametry widoku:
	parawid.kierunek_kamery_1 = Wektor3(11, -3, -14);   // kierunek patrzenia
	parawid.pol_kamery_1 = Wektor3(-34, 6, 10);         // po³o¿enie kamery
	parawid.pion_kamery_1 = Wektor3(0, 1, 0);           // kierunek pionu kamery        
	parawid.kierunek_kamery_2 = Wektor3(0, -1, 0.02);   // to samo dla widoku z góry
	parawid.pol_kamery_2 = Wektor3(0, 90, 0);
	parawid.pion_kamery_2 = Wektor3(0, 0, -1);
	parawid.kierunek_kamery = parawid.kierunek_kamery_1;
	parawid.pol_kamery = parawid.pol_kamery_1;
	parawid.pion_kamery = parawid.pion_kamery_1;
	parawid.sledzenie = 1;                             // tryb œledzenia obiektu przez kamerê
	parawid.widok_z_gory = 0;                          // tryb widoku z góry
	parawid.oddalenie = 21.0;                          // oddalenie widoku z kamery
	parawid.kat_kam_z = 0;                            // obrót kamery góra-dó³
	parawid.oddalenie_1 = parawid.oddalenie;
	parawid.kat_kam_z_1 = parawid.kat_kam_z;
	parawid.oddalenie_2 = parawid.oddalenie; 
	parawid.kat_kam_z_2 = parawid.kat_kam_z;
	parawid.oddalenie_3 = parawid.oddalenie;
	parawid.kat_kam_z_3 = parawid.kat_kam_z;


	TworzListyWyswietlania();		// definiujemy listy tworz¹ce ró¿ne elementy sceny
	teren.PoczatekGrafiki();
}


void RysujScene()
{
	GLfloat BlueSurface[] = { 0.3f, 0.0f, 0.8f, 0.5f };
	GLfloat DGreenSurface[] = { 0.1f, 0.8f, 0.05f, 0.5f };
	GLfloat RedSurface[] = { 0.8f, 0.2f, 0.1f, 0.5f };
	GLfloat GreenSurface[] = { 0.05f, 0.60f, 0.25f, 1.0f };
	GLfloat YellowSurface[] = { 0.75f, 0.75f, 0.0f, 1.0f };

	GLfloat LightAmbient[] = { 0.1f, 0.1f, 0.1f, 0.1f };
	GLfloat LightDiffuse[] = { 0.7f, 0.7f, 0.7f, 0.7f };
	GLfloat LightPosition[] = { 5.0f, 5.0f, 5.0f, 0.0f };

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glLightfv(GL_LIGHT0, GL_AMBIENT, LightAmbient);		//1 sk³adowa: œwiat³o otaczaj¹ce (bezkierunkowe)
	glLightfv(GL_LIGHT0, GL_DIFFUSE, LightDiffuse);		//2 sk³adowa: œwiat³o rozproszone (kierunkowe)
	glLightfv(GL_LIGHT0, GL_POSITION, LightPosition);
	glEnable(GL_LIGHT0);

	glPushMatrix();
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, BlueSurface);

	glClearColor(0.10, 0.08, 0.5, 0.7);

	Wektor3 kierunek_k, pion_k, pol_k;
	if (parawid.sledzenie)
	{
		kierunek_k = moj_pojazd->stan.qOrient.obroc_wektor(Wektor3(1, 0, 0));
		pion_k = moj_pojazd->stan.qOrient.obroc_wektor(Wektor3(0, 1, 0));
		Wektor3 prawo_kamery = moj_pojazd->stan.qOrient.obroc_wektor(Wektor3(0, 0, 1));
		if (parawid.widok_z_gory)
		{
			kierunek_k = (moj_pojazd->stan.wPol - teren.srodek).znorm()*(-1);
			pol_k = moj_pojazd->stan.wPol - parawid.kierunek_kamery*moj_pojazd->dlugosc * 10;
			pion_k = moj_pojazd->stan.qOrient.obroc_wektor(Wektor3(1, 0, 0));
		}
		else
		{
			pion_k = pion_k.obrot(parawid.kat_kam_z, prawo_kamery.x, prawo_kamery.y, prawo_kamery.z);
			kierunek_k = kierunek_k.obrot(parawid.kat_kam_z, prawo_kamery.x, prawo_kamery.y, prawo_kamery.z);
			pol_k = moj_pojazd->stan.wPol - kierunek_k*moj_pojazd->dlugosc * 0 +
				pion_k.znorm()*moj_pojazd->wysokosc * 5;
		}
	}
	else
	{
		pion_k = parawid.pion_kamery;
		kierunek_k = parawid.kierunek_kamery;
		pol_k = parawid.pol_kamery;
		Wektor3 prawo_kamery = (kierunek_k*pion_k).znorm();
		pion_k = pion_k.obrot(parawid.kat_kam_z / 20, prawo_kamery.x, prawo_kamery.y, prawo_kamery.z);
		kierunek_k = kierunek_k.obrot(parawid.kat_kam_z / 20, prawo_kamery.x, prawo_kamery.y, prawo_kamery.z);
	}

	// Ustawianie widoku sceny    
	gluLookAt(pol_k.x - parawid.oddalenie*kierunek_k.x,
		pol_k.y - parawid.oddalenie*kierunek_k.y, pol_k.z - parawid.oddalenie*kierunek_k.z,
		pol_k.x + kierunek_k.x, pol_k.y + kierunek_k.y, pol_k.z + kierunek_k.z,
		pion_k.x, pion_k.y, pion_k.z);

	//glRasterPos2f(0.30,-0.27);
	//glPrint("MojObiekt->iID = %d",moj_pojazd->iID ); 

	RysujGlobalnyUkladWsp();

	// s³oñce + t³o:
	int R = 50000;                // promieñ obiegu
	long x = (clock() - czas_start) % (dlugosc_dnia*CLOCKS_PER_SEC);
	float angle = (float)x / (dlugosc_dnia*CLOCKS_PER_SEC) * 2 * 3.1416;
	//char lan[128];
	//sprintf(lan,"angle = %f\n", angle);
	//SetWindowText(main_window, lan);


	GLfloat SunColor[] = { 8.0, 8.0, 8.0, 1.0f };
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, SunColor);
	glPushMatrix();
	glTranslatef(R*cos(angle), R*(cos(angle)*0.5 - 0.2), R*sin(angle));   // ustawienie s³oñca
	//glTranslatef(R*cos(angle), 5000, R*sin(angle));
	GLUquadricObj *Qsph = gluNewQuadric();
	gluSphere(Qsph, 700.0, 25, 25);
	gluDeleteQuadric(Qsph);
	glPopMatrix();

	//glPushMatrix();

	glPushMatrix();


	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, BlueSurface);
	glEnable(GL_BLEND);

	// Lock the Critical section
	EnterCriticalSection(&m_cs);               // wejœcie na œcie¿kê krytyczn¹ - by inne w¹tki (np. g³ówny) nie wspó³dzieli³ 

	moj_pojazd->Rysuj();

	for (map<int, ObiektRuchomy*>::iterator it = obiekty_ruchome.begin(); it != obiekty_ruchome.end(); it++)
	{
		int liczba_ob = obiekty_ruchome.size();
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, DGreenSurface);
		ObiektRuchomy *ob = it->second;
		if (ob != NULL)
			ob->Rysuj();
	}
	//Release the Critical section
	LeaveCriticalSection(&m_cs);               // wyjœcie ze œcie¿ki krytycznej

	glDisable(GL_BLEND);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, GreenSurface);

	teren.Rysuj();
	glPopMatrix();

	glPopMatrix();

	glFlush();

}

void ZmianaRozmiaruOkna(int cx, int cy)
{
	GLsizei width, height;
	GLdouble aspect;
	width = cx;
	height = cy;

	if (cy == 0)
		aspect = (GLdouble)width;
	else
		aspect = (GLdouble)width / (GLdouble)height;

	glViewport(0, 0, width, height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(35, aspect, 1, 1000000.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glDrawBuffer(GL_BACK);

	glEnable(GL_LIGHTING);

	glEnable(GL_DEPTH_TEST);

}


Wektor3 WspolrzedneKursora3D(int x, int y)
{
	//  pobranie macierz modelowania
	GLdouble model[16];
	glGetDoublev(GL_MODELVIEW_MATRIX, model);

	// pobranie macierzy rzutowania
	GLdouble proj[16];
	glGetDoublev(GL_PROJECTION_MATRIX, proj);

	// pobranie obszaru renderingu
	GLint view[4];
	glGetIntegerv(GL_VIEWPORT, view);

	// tablice ze odczytanymi wspó³rzêdnymi w przestrzeni widoku
	GLdouble wsp[3];

	//RECT rc;
	//GetClientRect (okno, &rc);

	GLdouble liczba;

	int wynik = gluProject(0, 0, 0, model, proj, view, wsp + 0, wsp + 1, &liczba);

	gluUnProject(x, y, liczba, model, proj, view, wsp + 0, wsp + 1, wsp + 2);
	//gluUnProject (x,rc.bottom - y,liczba ,model,proj,view,wsp+0,wsp+1,wsp+2);
	return Wektor3(wsp[0], wsp[1], wsp[2]);
}

void WspolrzedneEkranu(float *xx, float *yy, float *zz, Wektor3 Punkt3D)
{
	//  pobranie macierz modelowania
	GLdouble model[16];
	glGetDoublev(GL_MODELVIEW_MATRIX, model);
	// pobranie macierzy rzutowania
	GLdouble proj[16];
	glGetDoublev(GL_PROJECTION_MATRIX, proj);
	// pobranie obszaru renderingu
	GLint view[4];
	glGetIntegerv(GL_VIEWPORT, view);
	// tablice ze odczytanymi wspó³rzêdnymi w przestrzeni widoku
	GLdouble wsp[3], wsp_okn[3];
	GLdouble liczba;
	int wynik = gluProject(Punkt3D.x, Punkt3D.y, Punkt3D.z, model, proj, view, wsp_okn + 0, wsp_okn + 1, wsp_okn + 2);
	gluUnProject(wsp_okn[0], wsp_okn[1], wsp_okn[2], model, proj, view, wsp + 0, wsp + 1, wsp + 2);
	fprintf(f, "   Wsp. punktu 3D = (%f, %f, %f), wspolrzedne w oknie = (%f, %f, %f), wsp. punktu = (%f, %f, %f)\n",
		Punkt3D.x, Punkt3D.y, Punkt3D.z, wsp_okn[0], wsp_okn[1], wsp_okn[2], wsp[0], wsp[1], wsp[2]);

	(*xx) = wsp_okn[0]; (*yy) = wsp_okn[1]; (*zz) = wsp_okn[2];
}

void ZakonczenieGrafiki()
{
	if (wglGetCurrentContext() != NULL)
	{
		// dezaktualizacja kontekstu renderuj¹cego
		wglMakeCurrent(NULL, NULL);
	}
	if (g_hGLContext != NULL)
	{
		wglDeleteContext(g_hGLContext);
		g_hGLContext = NULL;
	}
	glDeleteLists(font_base, 96);
}

BOOL SetWindowPixelFormat(HDC hDC)
{
	PIXELFORMATDESCRIPTOR pixelDesc;

	pixelDesc.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pixelDesc.nVersion = 1;
	pixelDesc.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_STEREO_DONTCARE;
	pixelDesc.iPixelType = PFD_TYPE_RGBA;
	pixelDesc.cColorBits = 32;
	pixelDesc.cRedBits = 8;
	pixelDesc.cRedShift = 16;
	pixelDesc.cGreenBits = 8;
	pixelDesc.cGreenShift = 8;
	pixelDesc.cBlueBits = 8;
	pixelDesc.cBlueShift = 0;
	pixelDesc.cAlphaBits = 0;
	pixelDesc.cAlphaShift = 0;
	pixelDesc.cAccumBits = 64;
	pixelDesc.cAccumRedBits = 16;
	pixelDesc.cAccumGreenBits = 16;
	pixelDesc.cAccumBlueBits = 16;
	pixelDesc.cAccumAlphaBits = 0;
	pixelDesc.cDepthBits = 32;
	pixelDesc.cStencilBits = 8;
	pixelDesc.cAuxBuffers = 0;
	pixelDesc.iLayerType = PFD_MAIN_PLANE;
	pixelDesc.bReserved = 0;
	pixelDesc.dwLayerMask = 0;
	pixelDesc.dwVisibleMask = 0;
	pixelDesc.dwDamageMask = 0;
	g_GLPixelIndex = ChoosePixelFormat(hDC, &pixelDesc);

	if (g_GLPixelIndex == 0)
	{
		g_GLPixelIndex = 1;

		if (DescribePixelFormat(hDC, g_GLPixelIndex, sizeof(PIXELFORMATDESCRIPTOR), &pixelDesc) == 0)
		{
			return FALSE;
		}
	}

	if (SetPixelFormat(hDC, g_GLPixelIndex, &pixelDesc) == FALSE)
	{
		return FALSE;
	}

	return TRUE;
}
BOOL CreateViewGLContext(HDC hDC)
{
	g_hGLContext = wglCreateContext(hDC);

	if (g_hGLContext == NULL)
	{
		return FALSE;
	}

	if (wglMakeCurrent(hDC, g_hGLContext) == FALSE)
	{
		return FALSE;
	}

	return TRUE;
}

GLvoid BuildFont(HDC hDC)								// Build Our Bitmap Font
{
	HFONT	font;										// Windows Font ID
	HFONT	oldfont;									// Used For Good House Keeping

	font_base = glGenLists(96);								// Storage For 96 Characters

	font = CreateFont(-14,							// Height Of Font
		0,								// Width Of Font
		0,								// Angle Of Escapement
		0,								// Orientation Angle
		FW_NORMAL,						// Font Weight
		FALSE,							// Italic
		FALSE,							// Underline
		FALSE,							// Strikeout
		ANSI_CHARSET,					// Character Set Identifier
		OUT_TT_PRECIS,					// Output Precision
		CLIP_DEFAULT_PRECIS,			// Clipping Precision
		ANTIALIASED_QUALITY,			// Output Quality
		FF_DONTCARE | DEFAULT_PITCH,		// Family And Pitch
		"Courier New");					// Font Name

	oldfont = (HFONT)SelectObject(hDC, font);           // Selects The Font We Want
	wglUseFontBitmaps(hDC, 32, 96, font_base);				// Builds 96 Characters Starting At Character 32
	SelectObject(hDC, oldfont);							// Selects The Font We Want
	DeleteObject(font);									// Delete The Font
}

// Napisy w OpenGL
GLvoid glPrint(const char *fmt, ...)	// Custom GL "Print" Routine
{
	char		text[256];	// Holds Our String
	va_list		ap;		// Pointer To List Of Arguments

	if (fmt == NULL)		// If There's No Text
		return;			// Do Nothing

	va_start(ap, fmt);		// Parses The String For Variables
	vsprintf(text, fmt, ap);	// And Converts Symbols To Actual Numbers
	va_end(ap);			// Results Are Stored In Text

	glPushAttrib(GL_LIST_BIT);	// Pushes The Display List Bits
	glListBase(font_base - 32);		// Sets The Base Character to 32
	glCallLists(strlen(text), GL_UNSIGNED_BYTE, text);	// Draws The Display List Text
	glPopAttrib();			// Pops The Display List Bits
}


void TworzListyWyswietlania()
{
	glNewList(Wall1, GL_COMPILE);	// GL_COMPILE - lista jest kompilowana, ale nie wykonywana

	glBegin(GL_QUADS);		// inne opcje: GL_POINTS, GL_LINES, GL_LINE_STRIP, GL_LINE_LOOP
	// GL_TRIANGLES, GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN, GL_QUAD_STRIP, GL_POLYGON
	glNormal3f(-1.0, 0.0, 0.0);
	glVertex3f(-1.0, -1.0, 1.0);
	glVertex3f(-1.0, 1.0, 1.0);
	glVertex3f(-1.0, 1.0, -1.0);
	glVertex3f(-1.0, -1.0, -1.0);
	glEnd();
	glEndList();

	glNewList(Wall2, GL_COMPILE);
	glBegin(GL_QUADS);
	glNormal3f(1.0, 0.0, 0.0);
	glVertex3f(1.0, -1.0, 1.0);
	glVertex3f(1.0, 1.0, 1.0);
	glVertex3f(1.0, 1.0, -1.0);
	glVertex3f(1.0, -1.0, -1.0);
	glEnd();
	glEndList();

	glNewList(Auto, GL_COMPILE);
	glBegin(GL_QUADS);
	// przod
	glNormal3f(0.0, 0.0, 1.0);

	glVertex3f(0, 0, 1);
	glVertex3f(0, 1, 1);
	glVertex3f(0.7, 1, 1);
	glVertex3f(0.7, 0, 1);

	glVertex3f(0.7, 0, 1);
	glVertex3f(0.7, 0.5, 1);
	glVertex3f(1.0, 0.5, 1);
	glVertex3f(1.0, 0, 1);
	// tyl
	glNormal3f(0.0, 0.0, -1.0);
	glVertex3f(0, 0, 0);
	glVertex3f(0.7, 0, 0);
	glVertex3f(0.7, 1, 0);
	glVertex3f(0, 1, 0);

	glVertex3f(0.7, 0, 0);
	glVertex3f(1.0, 0, 0);
	glVertex3f(1.0, 0.5, 0);
	glVertex3f(0.7, 0.5, 0);
	// gora
	glNormal3f(0.0, 1.0, 0.0);
	glVertex3f(0, 1, 0);
	glVertex3f(0, 1, 1);
	glVertex3f(0.7, 1, 1);
	glVertex3f(0.7, 1, 0);

	glVertex3f(0.7, 0.5, 0);
	glVertex3f(0.7, 0.5, 1);
	glVertex3f(1.0, 0.5, 1);
	glVertex3f(1.0, 0.5, 0);
	// dol
	glNormal3f(0.0, -1.0, 0.0);
	glVertex3f(0, 0, 0);
	glVertex3f(1, 0, 0);
	glVertex3f(1, 0, 1);
	glVertex3f(0, 0, 1);
	// prawo
	glNormal3f(1.0, 0.0, 0.0);
	glVertex3f(0.7, 0.5, 0);
	glVertex3f(0.7, 0.5, 1);
	glVertex3f(0.7, 1, 1);
	glVertex3f(0.7, 1, 0);

	glVertex3f(1.0, 0.0, 0);
	glVertex3f(1.0, 0.0, 1);
	glVertex3f(1.0, 0.5, 1);
	glVertex3f(1.0, 0.5, 0);
	// lewo
	glNormal3f(-1.0, 0.0, 0.0);
	glVertex3f(0, 0, 0);
	glVertex3f(0, 1, 0);
	glVertex3f(0, 1, 1);
	glVertex3f(0, 0, 1);

	glEnd();
	glEndList();

}


void RysujGlobalnyUkladWsp(void)
{

	glColor3f(1, 0, 0);
	glBegin(GL_LINES);
	glVertex3f(0, 0, 0);
	glVertex3f(2, 0, 0);
	glVertex3f(2, -0.25, 0.25);
	glVertex3f(2, 0.25, -0.25);
	glVertex3f(2, -0.25, -0.25);
	glVertex3f(2, 0.25, 0.25);

	glEnd();
	glColor3f(0, 1, 0);
	glBegin(GL_LINES);
	glVertex3f(0, 0, 0);
	glVertex3f(0, 2, 0);
	glVertex3f(0, 2, 0);
	glVertex3f(0.25, 2, 0);
	glVertex3f(0, 2, 0);
	glVertex3f(-0.25, 2, 0.25);
	glVertex3f(0, 2, 0);
	glVertex3f(-0.25, 2, -0.25);

	glEnd();
	glColor3f(0, 0, 1);
	glBegin(GL_LINES);
	glVertex3f(0, 0, 0);
	glVertex3f(0, 0, 2);
	glVertex3f(-0.25, -0.25, 2);
	glVertex3f(0.25, 0.25, 2);
	glVertex3f(-0.25, -0.25, 2);
	glVertex3f(0.25, -0.25, 2);
	glVertex3f(-0.25, 0.25, 2);
	glVertex3f(0.25, 0.25, 2);

	glEnd();

	glColor3f(1, 1, 1);
}

