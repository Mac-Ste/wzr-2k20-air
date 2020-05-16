/****************************************************
		Wirtualne zespoly robocze - przykladowy projekt w C++
		Do zadañ dotycz¹cych wspó³pracy, ekstrapolacji i
		autonomicznych obiektów
		****************************************************/

#include <windows.h>
#include <math.h>
#include <time.h>

#include <gl\gl.h>
#include <gl\glu.h>
#include <iterator> 
#include <map>

#include "obiekty.h"
#include "siec.h"
#include "grafika.h"
using namespace std;
FILE *f = fopen("wwc_log.txt", "w");    // plik do zapisu informacji testowych

#define SERV_IP ("192.168.1.16")
#define SERV_RECIV_PORT 10000
#define MY_RECIV_PORT 10003
#define SERV_SEND_PORT 10001

ObiektRuchomy *moj_pojazd;          // obiekt przypisany do tej aplikacji
Teren teren;

map<int, ObiektRuchomy*> obiekty_ruchome;


float fDt;                          // sredni czas pomiedzy dwoma kolejnymi cyklami symulacji i wyswietlania
long czas_cyklu_WS, licznik_sym;     // zmienne pomocnicze potrzebne do obliczania fDt
long czas_start = clock();          // czas uruchomienia aplikacji
long dlugosc_dnia = 600;         // czas trwania dnia w [s]

unicast_net *uni_reciv;         // wsk do obiektu zajmujacego sie odbiorem komunikatow
unicast_net *uni_send;          //   -||-  wysylaniem komunikatow

HANDLE threadReciv;                 // uchwyt w¹tku odbioru komunikatów
CRITICAL_SECTION m_cs;              // do synchronizacji w¹tków
extern HWND okno;
int SHIFTwcisniety = 0;
bool czy_rysowac_ID = 1;            // czy rysowac nr ID przy ka¿dym obiekcie
bool sterowanie_myszkowe = 0;       // sterowanie za pomoc¹ klawisza myszki
int kursor_x = 0, kursor_y = 0;     // po³o¿enie kursora myszy


extern ParametryWidoku parawid;      // ustawienia widoku zdefiniowane w grafice


enum typy_ramek{ STAN_OBIEKTU, NIEAKTYWNOSC };


struct Ramka                                    // g³ówna struktura s³u¿¹ca do przesy³ania informacji
{
	int typ;
	int iID;
	unsigned long listen_port;
	long moment_wyslania;
	StanObiektu stan;
};


//******************************************
// Funkcja obs³ugi w¹tku odbioru komunikatów 
DWORD WINAPI FunkcjaWatkuOdbioru(void *ptr)
{
	unicast_net *pmt_net = (unicast_net*)ptr;                // wskaŸnik do obiektu klasy multicast_net
	int rozmiar;                                               // liczba bajtów ramki otrzymanej z sieci
	Ramka ramka;
	unsigned long sender_ip;
	StanObiektu stan;

	while (1)
	{
		rozmiar = pmt_net->reciv((char*)&ramka, &sender_ip, sizeof(Ramka));   // oczekiwanie na nadejœcie ramki - funkcja samoblokuj¹ca siê 
		switch (ramka.typ)
		{
		case STAN_OBIEKTU:
		{
			stan = ramka.stan;

			if (ramka.iID != moj_pojazd->iID)                     // jeœli to nie mój obiekt
			{
				// Lock the Critical section
				EnterCriticalSection(&m_cs);               // wejœcie na œcie¿kê krytyczn¹ - by inne w¹tki (np. g³ówny) nie wspó³dzieli³ 
				if ((obiekty_ruchome.size() == 0)||(obiekty_ruchome[ramka.iID] == NULL))                     // nie ma jeszcze takiego obiektu w tablicy -> trzeba go stworzyæ
				{
					ObiektRuchomy *ob = new ObiektRuchomy(&teren);
					ob->iID = ramka.iID;
					obiekty_ruchome[ramka.iID] = ob;
				}
				obiekty_ruchome[ramka.iID]->ZmienStan(stan);   // zmiana stanu obiektu obcego 
				//Release the Critical section
				LeaveCriticalSection(&m_cs);               // wyjœcie ze œcie¿ki krytycznej
			}
			break;
		} // case STAN_OBIEKTU
		case NIEAKTYWNOSC: {
			if ((obiekty_ruchome.size() != 0) && (obiekty_ruchome[ramka.iID] != NULL))                     // nie ma jeszcze takiego obiektu w tablicy -> trzeba go stworzyæ
			{
				ObiektRuchomy *ob = obiekty_ruchome[ramka.iID];
				obiekty_ruchome.erase(ramka.iID);
				delete ob;
			}
			break;
		}
		} // switch
	}  // while(1)
	return 1;
}

// *****************************************************************
// ****    Wszystko co trzeba zrobiæ podczas uruchamiania aplikacji
// ****    poza grafik¹   
void PoczatekInterakcji()
{
	DWORD dwThreadId;

	moj_pojazd = new ObiektRuchomy(&teren);    // tworzenie wlasnego obiektu


	czas_cyklu_WS = clock();             // pomiar aktualnego czasu

	uni_reciv = new unicast_net(MY_RECIV_PORT);      // obiekt do odbioru ramek sieciowych
	uni_send = new unicast_net(SERV_RECIV_PORT);       // obiekt do wysy³ania ramek


	// uruchomienie watku obslugujacego odbior komunikatow
	threadReciv = CreateThread(
		NULL,                        // no security attributes
		0,                           // use default stack size
		FunkcjaWatkuOdbioru,                // thread function
		(void *)uni_reciv,               // argument to thread function
		0,                           // use default creation flags
		&dwThreadId);                // returns the thread identifier

}


// *****************************************************************
// ****    Wszystko co trzeba zrobiæ w ka¿dym cyklu dzia³ania 
// ****    aplikacji poza grafik¹ 
void Cykl_WS()
{
	licznik_sym++;

	// obliczenie czasu fDt pomiedzy dwoma kolejnymi cyklami
	if (licznik_sym % 50 == 0)          // jeœli licznik cykli przekroczy³ pewn¹ wartoœæ, to
	{                                   // nale¿y na nowo obliczyæ œredni czas cyklu fDt
		char text[200];
		long czas_pop = czas_cyklu_WS;
		czas_cyklu_WS = clock();
		float fFps = (50 * CLOCKS_PER_SEC) / (float)(czas_cyklu_WS - czas_pop);
		if (fFps != 0) fDt = 1.0 / fFps; else fDt = 1;
		sprintf(text, "WWC-lab 2019/20 temat 1, wersja e (%0.0f fps  %0.2fms)  ", fFps, 1000.0 / fFps);
		SetWindowText(okno, text); // wyœwietlenie aktualnej iloœci klatek/s w pasku okna			
	}


	moj_pojazd->Symulacja(fDt);                    // symulacja obiektu w³asnego 

	Ramka ramka;
	ramka.typ = STAN_OBIEKTU;
	ramka.stan = moj_pojazd->Stan();               // stan w³asnego obiektu 
	ramka.iID = moj_pojazd->iID;
	ramka.listen_port = MY_RECIV_PORT;


	// wys³anie komunikatu o stanie obiektu przypisanego do aplikacji (moj_pojazd):  
	uni_send->send((char*)&ramka, SERV_IP, sizeof(Ramka));
}

// *****************************************************************
// ****    Wszystko co trzeba zrobiæ podczas zamykania aplikacji
// ****    poza grafik¹ 
void ZakonczenieInterakcji()
{
	fprintf(f, "Interakcja zosta³a zakoñczona\n");
	fclose(f);
}


//deklaracja funkcji obslugi okna
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);


HWND okno;                   // uchwyt do okna aplikacji
HDC g_context = NULL;        // uchwyt kontekstu graficznego



//funkcja Main - dla Windows
int WINAPI WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR     lpCmdLine,
	int       nCmdShow)
{
	//Initilize the critical section
	InitializeCriticalSection(&m_cs);
	MSG meldunek;		  //innymi slowy "komunikat"
	WNDCLASS nasza_klasa; //klasa g³ównego okna aplikacji

	static char nazwa_klasy[] = "KlasaPodstawowa";

	//Definiujemy klase g³ównego okna aplikacji
	//Okreslamy tu wlasciwosci okna, szczegoly wygladu oraz
	//adres funkcji przetwarzajacej komunikaty
	nasza_klasa.style = CS_HREDRAW | CS_VREDRAW;
	nasza_klasa.lpfnWndProc = WndProc; //adres funkcji realizuj¹cej przetwarzanie meldunków 
	nasza_klasa.cbClsExtra = 0;
	nasza_klasa.cbWndExtra = 0;
	nasza_klasa.hInstance = hInstance; //identyfikator procesu przekazany przez MS Windows podczas uruchamiania programu
	nasza_klasa.hIcon = 0;
	nasza_klasa.hCursor = LoadCursor(0, IDC_ARROW);
	nasza_klasa.hbrBackground = (HBRUSH)GetStockObject(GRAY_BRUSH);
	nasza_klasa.lpszMenuName = "Menu";
	nasza_klasa.lpszClassName = nazwa_klasy;

	//teraz rejestrujemy klasê okna g³ównego
	RegisterClass(&nasza_klasa);

	/*tworzymy okno g³ówne
	okno bêdzie mia³o zmienne rozmiary, listwê z tytu³em, menu systemowym
	i przyciskami do zwijania do ikony i rozwijania na ca³y ekran, po utworzeniu
	bêdzie widoczne na ekranie */
	okno = CreateWindow(nazwa_klasy, "WWC-lab 2019/20 temat 1, wersja e", WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
		100, 50, 900, 750, NULL, NULL, hInstance, NULL);


	ShowWindow(okno, nCmdShow);

	//odswiezamy zawartosc okna
	UpdateWindow(okno);

	// G£ÓWNA PÊTLA PROGRAMU

	// pobranie komunikatu z kolejki jeœli funkcja PeekMessage zwraca wartoœæ inn¹ ni¿ FALSE,
	// w przeciwnym wypadku symulacja wirtualnego œwiata wraz z wizualizacj¹
	ZeroMemory(&meldunek, sizeof(meldunek));
	while (meldunek.message != WM_QUIT)
	{
		if (PeekMessage(&meldunek, NULL, 0U, 0U, PM_REMOVE))
		{
			TranslateMessage(&meldunek);
			DispatchMessage(&meldunek);
		}
		else
		{
			Cykl_WS();    // Cykl wirtualnego œwiata
			InvalidateRect(okno, NULL, FALSE);
		}
	}

	return (int)meldunek.wParam;
}

/********************************************************************
FUNKCJA OKNA realizujaca przetwarzanie meldunków kierowanych do okna aplikacji*/
LRESULT CALLBACK WndProc(HWND okno, UINT kod_meldunku, WPARAM wParam, LPARAM lParam)
{


	switch (kod_meldunku)
	{
	case WM_CREATE:  //meldunek wysy³any w momencie tworzenia okna
	{

		g_context = GetDC(okno);

		srand((unsigned)time(NULL));
		int wynik = InicjujGrafike(g_context);
		if (wynik == 0)
		{
			printf("nie udalo sie otworzyc okna graficznego\n");
			//exit(1);
		}

		PoczatekInterakcji();

		SetTimer(okno, 1, 10, NULL);

		return 0;
	}


	case WM_PAINT:
	{
		PAINTSTRUCT paint;
		HDC kontekst;
		kontekst = BeginPaint(okno, &paint);

		RysujScene();
		SwapBuffers(kontekst);

		EndPaint(okno, &paint);

		return 0;
	}

	case WM_TIMER:
		return 0;

	case WM_SIZE:
	{
		int cx = LOWORD(lParam);
		int cy = HIWORD(lParam);

		ZmianaRozmiaruOkna(cx, cy);

		return 0;
	}

	case WM_DESTROY: //obowi¹zkowa obs³uga meldunku o zamkniêciu okna

		ZakonczenieInterakcji();

		ZakonczenieGrafiki();


		ReleaseDC(okno, g_context);
		KillTimer(okno, 1);

		DWORD ExitCode;
		GetExitCodeThread(threadReciv, &ExitCode);
		TerminateThread(threadReciv, ExitCode);

		obiekty_ruchome.clear();

		PostQuitMessage(0);
		return 0;

	case WM_LBUTTONDOWN: //reakcja na lewy przycisk myszki
	{
		int x = LOWORD(lParam);
		int y = HIWORD(lParam);
		if (sterowanie_myszkowe)
			moj_pojazd->F = 40000.0;        // si³a pchaj¹ca do przodu [N]
		break;
	}
	case WM_RBUTTONDOWN: //reakcja na prawy przycisk myszki
	{
		int x = LOWORD(lParam);
		int y = HIWORD(lParam);
		if (sterowanie_myszkowe)
			moj_pojazd->F = -20000.0;        // si³a pchaj¹ca do tylu

		break;
	}
	case WM_MBUTTONDOWN: //reakcja na œrodkowy przycisk myszki : uaktywnienie/dezaktywacja sterwania myszkowego
	{
		sterowanie_myszkowe = 1 - sterowanie_myszkowe;
		kursor_x = LOWORD(lParam);
		kursor_y = HIWORD(lParam);
		break;
	}
	case WM_LBUTTONUP: //reakcja na puszczenie lewego przycisku myszki
	{
		if (sterowanie_myszkowe)
			moj_pojazd->F = 0.0;        // si³a pchaj¹ca do przodu
		break;
	}
	case WM_RBUTTONUP: //reakcja na puszczenie lewy przycisk myszki
	{
		if (sterowanie_myszkowe)
			moj_pojazd->F = 0.0;        // si³a pchaj¹ca do przodu
		break;
	}
	case WM_MOUSEMOVE:
	{
		int x = LOWORD(lParam);
		int y = HIWORD(lParam);
		if (sterowanie_myszkowe)
		{
			float kat_skretu = (float)(kursor_x - x) / 20;
			if (kat_skretu > 45) kat_skretu = 45;
			if (kat_skretu < -45) kat_skretu = -45;
			moj_pojazd->stan.kat_skretu_kol = PI*kat_skretu / 180;
		}
		break;
	}
	case WM_KEYDOWN:
	{

		switch (LOWORD(wParam))
		{
		case VK_SHIFT:
		{
			SHIFTwcisniety = 1;
			break;
		}
		case VK_SPACE:
		{
			moj_pojazd->ham = 10.0;       // stopieñ hamowania (reszta zale¿y od si³y docisku i wsp. tarcia)
			break;                       // 1.0 to maksymalny stopieñ (np. zablokowanie kó³)
		}
		case VK_UP:
		{

			moj_pojazd->F = 20000.0;        // si³a pchaj¹ca do przodu
			break;
		}
		case VK_DOWN:
		{
			moj_pojazd->F = -20000.0;
			break;
		}
		case VK_LEFT:
		{
			if (moj_pojazd->predkosc_krecenia_kierownica < 0){
				moj_pojazd->predkosc_krecenia_kierownica = 0;
				moj_pojazd->czy_kierownica_trzymana = true;
			}
			else{
				if (SHIFTwcisniety) moj_pojazd->predkosc_krecenia_kierownica = 0.5;
				else moj_pojazd->predkosc_krecenia_kierownica = 0.25 / 8;
			}

			break;
		}
		case VK_RIGHT:
		{
			if (moj_pojazd->predkosc_krecenia_kierownica > 0){
				moj_pojazd->predkosc_krecenia_kierownica = 0;
				moj_pojazd->czy_kierownica_trzymana = true;
			}
			else{
				if (SHIFTwcisniety) moj_pojazd->predkosc_krecenia_kierownica = -0.5;
				else moj_pojazd->predkosc_krecenia_kierownica = -0.25 / 8;
			}
			break;
		}
		case 'I':   // wypisywanie nr ID
		{
			czy_rysowac_ID = 1 - czy_rysowac_ID;
			break;
		}
		case 'W':   // oddalenie widoku
		{
			//pol_kamery = pol_kamery - kierunek_kamery*0.3;
			if (parawid.oddalenie > 0.5) parawid.oddalenie /= 1.2;
			else parawid.oddalenie = 0;
			break;
		}
		case 'S':   // przybli¿enie widoku
		{
			//pol_kamery = pol_kamery + kierunek_kamery*0.3; 
			if (parawid.oddalenie > 0) parawid.oddalenie *= 1.2;
			else parawid.oddalenie = 0.5;
			break;
		}
		case 'Q':   // widok z góry
		{
			//if (sledzenie) break;
			parawid.widok_z_gory = 1 - parawid.widok_z_gory;
			if (parawid.widok_z_gory)                // przechodzimy do widoku z góry
			{
				parawid.pol_kamery_1 = parawid.pol_kamery; parawid.kierunek_kamery_1 = parawid.kierunek_kamery; 
				parawid.pion_kamery_1 = parawid.pion_kamery;
				parawid.oddalenie_1 = parawid.oddalenie; parawid.kat_kam_z_1 = parawid.kat_kam_z;

				//pol_kamery = pol_kamery_2; kierunek_kamery = kierunek_kamery_2; pion_kamery = pion_kamery_2;
				parawid.oddalenie = parawid.oddalenie_2; parawid.kat_kam_z = parawid.kat_kam_z_2;
				// Po³o¿enie kamery, kierunek oraz pion ustawiamy tak, by obiekt widziany by³ z góry i jecha³
				// pocz¹tkowo w górê ekranu:
				parawid.kierunek_kamery = (moj_pojazd->stan.wPol - teren.srodek).znorm()*(-1);
				parawid.pol_kamery = moj_pojazd->stan.wPol - parawid.kierunek_kamery*moj_pojazd->dlugosc * 10;
				parawid.pion_kamery = moj_pojazd->stan.qOrient.obroc_wektor(Wektor3(1, 0, 0));
			}
			else
			{
				parawid.pol_kamery_2 = parawid.pol_kamery; parawid.kierunek_kamery_2 = parawid.kierunek_kamery; 
				parawid.pion_kamery_2 = parawid.pion_kamery;
				parawid.oddalenie_2 = parawid.oddalenie; parawid.kat_kam_z_2 = parawid.kat_kam_z;

				// Po³o¿enie kamery, kierunek oraz pion ustawiamy tak, by obiekt widziany by³ z prawego boku i jecha³
				// pocz¹tkowo ze strony lewej na praw¹:
				parawid.kierunek_kamery = moj_pojazd->stan.qOrient.obroc_wektor(Wektor3(0, 0, 1))*-1;
				parawid.pol_kamery = moj_pojazd->stan.wPol - parawid.kierunek_kamery*moj_pojazd->dlugosc * 10;
				parawid.pion_kamery = (moj_pojazd->stan.wPol - teren.srodek).znorm();

				//pol_kamery = pol_kamery_1; kierunek_kamery = kierunek_kamery_1; pion_kamery = pion_kamery_1;
				parawid.oddalenie = parawid.oddalenie_1; parawid.kat_kam_z = parawid.kat_kam_z_1;
			}
			break;
		}
		case 'E':   // obrót kamery ku górze (wzglêdem lokalnej osi z)
		{
			parawid.kat_kam_z += PI * 5 / 180;
			break;
		}
		case 'D':   // obrót kamery ku do³owi (wzglêdem lokalnej osi z)
		{
			parawid.kat_kam_z -= PI * 5 / 180;
			break;
		}
		case 'A':   // w³¹czanie, wy³¹czanie trybu œledzenia obiektu
		{
			parawid.sledzenie = 1 - parawid.sledzenie;
			if (parawid.sledzenie)
			{
				parawid.oddalenie = parawid.oddalenie_3; parawid.kat_kam_z = parawid.kat_kam_z_3;
			}
			else
			{
				parawid.oddalenie_3 = parawid.oddalenie; parawid.kat_kam_z_3 = parawid.kat_kam_z;
				parawid.widok_z_gory = 0;
				parawid.pol_kamery = parawid.pol_kamery_1; parawid.kierunek_kamery = parawid.kierunek_kamery_1; 
				parawid.pion_kamery = parawid.pion_kamery_1;
				parawid.oddalenie = parawid.oddalenie_1; parawid.kat_kam_z = parawid.kat_kam_z_1;
			}
			break;
		}
		case VK_ESCAPE:
		{
			SendMessage(okno, WM_DESTROY, 0, 0);
			break;
		}
		} // switch po klawiszach

		break;
	}
	case WM_KEYUP:
	{
		switch (LOWORD(wParam))
		{
		case VK_SHIFT:
		{
			SHIFTwcisniety = 0;
			break;
		}
		case VK_SPACE:
		{
			moj_pojazd->ham = 0.0;
			break;
		}
		case VK_UP:
		{
			moj_pojazd->F = 0.0;

			break;
		}
		case VK_DOWN:
		{
			moj_pojazd->F = 0.0;
			break;
		}
		case VK_LEFT:
		{
			moj_pojazd->Fb = 0.00;
			//moj_pojazd->state.steering_angle = 0;
			if (moj_pojazd->czy_kierownica_trzymana) moj_pojazd->predkosc_krecenia_kierownica = -0.25 / 8;
			else moj_pojazd->predkosc_krecenia_kierownica = 0;
			moj_pojazd->czy_kierownica_trzymana = false;
			break;
		}
		case VK_RIGHT:
		{
			moj_pojazd->Fb = 0.00;
			//moj_pojazd->state.steering_angle = 0;
			if (moj_pojazd->czy_kierownica_trzymana) moj_pojazd->predkosc_krecenia_kierownica = 0.25 / 8;
			else moj_pojazd->predkosc_krecenia_kierownica = 0;
			moj_pojazd->czy_kierownica_trzymana = false;
			break;
		}

		}

		break;
	}

	default: //standardowa obs³uga pozosta³ych meldunków
		return DefWindowProc(okno, kod_meldunku, wParam, lParam);
	}


}

