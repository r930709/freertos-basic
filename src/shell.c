#include "shell.h"
#include <stddef.h>
#include "clib.h"
#include <string.h>
#include "fio.h"
#include "filesystem.h"

#include "FreeRTOS.h"
#include "task.h"
#include "host.h"

#include "queue.h" //task communicate

typedef struct {
    const char *name;
    cmdfunc *fptr;
    const char *desc;
} cmdlist;

void ls_command(int, char **);
void man_command(int, char **);
void cat_command(int, char **);
void ps_command(int, char **);
void host_command(int, char **);
void help_command(int, char **);
void host_command(int, char **);
void mmtest_command(int, char **);
void test_command(int, char **);
void test2_command(int, char **); //fib, prime1 ,prime2
void _command(int, char **);
void pwd_command(int, char**);
void new_command(int, char**);
void task_add( void * pvParameters); //task, add two integer number
void queue_process( void * pvParameters); //task, process data from test2_job_queue
double mySqrt(double); //mysqrt()


#define MKCL(n, d) {.name=#n, .fptr=n ## _command, .desc=d}
char pwd[20] = "/romfs/"; //current directory
xQueueHandle test2_job_queue = NULL; //test2 store data in queue temporarily
xQueueHandle queue_data_work = NULL; //get queue data from test2_job_queue,and process them
xTaskHandle test2_job_handle = NULL; //test2 task handle



cmdlist cl[]= {
    MKCL(ls, "List directory"),
    MKCL(man, "Show the manual of the command"),
    MKCL(cat, "Concatenate files and print on the stdout"),
    MKCL(ps, "Report a snapshot of the current processes"),
    MKCL(host, "Run command on host"),
    MKCL(mmtest, "heap memory allocation test"),
    MKCL(help, "help"),
    MKCL(test, "test new function"),
    MKCL(test2, "test fib , prime1 ,prime2 "),
    MKCL(pwd, "print working directory"),
    MKCL(new, "add two integer"),
    MKCL(, ""),

};

int parse_command(char *str, char *argv[])
{
    int b_quote=0, b_dbquote=0;
    int i;
    int count=0, p=0;
    for(i=0; str[i]; ++i) {
        if(str[i]=='\'')
            ++b_quote;
        if(str[i]=='"')
            ++b_dbquote;
        if(str[i]==' '&&b_quote%2==0&&b_dbquote%2==0) {
            str[i]='\0';
            argv[count++]=&str[p];
            p=i+1;
        }
    }
    /* last one */
    argv[count++]=&str[p];

    return count;
}
void ls_command(int n, char *argv[])
{

    fio_printf(1,"\r\n");
    int dir;

    if(n == 1) {

        dir = fs_opendir(pwd);

    } else if(n == 2) {

        dir = fs_opendir(argv[1]);
        if(dir > 0) {
            fio_printf(1,"\r\n");
        }

    } else {
        fio_printf(1," Too many argument!\r\n");
        return;
    }

    (void)dir;  //Use dir

}

int filedump(const char *filename)
{
    char buf[128];

    int fd=fs_open(filename, 0, O_RDONLY);

    if( fd == -2 || fd == -1)
        return fd;

    fio_printf(1, "\r\n");

    int count;
    while((count=fio_read(fd, buf, sizeof(buf)))>0) {
        fio_write(1, buf, count);
    }

    fio_printf(1, "\r");

    fio_close(fd);
    return 1;
}

void ps_command(int n, char *argv[])
{
    signed char buf[1024];
    vTaskList(buf);
    fio_printf(1, "\n\rName          State   Priority  Stack  Num\n\r");
    fio_printf(1, "*******************************************\n\r");
    fio_printf(1, "%s\r\n", buf + 2);
}

void cat_command(int n, char *argv[])
{
    if(n==1) {
        fio_printf(2, "\r\nUsage: cat <filename>\r\n");
        return;
    }

    int dump_status = filedump(argv[1]);
    if(dump_status == -1) {
        fio_printf(2, "\r\n%s : no such file or directory.\r\n", argv[1]);
    } else if(dump_status == -2) {
        fio_printf(2, "\r\nFile system not registered.\r\n", argv[1]);
    }
}

void man_command(int n, char *argv[])
{
    if(n==1) {
        fio_printf(2, "\r\nUsage: man <command>\r\n");
        return;
    }

    char buf[128]="/romfs/manual/";
    strcat(buf, argv[1]);

    int dump_status = filedump(buf);
    if(dump_status < 0)
        fio_printf(2, "\r\nManual not available.\r\n");
}

void host_command(int n, char *argv[])
{
    int i, len = 0, rnt;
    char command[128] = {0};

    if(n>1) {
        for(i = 1; i < n; i++) {
            memcpy(&command[len], argv[i], strlen(argv[i]));
            len += (strlen(argv[i]) + 1);
            command[len - 1] = ' ';
        }
        command[len - 1] = '\0';
        rnt=host_action(SYS_SYSTEM, command);
        fio_printf(1, "\r\nfinish with exit code %d.\r\n", rnt);
    } else {
        fio_printf(2, "\r\nUsage: host 'command'\r\n");
    }
}

void help_command(int n,char *argv[])
{
    int i;
    fio_printf(1, "\r\n");
    for(i = 0; i < sizeof(cl)/sizeof(cl[0]) - 1; ++i) {
        fio_printf(1, "%s - %s\r\n", cl[i].name, cl[i].desc);
    }
}

//  add string to integer
int myStrtoInt( char *str)
{
    int result =0;

    while(*str !='\0') {	       //check whether a string tail

        result = result * 10 ; //become to  a ten digit or hundred digit
        int compare = *str - '0';    // 0 ascii code is 48 ,compare the adcii ,we can get the correct number
        result = result + compare ;
        ++str;
    }
    return(result);

}

void test_command(int n, char *argv[])
{


    int i;
    int sum = 0;
    int previous = -1;
    int result = 1;

    char string[512] = "Fibonacci";
    char string_result[128] = "";
    strcat(string,"(");
    strcat(string,argv[1]);
    strcat(string,")");
    strcat(string," result = ");

    fio_printf(1, "\r\n",string);
    for(i = 0; i <= myStrtoInt(argv[1]); i++) {
        sum = previous + result;
        previous = result;
        result = sum;
    }
    sprintf(string_result,"%d", sum);
    strcat(string,string_result);
    strcat(string,"\n");
    fio_printf(1, "Fibonacci(%d) result = %d\r\n",myStrtoInt(argv[1]),sum);
    /*---------------------------------------------------------------------------------------------------------------*/
    int handle;
    int error;


    handle = host_action(SYS_SYSTEM, "mkdir -p output");
    handle = host_action(SYS_SYSTEM, "touch output/syslog");

    handle = host_action(SYS_OPEN, "output/syslog", 8);
    if(handle == -1) {
        fio_printf(1, "Open file error!\n\r");
        return;
    }

    error = host_action(SYS_WRITE, handle, (void *)string, strlen(string));
    if(error != 0) {
        fio_printf(1, "Write file error! Remain %d bytes didn't write in the file.\n\r", error);
        host_action(SYS_CLOSE, handle);
        return;
    }

    host_action(SYS_CLOSE, handle);
}

void test2_command(int n, char *argv[])
{

    int judge;

    if(test2_job_handle == NULL) {
        xTaskCreate( queue_process,(signed portCHAR *)"test2",256,NULL, 0|portPRIVILEGE_BIT, &test2_job_handle);
    }
    if(test2_job_queue == NULL) {
        test2_job_queue = xQueueCreate( 10, sizeof(int));
    }

    if(n < 3 || n > 3 ) {
        fio_printf(1,"input error \r\n");
        return;
    } else if(n == 3) {

        int number = myStrtoInt(argv[2]);

        if(number == -1) {
            return;
        }

        if(strcmp(argv[1],"fib") == 0) {
            judge = 0;
        } else if(strcmp(argv[1],"prime1") == 0) {
            judge = 1;
        } else if(strcmp(argv[1],"prime2") == 0) {
            judge = 2;
        }
        if( xQueueSend( test2_job_queue, (void *) &judge , (portTickType) 5) != pdPASS) {
            fio_printf(1,"\r\nFailed to post the message to queue\r\n");
        }
        if( xQueueSend( test2_job_queue, (void *) &number , (portTickType) 5) != pdPASS) {
            fio_printf(1,"\r\nFailed to post the message to queue\r\n");
        }

    }

}

void queue_process(void * pvParameters)
{
    int judge_function,number;
    int sum;
    int previous;
    int result;

    uint32_t check_prime; //prime number parameters


    queue_data_work = xQueueCreate( 15,sizeof( int ));
    while(1) {
        if( xQueueReceive( test2_job_queue,&(judge_function) ,(portTickType) 10 ) == pdTRUE) {

            while( xQueueReceive( test2_job_queue,&(number), (portTickType) 10) != pdTRUE) {}

            int sqrt_result;
            int i,j;

            if(judge_function == 0) {  /*fibonacci implement*/
                previous = -1;
                result = 1;
                sum = 0;
                for(i=0 ; i<=number ; i++) {
                    sum = previous + result;
                    previous = result;
                    result = sum;
                }
            }
            if( xQueueSend( queue_data_work,( void * ) &judge_function, (portTickType) 5) != pdPASS) {
                fio_printf(1,"\r\nFailed to post the message to queue");
            }
            if( xQueueSend( queue_data_work,( void * ) &number, (portTickType) 5) != pdPASS) {
                fio_printf(1,"\r\nFailed to post the message to queue");
            }
            if( xQueueSend( queue_data_work,( void * ) &sum, (portTickType) 5) != pdPASS) {
                fio_printf(1,"\r\nFailed to post the message to queue");
            }

            if(judge_function == 1) { //check this number whether the prime number is

                check_prime = 0;
                sqrt_result = (int)(mySqrt((double)number));
                for(i=2 ; i <= sqrt_result ; i++) {
                    if(!(number%i)) {
                        check_prime = 1;
                        break;
                    }
                }
                if( xQueueSend( queue_data_work,( void * ) &judge_function, (portTickType) 5) != pdPASS) {
                    fio_printf(1,"\r\nFailed to post the message to queue");
                }
                if( xQueueSend( queue_data_work,( void * ) &number, (portTickType) 5) != pdPASS) {
                    fio_printf(1,"\r\nFailed to post the message to queue");
                }
                if( xQueueSend( queue_data_work,( void * ) &check_prime, (portTickType) 5) != pdPASS) {
                    fio_printf(1,"\r\nFailed to post the message to queue");
                }
            }

            if(judge_function == 2) {
                int count = 0;
                check_prime = 0;
                j = 1;

                while(count < number) {

                    j++;
                    sqrt_result = (int)(mySqrt((double)j));

                    for(i=2 ; i<=sqrt_result ; i++) {
                        if(!(j%i)) {
                            check_prime = 1;
                            break;
                        }

                    }
                    if(!check_prime) {
                        count++;
                    }
                    check_prime = 0;
                }
                if( xQueueSend( queue_data_work,( void * ) &judge_function, (portTickType) 5) != pdPASS) {
                    fio_printf(1,"\r\nFailed to post the message to queue");
                }
                if( xQueueSend( queue_data_work,( void * ) &number, (portTickType) 5) != pdPASS) {
                    fio_printf(1,"\r\nFailed to post the message to queue");
                }
                if( xQueueSend( queue_data_work,( void * ) &j, (portTickType) 5) != pdPASS) {
                    fio_printf(1,"\r\nFailed to post the message to queue");
                }

            }

        }
    }

}

double mySqrt(double x)
{
    double diff = 0.000001;
    double low, high, mid;

    if( 0 < x && x < 1 ) {
        low = 0;
        high = 1;
    } else {
        low = 1;
        high = x;
    }

    while(high - low > diff) {

        mid = low + (high-low)/2 ; // prevent the overflow situation
        if((mid*mid) > x) {
            high = mid;
        } else {
            low = mid;
        }
    }
    return( low + diff);
}

void *get_queue_handle(void)
{
    return &queue_data_work;
}



void _command(int n, char *argv[])
{
    (void)n;
    (void)argv;
    fio_printf(1, "\r\n");
}

void pwd_command(int n,char *argv[])
{

    if(n==1) {
        fio_printf(1, "\r\n");
        fio_printf(1, pwd);
        fio_printf(1, "\r\n");
    } else fio_printf(1, "Too many argument!\r\n");
}


int input[5] = {0,0,0,0,0};
xTaskHandle xHandle = NULL;
void new_command(int n,char *argv[])
{

    if(n!=3) {
        fio_printf(1,"\rinput error!\r\n");
        return;

    }
    input[0] = myStrtoInt(argv[1]);
    input[1] = myStrtoInt(argv[2]);

    xTaskCreate(task_add,(signed portCHAR *)"task",256,input,tskIDLE_PRIORITY ,&xHandle);
    fio_printf(1,"\r\n");
}
void task_add(void *pvParameters)
{
    int sum = 0;
//`	char hint[] = USER_NAME "@" USER_NAME "-STM32:~$ ";
    sum = input[0]+input[1];
    fio_printf(1,"sum = %d\r\n",sum);

//	fio_printf(1,"%s",hint);

    vTaskSuspend(xHandle);


}

cmdfunc *do_command(const char *cmd)
{

    int i;

    for(i=0; i<sizeof(cl)/sizeof(cl[0]); ++i) {
        if(strcmp(cl[i].name, cmd)==0)   //if match will return function pointer
            return cl[i].fptr;
    }
    return NULL;
}
