#include <iostream>
#include <iomanip>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include "HoareMonitor.h"

using namespace std;
using namespace HM;

mutex mtx;

const int NCLIENTES = 4;

template< int min, int max > int aleatorio()
{
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
  return distribucion_uniforme( generador );
}

void EsperarFueraBarberia(int i)
{
	mtx.lock();
   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion( aleatorio<20,200>() );

   // informa de que comienza a fumar

    cout << "Cliente " << i << ": " << " sale y espera fuera (" << duracion.count() << " milisegundos)." << endl;

   // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
   this_thread::sleep_for( duracion );

    mtx.unlock();
}

void CortarPeloAlCliente(){
   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_cortar( aleatorio<20,200>() );

   // informa de que comienza a fumar

    cout << "El barbero comienza a cortarle el pelo al cliente (" << duracion_cortar.count() << " milisegundos)." << endl;

   // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
   this_thread::sleep_for( duracion_cortar );

}

class Barberia : public HoareMonitor{
	private:
		CondVar cliente, barbero;
		bool dormido;

	public:
		Barberia();
		void siguienteCliente();
		void CortarPelo(int i);
		void finCliente();
};

Barberia::Barberia(){
	cliente = newCondVar();
	barbero = newCondVar();
	dormido = true;
}

void Barberia::CortarPelo(int i){
	if(dormido){
		cout << "El barbero está despierto." << endl;
		barbero.signal();			//Si está dormido, lo despierta.
	}

	cliente.wait();					//espera a ser llamado
}

void Barberia::siguienteCliente(){
	if(cliente.empty()){			//Si no hay clientes se duerme (se hace wait)
    	cout << "El barbero se duerme." << endl;
    	dormido = true;
    	barbero.wait();

    	dormido=false;
   }
}

void Barberia::finCliente(){
	cliente.signal();				//Cuando termina con un cliente, avisa al otro.
}

void funcion_hebra_barbero(MRef<Barberia> monitor){
	while(true){
		monitor->siguienteCliente();
		CortarPeloAlCliente();
		monitor->finCliente();
	}
}

void funcion_hebra_clientes(MRef<Barberia> monitor, int i){
	while(true){
		monitor->CortarPelo(i);
		EsperarFueraBarberia(i);
	}
}

int main(){
	MRef<Barberia> monitor = Create<Barberia>();

	thread hebra_barbero(funcion_hebra_barbero, monitor), hebras_clientes[NCLIENTES];

	for(int i = 0; i < NCLIENTES; ++i)
		hebras_clientes[i] = thread(funcion_hebra_clientes, monitor, i);


   	hebra_barbero.join();
   	for(int i = 0; i < NCLIENTES; ++i)
   		hebras_clientes[i].join();

   	return 0;
}