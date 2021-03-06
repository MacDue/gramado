/*
 * sm - System Management -  Esse � o console do sistema.
 * Seu objetivo principal � receber os comandos de gerenciamento,
 * enviados por usu�rios atr�ves de dispositivos de interface humana. 
 *
 * ********** #importante: Essas classes s�o 'system dialogs' ************
 *
 * Note duas coisas importantes aqui: 
 * o 'console' � a interface de administra��o do sistema, e � ele � 
 * acessado atrav�s de um emulador de terminais utilizando-se drivers TTY.
 *
 * File: system.c
 *
 * Obs: CADA CLASSE PODE TER UMA BIBLIOTECA EM USER MODE.
 *      UM UMA INTERRUP��O PR�PRIA EX: system1(....).
 *
 * Sugest�es para nomes de bibliotecas em user mode:
 * ================================================
 * ramlib.o  - Classe Ram. 
 * iocpulib.o - Classe Io.Cpu
 * iodmalib.o - Classe Io.Dma
 * dunblocked.o - Classe Devices.Unblocked
 * dblocked.o - Classe Devices.Blocked
 * things.o - Classe .Things.
 *
 * Ambiente:
 *     /executive/sm/sys/system.c
 *     Kernel base. Ring0.
 *
 * Descri��o:
 *     Arquivo principal do m�dulo de gerenciamento do sistema.
 *     Esse m�dulo est� em kernel mode, no kernel base.
 *     Rotinas de gerenciamento do sistema.
 *
 * Obs: Pode-se aprender muito com a comunidade opensource(linux), principalmente
 *      nas quest�es que apresentam maior dificuldade, dadas as obstru��es
 *      impostas pela ind�stria mainstream, como gerenciamento de energia
 *      protocolos e barramentos.
 *
 * Obs:
 *     Sobre a sondagem de dispositivos nos barramentos do sistema:
 *     Sondar os dispositivos seguindo a ordem de velocidade, so mais veloz
 *     para o menos veloz, exemplo: CPU, Mem�ria, PCIE, South Bridge, Super IO ...
 *
 * @todo:
 *     Criar um servi�o, (servidor), em user mode que configure o sistema. As 
 * aplica��es e utilit�rios em user mode poder�o usar esse servi�o. Obs: O servidor
 * em user mode poder� chamar servi�os oferecidos por esse m�dulo aqui.
 * 
 * @todo: AS ROTINAS DE CLASSES NO COME�O DESSE M�DULO DEVEM EXECUTAR SOMENTE
 *        DEPOIS DE UMA CHECAGEM DE PERMISS�ES. PERMISS�ES DE N�VEL. UM PROCESSO
 *        DEVE TER A MESMA PERMISS�O DE N�VEL DO SERVI�O QUE ELE DESEJA. (K0~K5).
 *
 * In this file:
 *     +Strings padronizadas e definitivas. Para uso interno.
 *     +systemShowDevicesInfo
 *     +systemCreateSystemMenuBar
 *     +systemStartUp - Rotinas de inicializa��o do sistema.  
 *     +systemSetupVersion
 *     +system_dispatch_to_procedure - Chama o procedimento atual.
 *     +SystemMenu
 *     +SystemMenuProcedure
 *     +systemReboot
 *     +systemShutdown
 *
 * Hist�rico:
 *    Vers�o 1.0, 2015 - Esse arquivo foi criado por Fred Nora.
 *    Vers�o 1.0, 2016 - Aprimoramento geral das rotinas b�sicas. 
 *    //...
 */

 
#include <kernel.h>


//
// Informa��es herdadas do boot.
//


extern unsigned long SavedBootBlock;
extern unsigned long SavedLFB;
extern unsigned long SavedX;
extern unsigned long SavedY;
extern unsigned long SavedBPP; 

 
//Vari�veis internas.
//int systemStatus;
//int systemError;
//... 


/*
    Sobre os diret�rios e arquivos do sistema:
    Os nomes dos diret�rios e arquivos do sistema ficar�o aqui
	como forma de registro oficial. os m�dulos poder�o reproduzir
	o nome presente aqui ou poder�o solicitar um ponteiro para o nome.
	@todo: fun��es aqui nesse arquivo retornar�o o ponteiro para o nome de arquivo
	desejado.
	...
 */
 
/*
 *     **** Diret�rios do sistema. ****  obs:(8.3)
 *
 * * Obs: O esquema de diret�rios ainda esta em fase de planejamento.
 */

//Geral para todas as versoes do sistema operacional 
static char *systemPathNameRoot      = "/root";               //Raiz.
static char *systemPathNameSDK       = "/root/sdk";           //*SDK. 
static char *systemPathNameBoot      = "/root/boot";          //Boot. 
static char *systemPathNameDevices   = "/root/devices";       //Devices. 
static char *systemPathNameUsers     = "/root/users";         //Usu�rios.
static char *systemPathNameApps      = "/root/apps";          //Aplica��es.
static char *systemPathNameTemp      = "/root/temp";          //Arquivos tempor�rios.
static char *systemPathNameDownloads = "/root/dl";            //Arquivos tempor�rios.
static char *systemPathNameTrash     = "/root/trash";         //Lixeira. 
//...

//OSs: versoes. 
static char *systemPathNameOS1 = "/root/os";   //OS1. Principal (atual)
static char *systemPathNameOS2 = "/root/os1";  //OS2.
static char *systemPathNameOS3 = "/root/os2";  //OS3.
static char *systemPathNameOS4 = "/root/os3";  //OS4.
//...
 
//Diret�rios do sistema principal. 
static char *systemPathNameOS1Boot           = "/root/os/boot";            //boot.
static char *systemPathNameOS1BootFont       = "/root/os/boot/font";       //boot font.
static char *systemPathNameOS1System         = "/root/os/system";          //* Diret�rio do sistema.
static char *systemPathNameOS1SystemFont     = "/root/os/system/font";            //fontes do sistema.
static char *systemPathNameOS1SystemDrivers  = "/root/os/system/drivers";  //Drivers do sistema.
static char *systemPathNameOS1SystemSecurity = "/root/os/system/security";    //Fontes sistema.
//...


/*
 * Arquivos do sistema:
 *     os principais arquivos do sistema, padronizados e nunca mudar�o.
 *
 */

//alguns s�o provis�rios nessa fase de desenvolvimento. 
static char *systemInitFilePathName = "/root/INIT.TXT";  //"/root/init.txt";
static char *systemLogFilePathName  = "/root/log.txt";
//static char *systemInitFilePathName = "/root/log.txt";
//static char *systemInitFilePathName = "/root/log.txt";
//static char *systemInitFilePathName = "/root/log.txt";
//static char *systemInitFilePathName = "/root/log.txt";
static char *systemSwapFilePathName      = "/root/swap";  //'Arquivo' de pagina��o.
//... 

//Listas.
//unsigned long systemFileListX[128];
//unsigned long systemFileListXX[128];


//Informa��es sobre a mem�ria do sistema
//n�o uasr ponteiro.
//struct memory_info_d systemMemoryInfo;






/*
 *******************************************************
 * set_up_system_color: 
 *     Configura cor padr�o para o sistema.
 *     @todo: Isso pode ir para outro lugar.   
 */
void set_up_color ( unsigned long color ){  
 
	g_system_color = (unsigned long) color;	
};


/* 
 *********************************************************************
 * set_up_text_color:
 *     Atribui o primeiro plano e o fundo que n�s usaremos. 
 *     Top 4 bytes are the background,  bottom 4 bytes
 *     are the foreground color.
 *     @todo: Isso pode ir para outro lugar.
 */
void set_up_text_color ( unsigned char forecolor, unsigned char backcolor ){
	
    g_char_attrib = (backcolor << 4) | (forecolor & 0x0F);	
};


/*
 ***************************************************************
 * set_up_cursor:
 *     Configura cursor.
 *     @todo: Isso pode ir para outro lugar.
 */
void set_up_cursor ( unsigned long x, unsigned long y ){   

	g_cursor_x = (unsigned long) x;
	g_cursor_y = (unsigned long) y;	
};


/*
 **************************************************
 * get_cursor_x:
 *     Pega o valor de x.
 *     @todo: Isso pode ir para outro lugar.
 */
unsigned long get_cursor_x (){   
	
	return (unsigned long) g_cursor_x;
};


/*
 **************************************************
 * get_cursor_y:
 *     Pega o valor de y.
 *     @todo: Isso pode ir para outro lugar.
 */
unsigned long get_cursor_y (){  
       
    return (unsigned long) g_cursor_y; 	
};


void *systemNull (){
	
	return NULL;
}


/*
 * systemLinkDriver:
 *     Ligando um driver ao sistema operacional.
 */

void *systemLinkDriver( unsigned long arg1, 
                        unsigned long arg2, 
						unsigned long arg3 )
{
	
	// #todo: 
	// Ainda n�o implementada.
	
	printf ("systemLinkDriver:\n");
    refresh_screen();	
    return NULL; 	
}


/*
 * ************************************************************
 * systemShowDevicesInfo:
 *     Mostrar informa��es sobre o sistema, seguindo a ordem de
 *     velocidade dos dispositivos e barramentos.
 *
 *     Ex: CPU, Mem�ria, PCIE (video), South Bridge, Super Io ...
 *     A Apresenta��o da sond�gem pode ser feita em outra ordem,
 *     melhor que seja a ordem alfab�tica.
 */
void systemShowDevicesInfo()
{
	
#ifdef KERNEL_VERBOSE	
	printf("sm-sys-system-systemShowDevicesInfo:\n");
#endif	
	
    //T�tulo.
	printf(" ## DEVICES ## \n");
    //...	
	
	//CPU Devices.
	printf(" ** CPU Info ** \n\n");
	show_cpu_intel_parameters();
	//...
	
	//Memory Devices.
	printf(" ** Memory Info ** \n\n");
	//...
	
	//PCIE Devices (Graphic cards)
	printf(" ** Graphic Card Info ** \n\n");
	//...
	
	//PCI Devices.
	printf(" ** PCI Devices ** \n\n");
	pciInfo();
	
	//Legacy ...
	printf(" ** Legacy Devices Info ** \n\n");
	//...
	
	//
	// More ?!
	//
	
done:
    printf("done\n");
	refresh_screen();
    return;
}; 
 

/*
 * systemCreateSystemMenuBar:
 *     Cria a barra de menu do sistema.
 *
 *  @todo: 
 *      Essas informa��es devem ser repassadas para "struct system_d".
 *      #bugbug: A parent window deve ser gui->main e n�o gui->screen.
 */  
void *systemCreateSystemMenuBar (){
	
	struct window_d * hwBar;                //Barra.
	//Left: (Software).
	struct window_d * hwItemSystem;         //Item (System).
	struct window_d * hwItemApplications;   //Item (Applications).  
	struct window_d * hwItemWindow;         //Item (Window).
	//Right: (Hardware)
	struct window_d * hwItemMore;    //Abre uma bandeija ou menu com mais op��es.(V).
	struct window_d * hwItemClock;   //rel�gio.	
	
	if(gui->screen == NULL){
		return NULL;
	};
	
	 //Cria a barra no topo da tela.
	if(gui->screen != NULL){
		hwBar = (void*) create_menubar(gui->screen);
	};
	
	//
	// Deve-se criar a janela que servir� de parent window 
	// para os popup menus que aparecer�o quando clicar no item.
	// Obs: N�o importa qual dos tres �tems foi clicado,	
	//      a janela popupmenu ser� a mesma, 
	//
	
	hwItemSystem = (void*) CreateWindow( 1, 0, VIEW_MINIMIZED, "SystemMenuBar", 
	                               0, 60, (800/4), (600-60), 
							       gui->screen, 0, 0, COLOR_GRAY ); 
	if( (void*) hwItemSystem == NULL ){
		printf("sm-sys-system-systemCreateSystemMenuBar: hwItemSystem");
		die();
	}else{
		
		hwItemSystem = (void *) hwItemSystem;
		hwItemApplications = (void *) hwItemSystem;
		hwItemWindow = (void *) hwItemSystem;
	    RegisterWindow(hwItemSystem);
		RegisterWindow(hwItemApplications);
		RegisterWindow(hwItemWindow);
		//...
	};	
	
	
//done:

	return (void *) hwBar;
}; 
 

/*
 * systemCheck3TierArchitecture:
 * Checa o status dos servi�os oferecidos nas 3 camadas.
 */
/* 
void systemCheck3TierArchitecture(){
	
    //return; //@todo: Nothing for now.
};
*/

/*
 ***************************************
 * systemSetupVersion:
 *     Setup version info.     
 */
void systemSetupVersion (){
	
	//Version.
    Version = (void *) malloc( sizeof(struct version_d) );
    
	if ( (void *) Version == NULL )
	{
        //#todo:
        //Isso deve ser considerado um erro fatal,
        //pois existem aplica��es que dependem da vers�o do sistema 
        //para funcionarem corretamente.. 		
	    panic("sm-sys-system-systemSetupVersion: Version");
          
	} else {
		
        Version->Major = SYSTEM_VERSION_MAJOR;
		Version->Minor = SYSTEM_VERSION_MINOR;
		Version->Revision = SYSTEM_VERSION_REVISION;
		
	};
	
	//VersionInfo.
    VersionInfo = (void *) malloc ( sizeof(struct version_info_d) );
	
    if ( (void*) VersionInfo == NULL )
	{	
        //#todo:
        //Isso deve ser considerado um erro fatal,
        //pois existem aplica��es que dependem da vers�o do sistema 
        //para funcionarem corretamente.. 	
	    panic("sm-sys-system-systemSetupVersion: VersionInfo");
		
	}else{
		
		if ( (void *) Version != NULL  ){
			
            //VersionInfo->version = (void *) Version;
            //...VersionInfo->string = (char*) ...;
            //... 			
        };
		//...
	};
	
	
	//
	// Colocando na estrutura System se ela for v�lida.
	//
	
	if( (void *) System != NULL )
	{
		if ( System->used == 1 && System->magic == 1234 ){
			System->version = (void *) Version;
			System->version_info = (void *) VersionInfo;
		}
		//Nothing
	};
	
	//More ?!
	
//done:
    return;
};


/*
 * system_dispatch_to_procedure:
 *     Escolhe um procedimento de acordo com um status.
 * O teclado chama essa fun��o, que envia a mensagem para 
 * o procedimento ativo.
 *
 * OBS: 
 * Por enquanto essa rotina s� pode chamar o procedimento de janela
 * do kernel em ring 0. Para procedimentos de janela em ring 3
 * as mensagens ser�o salvas em filas na estrutura dos processos.
 *
 * Argumentos:
 *     +long param 2.
 *     +long param 1.
 *     +msg.
 *     +hwnd da janela ativa.
 *
 * classe: ram/sm
 */
int system_dispatch_to_procedure( struct window_d *window, 
                                  int msg, 
								  unsigned long long1, 
								  unsigned long long2)
{	
	asm(" pushl %0" :: "r" ((unsigned long) long2)  : "%esp"); 
    asm(" pushl %0" :: "r" ((unsigned long) long1)  : "%esp"); 
    asm(" pushl %0" :: "r" ((unsigned long) msg)    : "%esp");        
    asm(" pushl %0" :: "r" ((unsigned long) window) : "%esp"); 	 
    asm(" call *_g_next_proc ");    //Chama o xProc.
    asm(" pop %eax \n ");
	asm(" pop %eax \n ");
	asm(" pop %eax \n ");
	asm(" pop %eax \n ");	
   
	/*
	 * Status: 
	 *     A mensagem j� foi enviada ao procedimento e processada por ele.
	 *     Ent�o avisamos que n�o h� mais mensagens.
	 *
     */
	g_nova_mensagem = 0;
	
done:	
	return (int) 0;
};   


/*
 * SystemMenu:
 * Cria o system menu para manipular a janela ativa. Control Menu.
 */
int SystemMenu()
{
    struct window_d *Current;    //Parent window.	
	struct window_d *hWindow;    //Menu window.
	
	//Tentativa de utilizar control menu n�o estando no modo gr�afico.
	if(VideoBlock.useGui != 1)
	{
		//@todo: Notificar erro via modo texto.
	    return (int) 1;   
	};
	
    //
	// Usar a estrutura da Client Area.
	// A �rea da tela menos a �rea da barra de tarefas, ou barra de menu.
	//
	
	//
	// ?? desktop ?? Uma estrutura de janela , dentro da estrutura gui.
	//
	
	if( (void*) gui->desktop == NULL )
	{
		gui->desktop = (void*) gui->screen;
		
		//No Screen.
		if( (void*) gui->desktop == NULL ){
			return (int) 1; //erro.
		};		
	};

	//Set current.
	if( (void*) gui->desktop != NULL ){
        Current = (void*) gui->desktop;		
	};
	
	//No Current.
	if( (void*) Current == NULL ){
        return (int) 1;    //erro.
	};
	
	//
    // Cria o System Menu.
	// Envia uma parent window via argumento.
	// Retorna a janela criada para o menu.
    //	
	
    hWindow = (void *) create_menu(Current,4,0,0,0);
	
	//Se houve falha na cria��o da janela.
	if( (void *) hWindow == NULL ){
	    printf("sm-sys-system-SystemMenu: hWindow\n");
		die();
	};
	
	//Se houve falha na cria��o do menu.
	if( (void *) hWindow->defaultMenu == NULL ){
	    return (int) 1;
	}else{
		
		//Inicializa o array de menu�tens.
	    initmenuArray( hWindow->defaultMenu, 4);

        //Cria os �tens.		
	    create_menu_item( hWindow->defaultMenu, "System menu item 1", 0);
        create_menu_item( hWindow->defaultMenu, "System menu item 2", 0);
	    create_menu_item( hWindow->defaultMenu, "System menu item 3", 0);
	    create_menu_item( hWindow->defaultMenu, "System menu item 4", 1);
		//...
	};
	//Nothing.
done: 
	SetProcedure( (unsigned long) &SystemMenuProcedure);	
	return (int) 0;
};


/*
 * SystemMenuProcedure:
 *     O procedimento do control menu principal.
 *     menu do sistema, manipula a janela ativa.
 *     ram /sm  
 */																
unsigned long SystemMenuProcedure( struct window_d *window, 
                                   int msg, 
								   unsigned long long1, 
								   unsigned long long2 ) 
{
    switch(msg)
	{
        case MSG_KEYDOWN:
            switch(long1)
            {
                case VK_ESCAPE:	
				   SetProcedure( (unsigned long) &system_procedure);						
				   break;
				   
                default:
				    //Nothing.
                    break; 
            };
        break;	
	
		case MSG_SYSKEYDOWN:                 
            switch(long1)	       
            {   
				//Restaturar janela	
				case VK_F1:
				    //#bugbug: Aqui n�o devemos criar a 
					//status bar apenas atualizar as strings.
					//StatusBar( gui->screen, "Status Bar", "F1");
                    break;
					
				//Minimizar janela
                case VK_F2:
				    //#bugbug: Aqui n�o devemos criar a 
					//status bar apenas atualizar as strings.				
				    //StatusBar( gui->screen, "Status Bar", "F2");
                    break;
					
				//Maximizar janela
                //case VK_F3:
                //    break;

				//fechar janela
                //case VK_F4:
                //    break;
					
			    //...
				
				default:
                    //Nothing.				
				    break;
		    };              
        break;
		
	    case MSG_SYSKEYUP:
           //Nothing.		
        break;
		
		default:
		    //Nothing.
		    break;
	};
//Done.
done:
	if(VideoBlock.useGui == 1){
	    refresh_screen();
	};
	return (unsigned long) 0;
};


/*
 *******************************************************
 * systemReboot:
 *     Interface de inicializa��o da parte de sistema para o processo de reboot.
 *     realiza rotinas de desligamento de sistema antes de chamar o reiniciamento de hardware.
 *     *IMPORTANTE: Um processo em user mode deve realizar as rotinas de desligamento.
 *     passar para o kernel somente depois de fechar todos os processos.
 *     Quando essa rotina checar os processos ver� que n�o h� mais nada pra fechar.
 *     se ainda tiver algum processo pra fechar, ent�o essa rotina fecha, sen�o termina a rotina. 
 */

void systemReboot (){
	
	//int i;
	//unsigned long left;
	//unsigned long top;
	//unsigned long width;
	//unsigned long height;		
	
	//struct process_d *P;
	//struct thread_d *T;
		
	//struct window_d *hWnd;
	//struct window_d *hWindow;	
	
	asm ("cli");
	
	//No graphics.	
    if ( VideoBlock.useGui != 1 ){
		hal_reboot();
	}
	
	//#importante:
	//suspendendo a rotina de reboot,
	//at� que todas funcionalidades gr�ficas estejam em ordem.
	//Na verdade deveria existir um aplicativo chamado reboot 
	//que inicialise essa rotina deixando par ao kernel apenas 
	//a parte de baixo n�vel.
	
	hal_reboot();
	
/*	
		
	//Parent window.
	if ( (void *) gui->main == NULL )
	{
	    hal_reboot ();
		
	}else{
	    
		left = gui->main->left;
	    top = gui->main->top;
	    
		width = gui->main->width;
	    height = gui->main->height;	
		
	    set_up_cursor( (left/8) , (top/8) );
		//...
	};
	
	// @todo: Usar esquema de cores padr�o.
	
	if ( VideoBlock.useGui == 1 )
	{
		//Parent window.
	    if ( (void *) gui->main == NULL ){
	        hal_reboot();
	    }
			
	    //Create.
	    hWindow = (void *) CreateWindow( 3, 0, VIEW_MAXIMIZED, "systemReboot:", 
	                        left, top, width, height, 
			     			gui->main, 0, KERNEL_WINDOW_DEFAULT_CLIENTCOLOR, KERNEL_WINDOW_DEFAULT_BGCOLOR ); 

	    if ( (void *) hWindow == NULL ){
	        hal_reboot();
        }else{
		    RegisterWindow(hWindow);
			set_active_window(hWindow);
			SetFocus(hWindow);
	    };										
		
		
		// Auterando as margens.
		// Essas margens s�o usadas pela fun��o printf.
		// Obs: As medidas s�o feitas em n�meros de caracteres.
		// Obs: @todo: Devemos usar aqui o ret�ngulo da �rea de cliente,
		// e n�o as margens da janela.
		// A estrutura de janela deve nos oferecer os valores para a m�trica do 
		// ret�ngulo da �rea de cliente.
		// Obs: @todo:Acho que essa n�o � a forma correta de configurar isso. Uma 
		//rotina deveria perceber as dimens�es da janela de do caractere e determinar
		//as margens.
		
		g_cursor_left = (hWindow->left/8);
		g_cursor_top = (hWindow->top/8) + 4;   //Queremos o in�cio da �rea de clente.
		
		g_cursor_right = g_cursor_left + (width/8);
		g_cursor_bottom = g_cursor_top  + (height/8);
		
		//cursor (0, mas com margem nova).
		g_cursor_x = g_cursor_left; 
		g_cursor_y = g_cursor_top; 
        
	    //
	    //@todo: Criar a interface. realizando configura��es de sistema
	    // antes de chamar o hal.
	    // � apropriado deixar aqui as rotinas de desligamento do sistema
	    // ficando para hal a parte de hardware.
	    // +salvar arquivos de configura��o.
	    // +salvar discos virtuais.
	    // ...
	    //
    
     	//...
	
        //
        // Coloca uma tela preta pra come�ar.
        //
	
        //Cls and Message.
	    //backgroundDraw(COLOR_BACKGROUND);
	    //StatusBar( gui->screen, "StatusBar: ", "Rebooting...");	
        //refresh_screen();

	
	    //@todo:
	    //block current task.
	    //close tasks
	    //...

        //
        //@todo: 
	    // +criar uma variavel global que especifique o tipo de reboot.
        // +criar um switch para efetuar os tipos de reboot.
	    // +criar rota de fuga para reboot abortado.
	    // +Identificar o uso da gui antes de apagar a tela.
	    //  modo grafico ou modo texto.
	    //

	
	    //
	    // Video.
	    //
	
        set_up_cursor(0,4);	            //O cursor deveria ficar na �rea do cliente.	
        set_up_text_color(0x0f, 0x09);  //isso � para modo texto. eu acho??
	    
		//init message.
		sleep(8000);
        printf("\n REBOOT:\n");	
	    printf("The computer will restart in seconds\n");
	
	    //
	    // Scheduler stuffs.
	    //
	
	    sleep(8000);
	    printf("Locking Scheduler and taskswitch\n");
	    scheduler_lock();
	    taskswitch_lock();
	
	
	    //
	    // File System stuffs. @todo
	    //
	
	    //
	    // Fechando processos que n�o foram fechados pelo shutdown em user mode.
	    // Obs: Todos os processos nesse momento ja deveriam estar terminados
	    // atrav�s de rotinas em user mode. Ent�o checamos se falta algum e 
	    // caso haja, terminamos o processo.
	    // Obs: A rotina de fechar processo deve fechar as threads na ordem da lista encadeada.
	    //

	    sleep(8000);
	    printf("Terminating processes..\n");
		
		
		//
		// +Enviamos sinal para todas as threads bloquearem.
		// +desalocamos todos os recursos usados por elas.
		// +mandamos sinal para todas as thears fecharem.
		//
		
//blockingAllThreads:		
		//antes de terminarmos todos processos vamos boquear todas as threads.
	    //Come�a do 1 para n�o fechar o kernel.
	    i = 1;
	
	    while(i < THREAD_COUNT_MAX)
	    {
            //Pega da lista.
		    T = (void *) threadList[i];
		    if(T != NULL)
			{
#ifdef KERNEL_VERBOSE				
		        //bloqueia a thread. 
		        printf("blocking thread TID={%d} ...\n",i);
		        //refresh_screen();
#endif
                if( T->used == 1 ){
                    T->state = BLOCKED; //?? ZOMBIE ??   
		            //block_thread(i, 0);
				}
		    };
		    i++;
	    };

//exitingAllProcesses:	
	    //Come�a do 1 para n�o fechar o kernel.
	    i = 1;
	
	    while(i < PROCESS_COUNT_MAX)
	    {
            //Pega da lista.
		    P = (void *) processList[i];
		    if(P != NULL)
			{
#ifdef KERNEL_VERBOSE				
		        //Termina o processo. (>> TERMINATED)
		        printf("Killing Process PID %d\n", i);
		        //refresh_screen();
#endif
                if( P->used == 1 ){
		            exit_process(i, 0);
				}
		    };
		    i++;
	    };

	    // Final message.
	    sleep(8000);
	    printf("Rebooting..\n");
			
        SetFocus(hWindow);
	
		
		//voltando a margem normal a margem
		//?? isso n�o � necess�rio??
		g_cursor_left = (left/8);    //0;
		g_cursor_top = (top/8);        //0;
		
		g_cursor_right = (width/8);   
		g_cursor_bottom = (height/8);  
		
		//cursor (0, mas com margem nova)
		g_cursor_x = g_cursor_left; 
		g_cursor_y = g_cursor_top;
        //set_up_cursor(g_cursor_left,g_cursor_top); 		

        //#bugbug: Aqui n�o devemos criar uma status bar 
        //apenas atualizar as strings 		
		//StatusBar(hWindow,"Esc=EXIT","Enter=?");		
		//Nothing.
	};	   

 */
	
	//Nothing.
	
done:
	
    refresh_screen();
	
	sleep (8*8000);
	
	printf ("systemReboot: Rebooting ...");
	refresh_screen();
	
	KiReboot ();
    die ();
}


/*
 ****************************************
 * systemShutdown:
 *     Interface para shutdown.
 */ 

void systemShutdown (){
	 
	//@todo ...

	printf ("systemShutdown: It's safe to turnoff your computer");
	refresh_screen ();
	die ();
}


/*
 * systemShutdownViaAPM:
 *     Desliga a m�quina via APM.
 *     (Deve chamar uma rotina herdada do BM).
 */

void systemShutdownViaAPM (){

    // Obs: @todo:
	//     Existe uma rotina no BM que desliga a m�quina via APM usando 
	// recursos do BIOS. A rotina come�a em 32bit, assim podemos tentar herdar 
	// o ponteiro para a fun��o.
	
    //Chamar a fun��o de 32 bit herdado do BM.
    //todo: usar iret.
	
	// Check limits.
	// O ponteiro herdado tem que ser um valor dentro do endere�o onde 
	// roda o BM, que come�a em 0x8000.
	// if(shutdown_address > 0x8000 && shutdown_address < 0x20000 ){
		
	//Pilha para iret.
    //asm("pushl %0" :: "r" ((unsigned long) 8)     : "%esp");    //cs.
    //asm("pushl %0" :: "r" ((unsigned long) shutdown_address)    : "%esp");    //eip.
	//asm("iret \n");    //Fly!	
		
	//};
	
//hang:
	
    panic("sm-system-systemShutdownViaAPM:\n");
}


/*
 * systemGetSystemMetric:
 *     Retorna um valor de medida de algum elemento do sistema.
 * usado pelas rotinas de cria��o e an�lise.
 * O argumento seleciona a vari�vel global que ser� retornada.
 * Obs: Temos muitas vari�veis globais com valores de demens�es
 * de recurso do sistema. Essa rotina � um modo organizado de 
 * pegar os valores das variaveis globais relativas � medidas.
 * Obs: � muito apropriado essa fun��o ficar no arquivo \sm\sys\system.c
 * Pois � a parte mais importante do m�dulo System Manegement".
 */

void *systemGetSystemMetric (int number){
	
	if ( number <= 0 )
        return NULL;	
	
	switch ( number )
	{
		//case SM_NULL:
		//    return NULL;
		//	break;
			
		case SM_SCREENWIDTH:
            //pegar uma global
			break;
		
        case SM_SCREENHEIGHT:
            //pegar uma global
			break;
        
		//Essa � uma vers�o para o desenvolvedor??
        case SM_DEVELOPER_EDITION:
            if(gSystemEdition == SYSTEM_DEVELOPER_EDITION){
				return (void*) 1; //Sim, essa � uma vers�o para o desenvolvedor.
			}else{ return NULL; }; //N�o, essa n�o � uma vers�o para o desenvolvedor.
		    break;
			
            //Continuar ... 			
		 	
		default:
		    return NULL;
	};
};


/*
 ********************************************************************
 * systemGetSystemstatus:
 *     Retorna o valor de alguma vari�vel global relativa 
 * apenas a status de algum elemento do sistema.
 * Obs: � muito apropriado essa fun��o ficar no arquivo \sm\sys\system.c
 * Pois � a parte mais importante do m�dulo System Manegement".
 *
 */
void *systemGetSystemStatus (int number){
    
	if ( number <= 0 )
        return NULL;		
	
	
	switch ( number )
	{
		//case SS_NULL:
		//    return NULL;
		//	break;
			
		case SS_LOGGED:
            return (void *) g_logged;
			break;
		
        case SS_USING_GUI:
            return (void *) g_useGUI;
			break;
        //Continuar ... 			
		 	
		default:
		    return NULL;
	};
};



/*
 * ********************************************
 * die:
 *     Fun��o sem retorno. Aqui termina tudo.
 *      O sistema trava e n�o tem volta.
 * Final message!	
 * Refresh.	
 * HALT.
 */
void die (){
	
	//*Bullet.
    printf("sm-sys-system-die: * System Halted!\n");      
	
	if ( VideoBlock.useGui == 1 ){
	    refresh_screen ();
	}
	
//halt:
	asm ("hlt");   
	
	while (1){
		asm("cli");
	    asm("hlt");                        
	};     	
};


//o ID da janela que tem o terminal virtual ativo.
int systemGetTerminalWindow (){
	
    return (int) terminal_window;	
}


/*
 ****************************************************
 * systemSetTerminalWindow:
 * 
 * #importante
 * Configura qual vai ser a janela do terminal virtual.
 * Configurar as vari�veis de terminal presentes dentro da janela, pois cada 
 * janela pode ter um terminal. 
 *
 * #importante:
 * >>>> Essa e' a janela que o kernel precisa efetuar o refresh do stdout 
 * associado a ela. Dessa forma, as rotinas da libc precisa apenas escrever
 * em stdout, ficando por conta do kernel efetuar o refresh do arquivo 
 * para dentro da janela. 
 * >>>> Mas esse arquivo deve conter as caracteri'ticas de cada caractere. 
 * 
 */

void systemSetTerminalWindow ( struct window_d *window ){

	// Obs: ?? Como faremos para pintar dentro da janela do terminal.
	// Obs: a rotina de configura��o do terminal dever� ajustar as margens 
	// usadas pela rotina de impress�o de caracteres.
       
	// obs:
    // essa modifica��o de margens obriga o aplicativo a 
	// configurar o terminal somente na hora que for usa-lo,
	// para n�o correr o risco de imprimir no lugar errado.
		
//check_window:	

	if ( (void *) window == NULL )
	{
		return;
		//goto fail;
		
	} else {
		
		if ( window->used != 1 || window->magic != 1234 ){
			
			return;
			//goto fail;
		}
		
		
		// #bugbug
		// De que janela estamos falando, de qual dos terminais virtuais?
		// Pode ser da que esta em primeiro plano.
		// Configurando a vari�vel global que diz qual � 
		// o ID da janela que tem o terminal virtual ativo.
		
		terminal_window = (int) window->id;
		
		//configura o status do terminal dentro da janela
		//validade e reusabilidade das vari�veis de terminal 
		//dentro da estrutura de janela.
		
		window->terminal_used = 1;
		window->terminal_magic = 1234; 
		
		//tab
		//n�mero da tab.
		//indica qual tab o terminal est� usando.
		//@todo:
		// Criar uma forma de contar as tabs de terminal 
		// dentro do gerenciador de terminais.
		window->terminal_tab = 0; 
		
		
	    //rect
		//configura por �ltimo
	    window->teminal_left = 0;
	    window->teminal_top = 0;
	    window->teminal_width = 0;
	    window->teminal_height = 0;		
		//..
		
		//rcClient
		
		// test:
		// tentando ajustar as margens para as rotinas de impress�o.
		// para que as rotinas de impress�o imprimam dentro das 
		// dimens�es do terminal. 
		// obs: @todo: Essas margens dever�o ser reconfiguradas 
		// quando o terminal � fechado.
		
		// #bugbug:
        // #importante		
		// Esse ajuste pode significar problemas.
        
		if ( (void *) window->rcClient !=  NULL )
		{
		    //x inicial
		    if ( window->rcClient->left > 0 )
		    {
                g_cursor_x = (window->rcClient->left/8 );   
                g_cursor_left = g_cursor_x;
				
                window->teminal_left = window->rcClient->left; 				
		    }		
		
		    //y inicial
		    if ( window->rcClient->top > 0 )
		    {
                g_cursor_y = (window->rcClient->top/8 ); 
                g_cursor_top = g_cursor_y;
				
                window->teminal_top = window->rcClient->top; 				
            }		
        };
		
        if( window->rcClient->width > 0 )
		{
			window->teminal_width = window->rcClient->width; 
		}
	    
		if( window->rcClient->height > 0 )
	    {
			window->teminal_height = window->rcClient->height;
		}
	
	    if( window->rcClient->right > 0 )
		{
			// margem direita dada em linhas
			g_cursor_right = window->rcClient->right /8;
			
            window->teminal_right = window->rcClient->right; 			
        }
		
		if( window->rcClient->bottom > 0 )
		{
			// margem inferior dada em linhas
			g_cursor_bottom = window->rcClient->bottom /8;
			
		    window->teminal_bottom = window->rcClient->bottom;   
		}
		
		//limits
		//@todo: corrigir.
		// ajustes tempor�rios caso tenha havido um erro anteriormente...
		
		if( g_cursor_left > 800 ){
			g_cursor_left = 795;
		}

		if( g_cursor_top > 600 ){
			g_cursor_top = 595;
		}
		
		if( g_cursor_right > 800 ){
			g_cursor_right = 795;
		}

		if( g_cursor_bottom > 600 ){
			g_cursor_bottom = 595;
		}
		
        //Cursor.
		g_cursor_width = 8;  
        g_cursor_height = 8; 
		g_cursor_color = COLOR_TERMINALTEXT;		
		
	};
}


//@todo: precisamos de argumentos.
//configuramos o ret�ngulo do terminal virtual corrente.. 
void systemSetTerminalRectangle( unsigned long left,
                                 unsigned long top,
								 unsigned long width,
								 unsigned long height )
{
	//terminal_window
}




/*
 ***************************************
 * systemGetSystemMetrics:
 *     Retorna informa��es sobre o sistema.
 *     @todo: Criam um enum para essa fun��o, aqui mesmo nesse arquivo.
 */
unsigned long systemGetSystemMetrics ( int index )
{
	
	//print("#debug: systemGetSystemMetrics: i={%d} \n",index)
	
	if ( index <= 0 )
	    return (unsigned long) 0;	
	
	
	switch( index )
	{		
		//screen width.
		case 1:
		    return (unsigned long) screenGetWidth();
		    break;
			
		//screen height.	
		case 2:
		    return (unsigned long) screenGetHeight();
            break;		
			
		//cursor width.	
		case 3:
		    return (unsigned long) g_cursor_width;
            break;		

		//cursor hight.	
		case 4:
		    return (unsigned long) g_cursor_height;
            break;		
			
			
		//mouse pointer width.	
		case 5:
		    return (unsigned long) g_mousepointer_width;
            break;		
         
		//mouse pointer height. 
		case 6:
		    return (unsigned long) g_mousepointer_height;
            break;

		//char width.	
        case 7:
		    return (unsigned long) get_char_width();
            break;		

		//char height.	
        case 8:
		    return (unsigned long) get_char_height();
            break;		

		//...
		
		default:
		    goto done;
		    break;
	}
	
	
done:	
	return (unsigned long) 0;
};


//
//========================================================
//

//tentando mover fun��es de misc.c para c�.



/*
 * newLinkedlist:
 *     Cria uma nova linked list.
 */
void *newLinkedlist ()
{
    struct linkedlist_d *new_list; 
	
	new_list = (void *) malloc( sizeof(struct linkedlist_d) );
	
    if ( (void *) new_list == NULL ){
		return NULL;
	}

    /* put in the data  */
    //new_node->data  = new_data;
    
	new_list->head =  NULL;
    new_list->tail =  NULL;
	
//done:
    return (void *) new_list;
};



/*
 * newNode:
 *     Cria um novo nodo.
 */
void *newNode ()
{
    struct node_d *new_node; 
	
	new_node = (void *) malloc( sizeof(struct node_d) );
	
    if ( (void *) new_node == NULL ){
		return NULL;
	}
 
    /* put in the data  */
    //new_node->data  = new_data;
    
	new_node->flink = NULL;
 
//done:
    return (void *) new_node;
};


void Removing_from_the_beginning(struct linkedlist_d *list)
{
    struct node_d *old_head;
    struct node_d *new_head;
	
	if( (void*) list == NULL){
	   return;
	};
	
	old_head = list->head;
	new_head = old_head->flink;
	
	list->head = new_head; 
	
done:
	return;
};



void Removing_from_the_middle(struct linkedlist_d *list)
{
  //todo identifica~�ao do node que esta no meio,
};


void Removing_from_the_end (struct linkedlist_d *list){
	
    struct node_d *old_tail;
    struct node_d *new_tail;

	if ( (void *) list == NULL){
	   return;
	};

	old_tail = list->tail;
	new_tail = old_tail->blink;
	
	list->tail = new_tail; 
	
//done:
	//return;
};


/*
 *******************************************************************
 * systemStartUp:
 *     Rotina de inicializa��o do sistema.
 *
 * Fase 1:
 *     Inicia elementos independentes da arquitetura.
 *	   + Inicia v�deo, cursor, monitor. 
 *       Somente o necess�rio para ver mensagens.
 * Fase 2:
 *     Inicia elementos dependentes da arquiterura.
 *	   + inicia elementos de I/O e elementos conservadores de dados.
 *	   + inicia runtime.
 *     + inicia hal, microkernel, executive.
 *     + Inicia as tarefas. (@todo: threads ou processos ?).
 * Fase 3:
 *     classe system.device.unblocked.
 *	   @todo: Inicializar dispositivos LPC/super io.
 *            Keyboard, mouse, TPM, parallel port, serial port, FDC. 
 * Fase 4:
 *     classe system.device.unblocked.
 *     @todo: Dispositivos PCI, ACPI ...
 *
 *     Continua ...
 *
 * 2015 - Created.
 * 2016 - Revis�o.
 */

int systemStartUp (){
	
    int Status = 0;
	
	debug_print("systemStartUp:\n");

	KeInitPhase = 0;  //Set Kernel phase.    

    //
	// Antes de tudo: CLI, Video, runtime.
	//
	
	//   ## BUGBUG ##
	// As mensagens do abort podem n�o funcionarem nesse caso.
	// AINDA N�O INICIALIZAMOS O RECURSO DE MENSAGENS.
	
	if ( KeInitPhase != 0 )
	{		
		KiAbort ();	
		
	}else{
		
	    //Disable interrupts, lock taskswitch and scheduler.
	    
		asm("cli");	
	    taskswitch_lock();
	    scheduler_lock();
		
		//Set scheduler type. (Round Robin).
	    schedulerType = SCHEDULER_RR; 
		
	    
		//Obs: O video j� foi inicializado em main.c.
				
		//
		// BANNER !
		//
		
        //Welcome message. (Poderia ser um banner.) 
		set_up_cursor (0,1);
		
#ifdef EXECVE_VERBOSE		
        //#todo
		//printf("sm-sys-system-systemStartUp: Starting 32bit Kernel [%s]..\n",
		//    KERNEL_VERSION);
#endif		
		
#ifdef EXECVE_VERBOSE		
		//#todo
		//Avisar no caso de estarmos iniciando uma edi��o de desenvolvedor.
		//if (gSystemEdition == SYSTEM_DEVELOPER_EDITION){
		//    printf("sm-sys-system-systemStartUp: %s\n",developer_edition_string);
		//};
#endif

#ifdef EXECVE_VERBOSE
		printf("sm-sys-system-systemStartUp: LFB={%x} X={%d} Y={%d} BPP={%d}\n",
		    (unsigned long) SavedLFB,
			(unsigned long) SavedX,
			(unsigned long) SavedY,
			(unsigned long) SavedBPP );
#endif
	
        
        //
        // INIT ! 
        //  		
		
#ifdef EXECVE_VERBOSE
		//(inicializa as 4 fases.)
	    // B�sico. ( Vari�veis globais e estruturas ... ).
	    //printf("sm-sys-system-systemStartUp: Initializing Basics..\n");
#endif	
		
		
        Status = (int) init (); 
		
	    if ( Status != 0 )
		{
			debug_print("systemStartUp: init fail\n");
	        panic ("sm-sys-system-systemStartUp error: init\n");
	    }	
        //...	 
		
	}; //--else
	
	
	// System Version:
	//     Configurando a vers�o do sistema.
	systemSetupVersion();
	
	//
	// Inicializa a edi��o do sistema.
	// Define um tipo de destina��o para a vers�o do sistema operacional.
	//
	
    switch(SYSTEM_EDITION)
	{
		case SYSTEM_DEVELOPER_EDITION:
		    gSystemEdition = SYSTEM_DEVELOPER_EDITION; 
		    break;

		case SYSTEM_WORKSTATION_EDITION:
		    gSystemEdition = SYSTEM_WORKSTATION_EDITION;
		    break;

		case SYSTEM_SERVER_EDITION:
		    gSystemEdition = SYSTEM_SERVER_EDITION;
			break;

		case SYSTEM_IOT_EDITION:
		    gSystemEdition = SYSTEM_SERVER_EDITION;
			break;

        //...
        default:
		    gSystemEdition = 0;
            break; 		
	};
	
//
// Done: 
//     Completas as 3 fases de inicializa��o do sistema.
//     @todo: Na verdade ser�o mais fases..
//           as fases est�o em init().
done:	
    //printf("systemStartUp: Done!\n");	
	//refresh_screen();	
	if(KeInitPhase != 3){ 
	    Status = (int) 1; 
	};
    return (int) Status;
};


/*
 *******************************************
 * systemInit:
 *     Inicializando algumas vari�veis.
 */
int systemInit (){
	
	int Status;
	
	debug_print("systemInit:\n");
	
	//Colocando na vari�vel global, a op��o selecionada manualmente pelo 
	//desenvolvedor.
    gSystemEdition = SYSTEM_EDITION;
    //...
	
	// Podemos fazer algumas inicializa��es antes de chamarmos 
	//a rotina de start up.
	
	Status =  (int) systemStartUp ();	
	
	
#ifdef BREAKPOINT_TARGET_AFTER_SYSTEM
    //#debug 
	//a primeira mensagem s� aparece ap�s a inicializa��o da runtime.
	//por isso n�o deu pra limpar a tela antes.
	printf(">>>debug hang: after init");
	refresh_screen(); 
	while (1){
		asm ("hlt");
	}
#endif		
	
	//retornando para a rotina de entrypoint da arquitetura alvo.
	return (int) Status;
};


/*
 ************************************************************
 * systemSystem:
 *     Construtor.
 * N�o tem valor de retorno, tem o mesmo nome que a classe.
 * Uma chamada � um construtor criaria uma estrutura com seu nome e 
 * o construtor pode inicializar alguma vari�vel. */

void systemSystem (){
	
    gSystemStatus = 1;
}


//
// End.
//

