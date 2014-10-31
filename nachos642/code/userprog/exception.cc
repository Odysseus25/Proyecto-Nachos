// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#define FALSE  0
#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include "../threads/utility.h"
#include "nachostabla.h"

    NachosOpenFilesTable* tabla = new NachosOpenFilesTable();
//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------

void returnFromSystemCall() {

        int pc, npc;

        pc = machine->ReadRegister( PCReg );
        npc = machine->ReadRegister( NextPCReg );
        machine->WriteRegister( PrevPCReg, pc );        // PrevPC <- PC
        machine->WriteRegister( PCReg, npc );           // PC <- NextPC
        machine->WriteRegister( NextPCReg, npc + 4 );   // NextPC <- NextPC + 4

}       // returnFromSystemCall

void Nachos_Halt() {                    // System call 0

        DEBUG('a', "Shutdown, initiated by user program.\n");
        interrupt->Halt();

}       // Nachos_Halt

// Pass the user routine address as a parameter for this function
// This function is similar to "StartProcess" in "progtest.cc" file under "userprog"
// Requires a correct AddrSpace setup to work well

void NachosForkThread( int p ) {

    AddrSpace *space;

    space = currentThread->space;
    space->InitRegisters();             // set the initial register values
    space->RestoreState();              // load page table register

// Set the return address for this thread to the same as the main thread
// This will lead this thread to call the exit system call and finish
    machine->WriteRegister( RetAddrReg, 4 );

    machine->WriteRegister( PCReg, p );
    machine->WriteRegister( NextPCReg, p + 4 );

    machine->Run();                     // jump to the user progam
    ASSERT(FALSE);

}


void Nachos_Fork() {			// System call 9

	DEBUG( 'u', "Entering Fork System call\n" );
	// We need to create a new kernel thread to execute the user thread
	Thread * newT = new Thread( "child to execute Fork code" );

	// We need to share the Open File Table structure with this new child

	// Child and father will also share the same address space, except for the stack
	// Text, init data and uninit data are shared, a new stack area must be created
	// for the new child
	// We suggest the use of a new constructor in AddrSpace class,
	// This new constructor will copy the shared segments (space variable) from currentThread, passed
	// as a parameter, and create a new stack for the new child
	newT->space = new AddrSpace( currentThread->space );

	// We (kernel)-Fork to a new method to execute the child code
	// Pass the user routine address, now in register 4, as a parameter
	newT->Fork( NachosForkThread, machine->ReadRegister( 4 ) );

	returnFromSystemCall();	// This adjust the PrevPC, PC, and NextPC registers

	DEBUG( 'u', "Exiting Fork System call\n" );
}	// Kernel_Fork

void Nachos_Open() {                    // System call 5
	int valor = machine->ReadRegister(4);
	int letra;
	char buffer[100];	//queda guardado el mensaje del registro
	bool seguir;
	int i = 0;
	while(seguir == true){
		machine->ReadMem(valor, 1, &letra);
		buffer[i] = (char)letra;
		if((char) letra != '\0'){
			seguir = false;	
		}
	}
	//tabla.Open(); 
	// Read the name from the user memory, see 4 below
	// Use NachosOpenFilesTable class to create a relationship
	// between user file and unix file
	// Verify for errors

}       // Nachos_Open

void Nachos_Write() {                   

    int size = machine->ReadRegister( 5 );	

    OpenFileId id = machine->ReadRegister( 6 );	
    int indiceUnix = tabla->getUnixHandle(id);


    int bufferVirtual = machine->ReadRegister(4);
    int caracterLeido;
    int cantCaracteres = 0;

    char buffer[size];
    for(int i=0; i<size; ++i){
        machine->ReadMem(bufferVirtual, 1, &caracterLeido);
        buffer[i] = (char)caracterLeido;
        ++bufferVirtual;
        ++cantCaracteres;
        caracterLeido = 0;
    }
    switch (id) {

    case  ConsoleInput:	
        machine->WriteRegister( 2, -1 );
        break;
    case  ConsoleOutput:
        buffer[ size ] = 0;
        stats->numConsoleCharsWritten++;
        printf( "%s \n", buffer );
        break;
    case ConsoleError: 
        printf( "%d\n", machine->ReadRegister( 4 ) );
        break;
    default:
        bool abierto = tabla->isOpened(id);
        if(abierto == true){
            write(indiceUnix, buffer, size);
            machine->WriteRegister(2, cantCaracteres);
            stats->numDiskWrites++;
        }else{
            printf("El archivo no está abierto\n");
            machine->WriteRegister(2, -1);
        }
    }
    returnFromSystemCall();
} 

void ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);

    switch ( which ) {

       case SyscallException:
          switch ( type ) {
             case SC_Halt:
                Nachos_Halt();             // System call # 0
                break;
             case SC_Open:	//hacer este que recupera el nombre del archivo del registro 4 caracter por caracter, 
                Nachos_Open();             // System call # 5
                break;
             case SC_Write:
                Nachos_Write();             // System call # 7
                break;                
             case SC_Fork:		// System call # 9
                Nachos_Fork();
                break;

             default:
                printf("Unexpected syscall exception %d\n", type );
                ASSERT(FALSE);
                break;
          }
       break;
       default:
          printf( "Unexpected exception %d\n", which );
          ASSERT(FALSE);
          break;
    }
}
