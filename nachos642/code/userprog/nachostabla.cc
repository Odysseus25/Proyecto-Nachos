#include "nachostabla.h"
#include <iostream>

int usage;			//how many threads are using this table
int * openFiles;		// A vector with user opened files
//BitMap * openFilesMap;

using namespace std;

NachosOpenFilesTable::NachosOpenFilesTable(){ // crea la tabla q es un vector
	usage = 0;
	openFiles = new int[128];
	for(int i = 0; i < 128; ++i){
		openFiles[i] = -1;
	}
}

NachosOpenFilesTable::~NachosOpenFilesTable(){
	delete openFiles;
}

int NachosOpenFilesTable::Open(int UnixHandle){ //ingresa el ID del parametro a la tabla creada si este no existe
	int i = 0;
	bool abierto = false;
	while(i < usage){
		if(openFiles[i] == UnixHandle){
			abierto = true;
		}
		++i;
	}
	if(abierto == false){
		openFiles[usage] = UnixHandle;
		usage++;
	}
	return usage;
}

int NachosOpenFilesTable::Close(int NachosHandle){	//borrar el archivo de la posiscion q entra por parametro
	if(openFiles[NachosHandle] != -1){
		openFiles[NachosHandle] = -1;
	}
	return 0;
}

bool NachosOpenFilesTable::isOpened(int NachosHandle){	//busca en la tabla en la posicion q se le pasa por parametro a ver si esta abierto
	bool abierto = false;
	if(openFiles[NachosHandle] != -1){
		abierto = true;
	}
	return abierto;
}

int NachosOpenFilesTable::getUnixHandle(int NachosHandle){ 	//devuelve el ID q se encuentra en la tabla en esa posicion
	return openFiles[NachosHandle];
}

void NachosOpenFilesTable::addThread(){
	usage++;
}

void NachosOpenFilesTable::delThread(){
	usage--;
}

void NachosOpenFilesTable::Print(){	//cuidado falla
	cout << "Contenidos de la tabla de archivos: " << endl;
	for(int i = 0; i < usage; ++i){
		cout << openFiles[i] << endl;
	}
}
