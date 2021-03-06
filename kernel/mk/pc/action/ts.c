/*
 * File: mk/pc/action/ts.c
 *
 *     Task Switching.
 *
 *     +Save the context of a thread.
 *     +Call scheduler to switch the thread.
 *     +Call dispatcher to dispatch a thread.
 *
 * History:
 *     2015 - Created by Fred Nora.
 *     2016 - 2019 Revision.
 *     //...
 */


#include <kernel.h>
 
 
//
// Vari�veis internas.
//
  
int lock_taskswitch;  
//int __taskswitch_lock;
//...


  
//  
// Internal.  
//

void taskswitchRR();
//...



//
// ...
//

/*
void taskswitchFlushTLB();
void taskswitchFlushTLB(){
	//@todo: Implementar.
	return;
}
*/


/*
 * KiTaskSwitch:
 *     Interface para chamar a rotina de Task Switch.
 *     Essa rotina somente � chamada por hw.inc.
 *     KiTaskSwitch em ts.c gerencia a rotina de 
 * troca de thread, realizando opera��es de salvamento e 
 * restaura��o de contexto utilizado vari�veis globais e 
 * extrutura de dados, seleciona a pr�xima thread atrav�s 
 * do scheduler, despacha a thread selecionada atrav�s do 
 * dispatcher e retorna para a fun��o _irq0 em hw.inc, 
 * que configurar� os registradores e executar� a 
 * thread atrav�s do m�todo iret.
 *
 * #importante:
 * Na verdade, � uma interface pra uma rotina que 
 * faz tudo isso.
 * 
 */
 
/*
	// @todo: Fazer alguma rotina antes aqui ?!
	
	// Obs: A qui poderemos criar rotinas que n�o lidem com a troca de 
	// threads propriamente, mas com atualiza��es de vari�veis e gerenciamento 
	// de contagem.
	// >> Na entrada da rotina podemos atualizar a contagem da tarefa que acabou de rodar.
	// >> A rotina task_switch fica respons�vel apenas troca de contexto, n�o fazendo 
	// atualiza��o de vari�veis de contagem.
	// >> ?? Na sa�da ??
	
	// ?? quem atualizou as vari�veis de crit�rio de escolha ??? o dispacher ??
*/ 
 
void KiTaskSwitch (){
	
	//Limits.
	
	if ( current_thread < 0 || current_thread >= THREAD_COUNT_MAX )
	{	
	    printf("KiTaskSwitch error: current_thread TID={%d}", 
		    current_thread );										   
        die();
	}
	
	if ( current_process < 0 || current_process >= PROCESS_COUNT_MAX )
	{
		
	    printf("KiTaskSwitch error: current_thread TID={%d}",
		    current_process );										   
        die();
	}
	
	debug_print("ts ");
	

	// ## Task switch ##
	
	task_switch ();
	
    // obs: 
	// Nessa hora ja temos um thread atual e um processo atual selecionados.
    // podemos checar as vari�veis para conferir se n�o est� fora dos limites.
    // Se estiverem fora dos limites, podemos usar algum etodo para selecioanrmos 
    // outro processo ou outra thread com limites v�lidos.
	
	//Retornando para _irq0 em head\x86\hw.inc.
};


/*
 **********************************************************
 * task_switch:
 *
 * Switch the thread.
 * Save and restore context.
 * Select the next thread and dispatch.
 * return to _irq0.
 * Called by KiTaskSwitch.
 */
 
void task_switch (){

	int New;
	int Max;
	
	struct thread_d *Current;
	struct process_d *P;

	Max = PRIORITY_MAX;


	// ## Current thread ##
	
	Current = (void *) threadList[current_thread]; 
	
	if ( (void *) Current == NULL )
	{
		printf ("ts-task_switch: Struct={%x}", (void *) Current );
		die();
	}

	// ## Current process ##

	P = (void *) Current->process;

	if ( (void *) P == NULL )
	{

		printf("task_switch error: P Struct={%x}", (void *) P );
		die();
	}
	
	if ( (void *) P != NULL )
	{
		if ( P->used == 1 && P->magic == 1234 )
		{	
			current_process = (int) P->pid;
		}
		
		//...
	};

	//...


	// ## Conting ... ##

	
	//            step: Quantas vezes ela j� rodou no total.
	//    runningCount: Quanto tempo ela est� rodando antes de parar.

	Current->step++; 
	
	//quanto tempo em ms ele rodou no total.
	Current->total_time_ms = (unsigned long) Current->total_time_ms + (1000/sys_time_hz);	
	
	//incrementa a quantidade de ms que ela est� rodando antes de parar.
	//isso precisa ser zerado quando ela reiniciar no pr�ximo round.
	Current->runningCount_ms = (unsigned long) Current->runningCount_ms + (1000/sys_time_hz);	

	Current->runningCount++;
	
	// ## locked ##
	// Taskswitch locked, Retor without saving.
	
	if ( task_switch_status == LOCKED )
	{    		
		IncrementDispatcherCount (SELECT_CURRENT_COUNT);
		return;  
	}


	// ## unlocked ##
	
	if ( task_switch_status == UNLOCKED )
	{
		//
		// ## SAVE CONTEXT ##
		//
		
		save_current_context ();
		Current->saved = 1;	
						
		if ( Current->runningCount < Current->quantum )
		{
			IncrementDispatcherCount (SELECT_CURRENT_COUNT);
			return; 
		
		}else{
			
			//
			// ## PREEMPT ##
			//
		
			// * MOVEMENT 3 (Running --> Ready).
			if ( Current->state == RUNNING )
			{
				// MOVEMENT 3 (running >> ready)  
				Current->state = READY;    //Preempt.

				if ( Current->preempted == PREEMPTABLE )
				{
					debug_print(" preempt_q1 ");
					queue_insert_head ( queue, (unsigned long) Current, 
						QUEUE_READY );	
				};

				if ( Current->preempted == UNPREEMPTABLE )
				{
					debug_print(" preempt_q2 ");
					queue_insert_data( queue, (unsigned long) Current, 
						QUEUE_READY );	
				};
			};

			debug_print(" ok ");
			
			//
			// ## EXTRA ##
			//

			if (extra == 1)
			{
				KiRequest ();

				extra = 0;
			}

			// schedi.c
			check_for_standby (); 

			goto try_next;

		};
	};

//crazyFail:
	
	goto dispatch_current;      	
	
	//
	//    ####  NEXT ####
	//
	
try_next: 
	
	debug_print(" N ");
	
	if (ProcessorBlock.threads_counter == 1)
	{		
		debug_print(" JUSTONE ");
		Conductor = IdleThread;

		goto go_ahead;
	}
	
	if ( (void *) Conductor->Next == NULL )
	{	
		debug_print(" LAST ");	

		KiScheduler ();
		goto go_ahead;
	}
	
	if ( (void *) Conductor->Next != NULL )
	{		
		Conductor = (void *) Conductor->Next;	
		goto go_ahead;
		
	}else{ 
		//@todo: Se o pr�ximo for NULL.
		//goto go_ahead;
	}
	
go_ahead:

	//##############################################//
	//                                              //
	//    #### We have a thread now ####            //
	//                                              //
	//##############################################//

	Current = (void *) Conductor;
		
	if( (void *) Current == NULL )
	{ 		
		debug_print(" Struct ");
		
		KiScheduler ();
		
		goto try_next;
		
	}else{
		
		if ( Current->used != 1 || Current->magic != 1234 )
		{
			debug_print(" val ");
			KiScheduler ();
			goto try_next;	
		}	
		
		if ( Current->state != READY )
		{
			debug_print(" state ");		
			KiScheduler ();
			goto try_next;	
		}

		//
		//    ####  Dispatcher ####
		//

		IncrementDispatcherCount (SELECT_DISPATCHER_COUNT);
		
		current_thread = (int) Current->tid;
		
		goto dispatch_current;
	}
	
	//
	// # fail #
	//
	
 	
//superCrazyFail:
	
	goto dispatch_current; 
	
	//
	//    ####  Dispatch current ####
	//	

dispatch_current:
	
	debug_print(" DISPATCH_CURRENT \n");

	//
	//    ####  Validation ####
	//	

	Current = (void *) threadList[current_thread];
	
	if ( (void *) Current == NULL )
	{	
		panic ("pc-action-ts-task_switch.dispatch_current: Struct ERROR");
	
	}else{
				
		if ( Current->used != 1 || 
			 Current->magic != 1234 || 
			 Current->state != READY )
		{
			panic ("pc-action-ts-task_switch.dispatch_current: validation ERROR");
		}

		Current->runningCount = 0;	
	}
	
	//
	// #### Call dispatcher #### 
	//
	
	//
	// * MOVEMENT 4 (Ready --> Running).
	//
	
	dispatcher (DISPATCHER_CURRENT);    	

	//
	//  #### DONE ####
	//

done:

	if ( Current->ownerPID < 0 || Current->ownerPID >= THREAD_COUNT_MAX )
	{
		printf ("action-ts-task_switch: ownerPID ERROR \n", Current->ownerPID );
		die();
	}
	
	//
	//    ## PROCESS ## 
	//
	
	
	P = (void *) processList[Current->ownerPID];
	
	if ( (void *) P == NULL )
	{
		printf("action-ts-task_switch: Process %s struct fail \n", P->name_address );
		die();
	}
	
	if ( (void *) P != NULL )
	{
		if ( P->used != 1 || P->magic != 1234 )
		{
			printf("action-ts-task_switch: Process %s corrompido \n", 
				P->name_address );
			die();
		}

		if ( P->used == 1 && P->magic == 1234 )
		{
			current_process = (int) P->pid;

			if ( (unsigned long) P->DirectoryPA == 0 )
			{	
				printf ("action-ts-task_switch: Process %s directory fail\n", 
					P->name_address );
				die();
			}
			
			current_process_pagedirectory_address = (unsigned long) P->DirectoryPA;
			goto doneRET;
		}
		
		panic ("action-ts-task_switch: * Struct * \n");
	}

	panic ("action-ts-task_switch: bug sinistro kkk \n");

doneRET:
	return; 
}


/*
 * taskswitchRR:
 *     Task switch usando Round Robin.
 *     Obs: Esse m�todo ainda n�o foi habilitado.
 */
void taskswitchRR()
{
	int i;
	int Max = (int) ProcessorBlock.threads_counter;
	struct thread_d *Current; //Thread atual.
	
	//Filtro.
	//if(current_thread ...){}
	
	Current = (void *) threadList[current_thread]; 
	if((void *) Current == NULL)
	{
	    panic("taskswitchRR error: Struct");
		//die();
	};
	
	//
	//todo: Checar se a estrutura � v�lida. 
	//
	
	/*
	 *    Incrementa.
	 *     Continua na mesma tarefa enquanto a contagem de quantum for
	 *     menor que o quantum limite da tarefa atual.
	 *     Assim temos uma contagem de vezes que uma tarefa deve ficar
     *     no processador sem sair.	 
	 */	


	//se o runningCount ainda � menor que a cota, dispacha.
	//Obs: Cuidado !! A m�dia � (PRIORITY_NORMAL*2)
	if(Current->runningCount < Current->quantum){ 
		goto dispatch_current;
	}else{
		
        //?? @todo: Talvez aqui seja o movimento 3, running >> ready. Conferir.  		
		Current->state = READY;
		
		//
		// @todo: Nesse momento, colocar no fim da fila ready. tail
		//
		//queue_insert_data( queue, (unsigned long) Current, QUEUE_READY);
		
		//se ouver outra v�lida nos slots despacha.
		i = (int) current_thread;
				
		do
		{
		    i++;
			
			//Contando++
			if(i < Max)
			{
		        Current = (void *) threadList[i];
		        if( (void *) Current != NULL )
			    {
			        if( Current->used == 1 && 
					    Current->magic == 1234 && 
						Current->state == READY)
				    {
				        Current->runningCount = 0;
				        current_thread = (int) i;
				        goto dispatch_current;
				    };
			    };
			};
			
			// Overflow
			if(i >= Max)
			{ 
			    i = 0; 
				Current = (void *) threadList[i];
			    if( (void *) Current != NULL )
				{
				    if( Current->used == 1 && 
					    Current->magic == 1234 && 
						Current->state == READY )
					{
					    Current->runningCount = 0;
					    current_thread = (int) i;
						goto dispatch_current;
					};
				};
			};

		}while(i < Max);
			
        panic("taskswitchRR error: *HANG");
  			
		//Use idle. N�o havendo outra alem da idle usa ela.
		//current_idle_thread.
		//current_thread = (int) 0;
		//goto dispatch_current;
	}; 
	
//	
// Done.
//
dispatch_current:	
	dispatcher(DISPATCHER_CURRENT);	
	return;	
};




/*
 ***************************************************
 * set_task_status:
 *    Configura o status do mecanismo de task switch.
 *    Se o mecanismo de taskswitch estiver desligado 
 * n�o ocorrer� a mudan�a.
 *
 * @todo: Mudar o nome dessa fun��o para taskswitchSetStatus(.);
 *
 */ 
void set_task_status( unsigned long status )
{
    //#bugbug: Mudar para int.	
	task_switch_status = (unsigned long) status;
};


/*
 **************************************************
 * get_task_status:
 *     Obtem o status do mecanismo de taskswitch.
 *
 * @todo: Mudar o nome dessa fun��o para taskswitchGetStatus();.
 */
unsigned long get_task_status()
{
    //#bugbug: Mudar para int.		
    return (unsigned long) task_switch_status;
};


/*
 **********************************************
 * taskswitch_lock:
 *     Trava o mecanismo de taskswitch.
 *     @todo: Mudar para taskswitchLock().
 */ 
void taskswitch_lock (){
	
    task_switch_status = (unsigned long) LOCKED;
};


/*
 *********************************************
 * taskswitch_unlock:
 *     Destrava o mecanismo de taskswitch.
 *     @todo: Mudar para taskswitchUnlock().
 */ 
void taskswitch_unlock (){
	
    task_switch_status = (unsigned long) UNLOCKED;
};


//
// End.
//
