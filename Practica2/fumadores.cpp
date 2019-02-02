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
const int NFUMADORES = 3;

template< int min, int max > int aleatorio()
{
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
  return distribucion_uniforme( generador );
}

void fumar( int num_fumador )
{
	mtx.lock();
   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_fumar( aleatorio<20,200>() );

   // informa de que comienza a fumar

    cout << "Fumador " << num_fumador << ":"
          << " empieza a fumar (" << duracion_fumar.count() << " milisegundos)" << endl;

   // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
   this_thread::sleep_for( duracion_fumar );

   // informa de que ha terminado de fumar

    cout << "Fumador " << num_fumador << ": termina de fumar, comienza espera de ingrediente." << endl;

    mtx.unlock();
}

int ProducirIngrediente(){
	chrono::milliseconds duracion_producir( aleatorio<20,200>());
	this_thread::sleep_for( duracion_producir);
	int random = aleatorio<0,2>();
	mtx.lock();
	cout << "El estanquero pone en el mostrador el ingrediente: " <<  random << endl;
	mtx.unlock();
	return random;
}

class Estanco : public HoareMonitor{
	private:
		CondVar fumar[NFUMADORES], estanquero;
		bool fumando; 

	public:
		Estanco();
		void obtenerIngrediente(int i);
		void PonerIngrediente(int ingre);
		void EsperarRecogidaIngrediente();
};

Estanco::Estanco(){
	estanquero = newCondVar();

	for(int i = 0; i < NFUMADORES; ++i)
		fumar[i] = newCondVar();

	fumando = false;
}

void Estanco::obtenerIngrediente(int i){
	if(fumando)
		estanquero.signal();

	fumar[i].wait();
}

void Estanco::PonerIngrediente(int ingre){
	fumar[ingre].signal();
}

void Estanco::EsperarRecogidaIngrediente(){
	fumando = true;
	estanquero.wait();
	fumando = false;
}

void funcion_hebra_fumadores(MRef<Estanco> monitor, int i){
	while(true){
		monitor->obtenerIngrediente(i);
		fumar(i);
	}
}

void funcion_hebra_estanquero(MRef<Estanco> monitor){
	while(true){
		int ingre = ProducirIngrediente();
		monitor->PonerIngrediente(ingre);
		monitor->EsperarRecogidaIngrediente();
	}
}

int main(){
	MRef<Estanco> monitor = Create<Estanco>();

	thread hebra_estanquero(funcion_hebra_estanquero, monitor), hebras_fumadores[NFUMADORES];

	for(int i = 0; i < NFUMADORES; ++i)
		hebras_fumadores[i] = thread(funcion_hebra_fumadores, monitor, i);


   	hebra_estanquero.join();
   	for(int i = 0; i < NFUMADORES; ++i)
   		hebras_fumadores[i].join();

   	return 0;
}
