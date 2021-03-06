#define USE_STDPERIPH_DRIVER
#include "stm32f10x.h"
#include "stm32_p103.h"
/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <string.h>

/* Filesystem includes */
#include "filesystem.h"
#include "fio.h"
#include "romfs.h"

#include "clib.h"
#include "shell.h"
#include "host.h"

/* _sromfs symbol can be found in main.ld linker script
 * it contains file system structure of test_romfs directory
 */
extern const unsigned char _sromfs;

//static void setup_hardware();

volatile xSemaphoreHandle serial_tx_wait_sem = NULL;
/* Add for serial input */
volatile xQueueHandle serial_rx_queue = NULL;

/* IRQ handler to handle USART2 interruptss (both transmit and receive
 * interrupts). */
void USART2_IRQHandler()
{
	static signed portBASE_TYPE xHigherPriorityTaskWoken;

	/* If this interrupt is for a transmit... */
	if (USART_GetITStatus(USART2, USART_IT_TXE) != RESET) {
		/* "give" the serial_tx_wait_sem semaphore to notfiy processes
		 * that the buffer has a spot free for the next byte.
		 */
		xSemaphoreGiveFromISR(serial_tx_wait_sem, &xHigherPriorityTaskWoken);

		/* Diables the transmit interrupt. */
		USART_ITConfig(USART2, USART_IT_TXE, DISABLE);
		/* If this interrupt is for a receive... */
	}else if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET){
		char msg = USART_ReceiveData(USART2);

		/* If there is an error when queueing the received byte, freeze! */
		if(!xQueueSendToBackFromISR(serial_rx_queue, &msg, &xHigherPriorityTaskWoken))
			while(1);
	}
	else {
		/* Only transmit and receive interrupts should be enabled.
		 * If this is another type of interrupt, freeze.
		 */
		while(1);
	}

	if (xHigherPriorityTaskWoken) {
		taskYIELD();
	}
}

void send_byte(char ch)
{
	/* Wait until the RS232 port can receive another byte (this semaphore
	 * is "given" by the RS232 port interrupt when the buffer has room for
	 * another byte.
	 */
	while (!xSemaphoreTake(serial_tx_wait_sem, portMAX_DELAY));

	/* Send the byte and enable the transmit interrupt (it is disabled by
	 * the interrupt).
	 */
	USART_SendData(USART2, ch);
	USART_ITConfig(USART2, USART_IT_TXE, ENABLE);
}

char recv_byte()
{
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
	char msg;
	while(!xQueueReceive(serial_rx_queue, &msg, portMAX_DELAY));
	return msg;
}
void command_prompt(void *pvParameters)  //make sucess will jump a command interface 
{
	char buf[128];
	char *argv[20];
    char hint[] = USER_NAME "@" USER_NAME "-STM32:~$ ";
/*-----------data from the queue(queue_data_work) and print out the result---------------*/
	int func,in,out;
	xQueueHandle *print_queue;
	print_queue = get_queue_handle();
	
/*---------------------------------------------------------------------------------------*/
	
	fio_printf(1, "\rWelcome to FreeRTOS Shell\r\n");
	while(1){
                fio_printf(1, "%s", hint);
		fio_read(0, buf, 127);  //get command
	
		int n=parse_command(buf, argv);  //implement parse command in src/shell.c 

		/* will return pointer to the command function */
		cmdfunc *fptr=do_command(argv[0]);
		if(fptr!=NULL)
			fptr(n, argv);
		else
			fio_printf(2, "\r\n\"%s\" command not found.\r\n", argv[0]);
/*---------------------------------------------------------------------------------------------------------*/
		
	if(*print_queue != NULL){
		if( xQueueReceive( *print_queue, &(func), (portTickType)10) == pdTRUE){
			do{
				
			while(xQueueReceive( *print_queue, &(in), (portTickType) 10) != pdTRUE){}
				
			while(xQueueReceive( *print_queue, &(out), (portTickType) 10) != pdTRUE){}
			
			if(func == 0){ fio_printf(1,"work ok!!the fibonacci(%d) is %d \r\n",in,out);}
			else if(func == 1){
				
				if(out == 1){	fio_printf(1,"work ok!! %d is the composite number \r\n",in);}

				else{	fio_printf(1,"work ok!! %d is the prime number\r\n",in);}
							
			}

			else if(func == 2){ 
				fio_printf(1,"work ok!! the %dth prime number is %d\r\n",in,out);
			
			}	
			
			}while(xQueueReceive( *print_queue, &(func), (portTickType) 1) == pdTRUE);
		}	
	
	}

	}

}

void system_logger(void *pvParameters)
{
    signed char buf[128];
    char output[512] = {0};
    char *tag = "\nName          State   Priority  Stack  Num\n*******************************************\n";
    int handle, error;
    const portTickType xDelay = 100000 / 300;
    char hint[] = USER_NAME "@" USER_NAME "-STM32:~$ ";

   handle = host_action(SYS_OPEN, "output_ps/syslog_ps", 4);
    if(handle == -1) 
     {  fio_printf(1, "Open file error! and create new one \r\n");
      
       handle = host_action(SYS_SYSTEM, "mkdir -p output_ps");
       handle = host_action(SYS_SYSTEM, "touch output_ps/syslog_ps");
       handle = host_action(SYS_OPEN, "output_ps/syslog_ps",4);
	if(handle == -1)  
	{	fio_printf(1, "Create file error\r\n");
		return;
    	}
	else{	
		fio_printf(1, "Create file sucess\r\n");
		fio_printf(1, "%s",hint);
	    }
     }
	
	
    while(1) {
        memcpy(output, tag, strlen(tag));
        error = host_action(SYS_WRITE, handle, (void *)output, strlen(output));
        if(error != 0) {
            fio_printf(1, "Write file error! Remain %d bytes didn't write in the file.\n\r", error);
            host_action(SYS_CLOSE, handle);
            return;
        }
        vTaskList(buf);

        memcpy(output, (char *)(buf + 2), strlen((char *)buf) - 2);

        error = host_action(SYS_WRITE, handle, (void *)buf, strlen((char *)buf));
        if(error != 0) {
            fio_printf(1, "Write file error! Remain %d bytes didn't write in the file.\n\r", error);
            host_action(SYS_CLOSE, handle);
            return;
        }

        vTaskDelay(xDelay);
    }
    
    host_action(SYS_CLOSE, handle);
}

int main()
{
	init_rs232();                   //initilize and enable rs232
	enable_rs232_interrupts();
	enable_rs232();
	
	fs_init();   //filesystem memory set in src/filesystem.c  
	fio_init();  // file descriptor(0,1,2)
	
	register_romfs("romfs", &_sromfs);   //register filesystem
	
	/* Create the queue used by the serial task.  Messages for write to
	 * the RS232. */
	vSemaphoreCreateBinary(serial_tx_wait_sem);  
	/* Add for serial input 
	 * Reference: www.freertos.org/a00116.html */
	serial_rx_queue = xQueueCreate(1, sizeof(char));

    register_devfs();
	/* Create a task to output text read from romfs. */   //create a task ,command_prompt is a function point ,a task stack size  512 bytes 
	xTaskCreate(command_prompt,			      //task priority is 0 + 2 = 2	
	            (signed portCHAR *) "CLI",
	            512 /* stack size */, NULL, tskIDLE_PRIORITY + 2, NULL);


	/* Create a task to record system log. */
	xTaskCreate(system_logger,
	            (signed portCHAR *) "Logger",
	            1024 /* stack size */, NULL, tskIDLE_PRIORITY + 1, NULL);


	/* Start running the tasks. */
	vTaskStartScheduler();

	return 0;
}

void vApplicationTickHook()
{
}
