#include <windows.h>
#include <math.h>
#include <time.h>

#include <iterator> 
#include <map>
#include <vector>

#include "wektor.h"
#include "kwaternion.h"
#include "siec.h"

using namespace std;
#define CLIENT_TIMEOUT 5

struct StanObiektu
{
	//int iID;                  // identyfikator obiektu
	Wektor3 wPol;             // polozenie obiektu (œrodka geometrycznego obiektu) 
	kwaternion qOrient;       // orientacja (polozenie katowe)
	Wektor3 wV, wA;            // predkosc, przyspiesznie liniowe
	Wektor3 wV_kat, wA_kat;   // predkosc i przyspieszenie liniowe
	float masa;
	float kat_skretu_kol;
};

struct ClientData {
	int id;
	unsigned long ip;
	unicast_net *send_socket;
	long last_contact;
};

map<int, ClientData*> clients;
vector<int> delete_queue;
unicast_net *uni_reciv;         // wsk do obiektu zajmujacego sie odbiorem komunikatow
//unicast_net *uni_send;          //   -||-  wysylaniem komunikatow

enum typy_ramek{ STAN_OBIEKTU, NIEAKTYWNOSC };

struct Ramka                                    // g³ówna struktura s³u¿¹ca do przesy³ania informacji
{
	int typ;
	int iID;
	unsigned long listen_port;
	long moment_wyslania;
	StanObiektu stan;
};


int main() 
{
	printf("Starting server... ");
	uni_reciv = new unicast_net(10000);
	//uni_send = new unicast_net(10001);
	Ramka ramka;
	unsigned long sender_ip;
	unsigned long sender_port;

	while (1)
	{
		int rozmiar = uni_reciv->reciv((char*)&ramka, &sender_ip, sizeof(Ramka));
		if (rozmiar > 0) {
			switch (ramka.typ)
			{
			case STAN_OBIEKTU:
			{
				if ((clients.size() == 0) || (clients[ramka.iID] == NULL)) {
					printf("Registering IP: %d PORT %d CLIENT_ID: %d\n", sender_ip, ramka.listen_port, ramka.iID);
					ClientData *data = new ClientData();
					data->ip = sender_ip;
					data->id = ramka.iID;
					data->send_socket = new unicast_net(ramka.listen_port);
					clients[ramka.iID] = data;
				}

				clients[ramka.iID]->last_contact = clock();

				for (std::map<int, ClientData*>::iterator it = clients.begin(); it != clients.end(); it++) {
					if (it->first != ramka.iID) {
						it->second->send_socket->send((char*)&ramka, it->second->ip, sizeof(Ramka));
					}
						
				}
				break;
			} // case STAN_OBIEKTU

			} // switch
		}

		float current_time = clock();

		for (std::map<int, ClientData*>::iterator it = clients.begin(); it != clients.end(); it++) {
			if (((current_time - it->second->last_contact) / CLOCKS_PER_SEC) > CLIENT_TIMEOUT)
				delete_queue.push_back(it->first);
		}

		for (int i = 0; i < delete_queue.size(); i++) {
			int id = delete_queue[i];
			printf("Unregistering IP: %d  CLIENT_ID: %d\n", clients[id]->ip, id);
			ClientData* data = clients[id];
			clients.erase(id);
			delete data->send_socket;
			delete data;
		}

		while (delete_queue.size() > 0) {
			int id = delete_queue.back();
			delete_queue.pop_back();
			Ramka ramka;
			ramka.iID = id;
			ramka.typ = NIEAKTYWNOSC;
			for (std::map<int, ClientData*>::iterator it = clients.begin(); it != clients.end(); it++) {
				if (it->first != ramka.iID)
					it->second->send_socket->send((char*)&ramka, it->second->ip, sizeof(Ramka));
			}
		}

		Sleep(1);
	} 
	return 0;
}

