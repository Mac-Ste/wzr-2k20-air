/****************************************************
		Wirtualne zespoly robocze - przykladowy projekt w C++
		Do zada� dotycz�cych wsp�pracy, ekstrapolacji i
		autonomicznych obiekt�w
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


ObiektRuchomy *moj_pojazd;          // obiekt przypisany do tej aplikacji
Teren teren;

struct KlientServer {
	long ip;
	long last_contact;
	long obj_id;
	StanObiektu* stan;
};

map<int, ObiektRuchomy*> obiekty_ruchome;
map<long, KlientServer*> klienci_server;

float fDt;                          // sredni czas pomiedzy dwoma kolejnymi cyklami symulacji i wyswietlania
long czas_cyklu_WS, licznik_sym;     // zmienne pomocnicze potrzebne do obliczania fDt
long czas_start = clock();          // czas uruchomienia aplikacji
long dlugosc_dnia = 600;         // czas trwania dnia w [s]

char* local_ip = "192.168.1.16";
unicast_net *uni_reciv_client;         
unicast_net *uni_send_client;          

unicast_net *uni_reciv_server;
unicast_net *uni_send_server;


HANDLE threadReciv;                 // uchwyt w�tku odbioru komunikat�w klienta

HANDLE threadServer;                 // uchwyt w�tku servera

CRITICAL_SECTION m_cs;              // do synchronizacji w�tk�w
extern HWND okno;
int SHIFTwcisniety = 0;
bool czy_rysowac_ID = 1;            // czy rysowac nr ID przy ka�dym obiekcie
bool sterowanie_myszkowe = 0;       // sterowanie za pomoc� klawisza myszki
int kursor_x = 0, kursor_y = 0;     // po�o�enie kursora myszy


extern ParametryWidoku parawid;      // ustawienia widoku zdefiniowane w grafice


enum typy_ramek{ STAN_OBIEKTU, ZAMIANA };


struct Ramka                                    // g��wna struktura s�u��ca do przesy�ania informacji
{
	int typ;
	int iID;
	long moment_wyslania;
	StanObiektu stan;
};

DWORD WINAPI ServerHandler(void *ptr);


//******************************************
// Funkcja obs�ugi w�tku odbioru komunikat�w 
DWORD WINAPI FunkcjaWatkuOdbioru(void *ptr)
{
	unicast_net *pmt_net = uni_reciv_client;                // wska�nik do obiektu klasy multicast_net
	int rozmiar;                                               // liczba bajt�w ramki otrzymanej z sieci
	Ramka ramka;
	StanObiektu stan;
	unsigned long ip = 0;

	while (1)
	{
		rozmiar = pmt_net->reciv((char*)&ramka, &ip, sizeof(Ramka));   // oczekiwanie na nadej�cie ramki - funkcja samoblokuj�ca si� 
		switch (ramka.typ)
		{
		case STAN_OBIEKTU:
		{
			stan = ramka.stan;

			if (ramka.iID != moj_pojazd->iID)                     // je�li to nie m�j obiekt
			{
				// Lock the Critical section
				EnterCriticalSection(&m_cs);               // wej�cie na �cie�k� krytyczn� - by inne w�tki (np. g��wny) nie wsp�dzieli� 
				if ((obiekty_ruchome.size() == 0)||(obiekty_ruchome[ramka.iID] == NULL))                     // nie ma jeszcze takiego obiektu w tablicy -> trzeba go stworzy�
				{
					ObiektRuchomy *ob = new ObiektRuchomy(&teren);
					ob->iID = ramka.iID;
					obiekty_ruchome[ramka.iID] = ob;
				}
				obiekty_ruchome[ramka.iID]->ZmienStan(stan);   // zmiana stanu obiektu obcego 
				//Release the Critical section
				LeaveCriticalSection(&m_cs);               // wyj�cie ze �cie�ki krytycznej
			}
			break;
		} // case STAN_OBIEKTU
		case ZAMIANA: {
			moj_pojazd->ZmienStan(ramka.stan);
			moj_pojazd->iID = ramka.iID;
			break;
		}
		} // switch
	}  // while(1)
	return 1;
}

//******************************************
// Funkcja obs�ugi w�tku servera
DWORD WINAPI ServerHandler(void *ptr)
{
	int rozmiar;                                               // liczba bajt�w ramki otrzymanej z sieci
	Ramka ramka;
	StanObiektu stan;
	unsigned long ip = 0;

	while (1)
	{
		rozmiar = uni_reciv_server->reciv((char*)&ramka, &ip, sizeof(Ramka));   // oczekiwanie na nadej�cie ramki - funkcja samoblokuj�ca si� 
		if (klienci_server[ramka.iID] == NULL) {
			klienci_server[ramka.iID] = new KlientServer();
			klienci_server[ramka.iID]->ip = ip;
			klienci_server[ramka.iID]->obj_id = ramka.iID;
			klienci_server[ramka.iID]->stan = new StanObiektu();
		}

		klienci_server[ramka.iID]->last_contact = ramka.moment_wyslania;

		switch (ramka.typ)
		{
		case STAN_OBIEKTU:
		{
			stan = ramka.stan;

			
			klienci_server[ramka.iID]->stan->kat_skretu_kol = ramka.stan.kat_skretu_kol;
			klienci_server[ramka.iID]->stan->masa = ramka.stan.masa;
			klienci_server[ramka.iID]->stan->qOrient = ramka.stan.qOrient;
			klienci_server[ramka.iID]->stan->wA = ramka.stan.wA;
			klienci_server[ramka.iID]->stan->wA_kat = ramka.stan.wA_kat;
			klienci_server[ramka.iID]->stan->wPol = ramka.stan.wPol;
			klienci_server[ramka.iID]->stan->wV = ramka.stan.wV;
			klienci_server[ramka.iID]->stan->wV_kat = ramka.stan.wV_kat;

			for (map<long, KlientServer*>::iterator it = klienci_server.begin(); it != klienci_server.end(); it++)
			{
				uni_send_server->send((char*)&ramka, it->second->ip, sizeof(Ramka));
			}
			
			break;
		} // case STAN_OBIEKTU
		case ZAMIANA: {
			int destId = klienci_server[ramka.iID]->stan->requestedChange;
			if (destId != 0 && destId != ramka.iID) {
				int srcId = klienci_server[destId]->stan->requestedChange;
				if (srcId == ramka.iID) {
					Ramka odpowiedz;
					odpowiedz.iID = srcId;
					odpowiedz.stan = *klienci_server[srcId]->stan;
					odpowiedz.typ = ZAMIANA;

					uni_send_server->send((char*)&odpowiedz, klienci_server[destId]->ip, sizeof(Ramka));

					odpowiedz.iID = destId;
					odpowiedz.stan = *klienci_server[destId]->stan;
					odpowiedz.typ = ZAMIANA;
					uni_send_server->send((char*)&odpowiedz, klienci_server[srcId]->ip, sizeof(Ramka));

					klienci_server[srcId]->obj_id = destId;
					klienci_server[destId]->obj_id = srcId;

					KlientServer* tmp = klienci_server[srcId];
					klienci_server[srcId] = klienci_server[destId];
					klienci_server[destId] = tmp;
				}
				else {
					klienci_server[ramka.iID]->stan->requestedChange = 0;
				}
			}
			else {
				Wektor3 sourcePos = klienci_server[ramka.iID]->stan->wPol;
				long idForChange = ramka.iID;
				for (map<long, KlientServer*>::iterator it = klienci_server.begin(); it != klienci_server.end(); it++)
				{
					if (ramka.iID != it->second->obj_id) {
						float dist = (sourcePos - it->second->stan->wPol).dlugosc();
						if (dist < 100)
						{
							idForChange = it->second->obj_id;
							break;
						}
					}
				}

				if (idForChange != ramka.iID) {
					klienci_server[ramka.iID]->stan->requestedChange = idForChange;
					klienci_server[idForChange]->stan->requestedChange = ramka.iID;
				}
				else {
					klienci_server[ramka.iID]->stan->requestedChange = 0;
				}
			}
	


			break;
		}
		} // switch
	}  // while(1)
	return 1;
}


// *****************************************************************
// ****    Wszystko co trzeba zrobi� podczas uruchamiania aplikacji
// ****    poza grafik�   
void PoczatekInterakcji()
{
	DWORD dwThreadId;

	moj_pojazd = new ObiektRuchomy(&teren);    // tworzenie wlasnego obiektu


	czas_cyklu_WS = clock();             // pomiar aktualnego czasu

	// obiekty sieciowe typu multicast (z podaniem adresu WZR oraz numeru portu)
	uni_reciv_client = new unicast_net(35090);      // obiekt do odbioru ramek sieciowych
	uni_send_client = new unicast_net(35091);       // obiekt do wysy�ania ramek
	
	uni_reciv_server = uni_send_client;      // ob	iekt do odbioru ramek sieciowych
	uni_send_server = uni_reciv_client;       // obiekt do wysy�ania ramek

	// uruchomienie watku obslugujacego serwer
	threadReciv = CreateThread(
		NULL,                        // no security attributes
		0,                           // use default stack size
		FunkcjaWatkuOdbioru,                // thread function
		(void *)0,               // argument to thread function
		0,                           // use default creation flags
		&dwThreadId);                // returns the thread identifier

	// uruchomienie watku obslugujacego odbior komunikatow
	threadServer = CreateThread(
		NULL,                        // no security attributes
		0,                           // use default stack size
		ServerHandler,                // thread function
		(void *)0,               // argument to thread function
		0,                           // use default creation flags
		&dwThreadId);                // returns the thread identifier

}


// *****************************************************************
// ****    Wszystko co trzeba zrobi� w ka�dym cyklu dzia�ania 
// ****    aplikacji poza grafik� 
void Cykl_WS()
{
	licznik_sym++;

	// obliczenie czasu fDt pomiedzy dwoma kolejnymi cyklami
	if (licznik_sym % 50 == 0)          // je�li licznik cykli przekroczy� pewn� warto��, to
	{                                   // nale�y na nowo obliczy� �redni czas cyklu fDt
		char text[200];
		long czas_pop = czas_cyklu_WS;
		czas_cyklu_WS = clock();
		float fFps = (50 * CLOCKS_PER_SEC) / (float)(czas_cyklu_WS - czas_pop);
		if (fFps != 0) fDt = 1.0 / fFps; else fDt = 1;
		sprintf(text, "WWC-lab 2019/20 temat 1, wersja e (%0.0f fps  %0.2fms)  ", fFps, 1000.0 / fFps);
		SetWindowText(okno, text); // wy�wietlenie aktualnej ilo�ci klatek/s w pasku okna			
	}


	moj_pojazd->Symulacja(fDt);                    // symulacja obiektu w�asnego 

	Ramka ramka;
	ramka.typ = STAN_OBIEKTU;
	ramka.stan = moj_pojazd->Stan();               // stan w�asnego obiektu 
	ramka.iID = moj_pojazd->iID;
	


	// wys�anie komunikatu o stanie obiektu przypisanego do aplikacji (moj_pojazd):    
	uni_send_client->send((char*)&ramka, local_ip, sizeof(Ramka));

	ramka.stan.wPol.x = ramka.stan.wPol.x - 10;
	ramka.iID = moj_pojazd->iID+1;
	uni_send_client->send((char*)&ramka, local_ip, sizeof(Ramka));

}

// *****************************************************************
// ****    Wszystko co trzeba zrobi� podczas zamykania aplikacji
// ****    poza grafik� 
void ZakonczenieInterakcji()
{
	fprintf(f, "Interakcja zosta�a zako�czona\n");
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
	WNDCLASS nasza_klasa; //klasa g��wnego okna aplikacji

	static char nazwa_klasy[] = "KlasaPodstawowa";

	//Definiujemy klase g��wnego okna aplikacji
	//Okreslamy tu wlasciwosci okna, szczegoly wygladu oraz
	//adres funkcji przetwarzajacej komunikaty
	nasza_klasa.style = CS_HREDRAW | CS_VREDRAW;
	nasza_klasa.lpfnWndProc = WndProc; //adres funkcji realizuj�cej przetwarzanie meldunk�w 
	nasza_klasa.cbClsExtra = 0;
	nasza_klasa.cbWndExtra = 0;
	nasza_klasa.hInstance = hInstance; //identyfikator procesu przekazany przez MS Windows podczas uruchamiania programu
	nasza_klasa.hIcon = 0;
	nasza_klasa.hCursor = LoadCursor(0, IDC_ARROW);
	nasza_klasa.hbrBackground = (HBRUSH)GetStockObject(GRAY_BRUSH);
	nasza_klasa.lpszMenuName = "Menu";
	nasza_klasa.lpszClassName = nazwa_klasy;

	//teraz rejestrujemy klas� okna g��wnego
	RegisterClass(&nasza_klasa);

	/*tworzymy okno g��wne
	okno b�dzie mia�o zmienne rozmiary, listw� z tytu�em, menu systemowym
	i przyciskami do zwijania do ikony i rozwijania na ca�y ekran, po utworzeniu
	b�dzie widoczne na ekranie */
	okno = CreateWindow(nazwa_klasy, "WWC-lab 2019/20 temat 1, wersja e", WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
		100, 50, 900, 750, NULL, NULL, hInstance, NULL);


	ShowWindow(okno, nCmdShow);

	//odswiezamy zawartosc okna
	UpdateWindow(okno);

	// G��WNA P�TLA PROGRAMU

	// pobranie komunikatu z kolejki je�li funkcja PeekMessage zwraca warto�� inn� ni� FALSE,
	// w przeciwnym wypadku symulacja wirtualnego �wiata wraz z wizualizacj�
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
			Cykl_WS();    // Cykl wirtualnego �wiata
			InvalidateRect(okno, NULL, FALSE);
		}
	}

	return (int)meldunek.wParam;
}

/********************************************************************
FUNKCJA OKNA realizujaca przetwarzanie meldunk�w kierowanych do okna aplikacji*/
LRESULT CALLBACK WndProc(HWND okno, UINT kod_meldunku, WPARAM wParam, LPARAM lParam)
{


	switch (kod_meldunku)
	{
	case WM_CREATE:  //meldunek wysy�any w momencie tworzenia okna
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

	case WM_DESTROY: //obowi�zkowa obs�uga meldunku o zamkni�ciu okna

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
			moj_pojazd->F = 40000.0;        // si�a pchaj�ca do przodu [N]
		break;
	}
	case WM_RBUTTONDOWN: //reakcja na prawy przycisk myszki
	{
		int x = LOWORD(lParam);
		int y = HIWORD(lParam);
		if (sterowanie_myszkowe)
			moj_pojazd->F = -20000.0;        // si�a pchaj�ca do tylu

		break;
	}
	case WM_MBUTTONDOWN: //reakcja na �rodkowy przycisk myszki : uaktywnienie/dezaktywacja sterwania myszkowego
	{
		sterowanie_myszkowe = 1 - sterowanie_myszkowe;
		kursor_x = LOWORD(lParam);
		kursor_y = HIWORD(lParam);
		break;
	}
	case WM_LBUTTONUP: //reakcja na puszczenie lewego przycisku myszki
	{
		if (sterowanie_myszkowe)
			moj_pojazd->F = 0.0;        // si�a pchaj�ca do przodu
		break;
	}
	case WM_RBUTTONUP: //reakcja na puszczenie lewy przycisk myszki
	{
		if (sterowanie_myszkowe)
			moj_pojazd->F = 0.0;        // si�a pchaj�ca do przodu
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
			moj_pojazd->ham = 10.0;       // stopie� hamowania (reszta zale�y od si�y docisku i wsp. tarcia)
			break;                       // 1.0 to maksymalny stopie� (np. zablokowanie k�)
		}
		case VK_UP:
		{

			moj_pojazd->F = 20000.0;        // si�a pchaj�ca do przodu
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
		case 'S':   // przybli�enie widoku
		{
			//pol_kamery = pol_kamery + kierunek_kamery*0.3; 
			if (parawid.oddalenie > 0) parawid.oddalenie *= 1.2;
			else parawid.oddalenie = 0.5;
			break;
		}
		case 'V':
		{
			EnterCriticalSection(&m_cs);
			Ramka ramka;
			ramka.typ = ZAMIANA;
			ramka.iID = moj_pojazd->iID;
			ramka.stan.requestedChange = 0;

			uni_send_client->send((char*)&ramka, local_ip, sizeof(Ramka));
			LeaveCriticalSection(&m_cs);
			break;
		}
		case 'B':
		{
			EnterCriticalSection(&m_cs);
			Ramka ramka;
			ramka.typ = ZAMIANA;
			ramka.iID = moj_pojazd->iID+1;
			ramka.stan.requestedChange = 0;

			uni_send_client->send((char*)&ramka, local_ip, sizeof(Ramka));
			LeaveCriticalSection(&m_cs);
			LeaveCriticalSection(&m_cs);
			break;
		}
		case 'Q':   // widok z g�ry
		{
			//if (sledzenie) break;
			parawid.widok_z_gory = 1 - parawid.widok_z_gory;
			if (parawid.widok_z_gory)                // przechodzimy do widoku z g�ry
			{
				parawid.pol_kamery_1 = parawid.pol_kamery; parawid.kierunek_kamery_1 = parawid.kierunek_kamery; 
				parawid.pion_kamery_1 = parawid.pion_kamery;
				parawid.oddalenie_1 = parawid.oddalenie; parawid.kat_kam_z_1 = parawid.kat_kam_z;

				//pol_kamery = pol_kamery_2; kierunek_kamery = kierunek_kamery_2; pion_kamery = pion_kamery_2;
				parawid.oddalenie = parawid.oddalenie_2; parawid.kat_kam_z = parawid.kat_kam_z_2;
				// Po�o�enie kamery, kierunek oraz pion ustawiamy tak, by obiekt widziany by� z g�ry i jecha�
				// pocz�tkowo w g�r� ekranu:
				parawid.kierunek_kamery = (moj_pojazd->stan.wPol - teren.srodek).znorm()*(-1);
				parawid.pol_kamery = moj_pojazd->stan.wPol - parawid.kierunek_kamery*moj_pojazd->dlugosc * 10;
				parawid.pion_kamery = moj_pojazd->stan.qOrient.obroc_wektor(Wektor3(1, 0, 0));
			}
			else
			{
				parawid.pol_kamery_2 = parawid.pol_kamery; parawid.kierunek_kamery_2 = parawid.kierunek_kamery; 
				parawid.pion_kamery_2 = parawid.pion_kamery;
				parawid.oddalenie_2 = parawid.oddalenie; parawid.kat_kam_z_2 = parawid.kat_kam_z;

				// Po�o�enie kamery, kierunek oraz pion ustawiamy tak, by obiekt widziany by� z prawego boku i jecha�
				// pocz�tkowo ze strony lewej na praw�:
				parawid.kierunek_kamery = moj_pojazd->stan.qOrient.obroc_wektor(Wektor3(0, 0, 1))*-1;
				parawid.pol_kamery = moj_pojazd->stan.wPol - parawid.kierunek_kamery*moj_pojazd->dlugosc * 10;
				parawid.pion_kamery = (moj_pojazd->stan.wPol - teren.srodek).znorm();

				//pol_kamery = pol_kamery_1; kierunek_kamery = kierunek_kamery_1; pion_kamery = pion_kamery_1;
				parawid.oddalenie = parawid.oddalenie_1; parawid.kat_kam_z = parawid.kat_kam_z_1;
			}
			break;
		}
		case 'E':   // obr�t kamery ku g�rze (wzgl�dem lokalnej osi z)
		{
			parawid.kat_kam_z += PI * 5 / 180;
			break;
		}
		case 'D':   // obr�t kamery ku do�owi (wzgl�dem lokalnej osi z)
		{
			parawid.kat_kam_z -= PI * 5 / 180;
			break;
		}
		case 'A':   // w��czanie, wy��czanie trybu �ledzenia obiektu
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

	default: //standardowa obs�uga pozosta�ych meldunk�w
		return DefWindowProc(okno, kod_meldunku, wParam, lParam);
	}


}

