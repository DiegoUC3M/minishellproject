//  MSH main file
// Write your msh source code here

//#include "parser.h"
#include <stddef.h>         /* NULL */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_COMMANDS 8


// ficheros por si hay redirección
char filev[3][64];

//to store the execvp second parameter
char *argv_execvp[8];

void siginthandler(int param)
{
    printf("****  Saliendo del MSH **** \n");
    //signal(SIGINT, siginthandler);
        exit(0);
}

/**
 * Get the command with its parameters for execvp
 * Execute this instruction before run an execvp to obtain the complete command
 * @param argvv
 * @param num_command
 * @return
 */
void getCompleteCommand(char*** argvv, int num_command) {
    //reset first
    for(int j = 0; j < 8; j++)
        argv_execvp[j] = NULL;

    int i = 0;
    for ( i = 0; argvv[num_command][i] != NULL; i++)
        argv_execvp[i] = argvv[num_command][i];
}


/**
 * Main sheell  Loop  
 */
int main(int argc, char* argv[])
{
    /**** Do not delete this code.****/
    int end = 0; 
    int executed_cmd_lines = -1;
    char *cmd_line = NULL;
    char *cmd_lines[10];

    if (!isatty(STDIN_FILENO)) {
        cmd_line = (char*)malloc(100);
        while (scanf(" %[^\n]", cmd_line) != EOF){
            if(strlen(cmd_line) <= 0) return 0;
            cmd_lines[end] = (char*)malloc(strlen(cmd_line)+1);
            strcpy(cmd_lines[end], cmd_line);
            end++;
            fflush (stdin);
            fflush(stdout);
        }
    }

    /*********************************/

    char ***argvv = NULL;
    int num_commands;

    //es necesario poner la variable para acumular las sumas aqui, porque si la metemos dentro del while todo el rato se resetea la variable a 0 antes de sumar, y no es precisamente lo que queremos
    int acc=0;


    while (1) 
    {
        int status = 0;
        int command_counter = 0;
        int in_background = 0;
        signal(SIGINT, siginthandler);

        // Prompt 
        write(STDERR_FILENO, "MSH>>", strlen("MSH>>"));

        // Get command
        //********** DO NOT MODIFY THIS PART. IT DISTINGUISH BETWEEN NORMAL/CORRECTION MODE***************
        executed_cmd_lines++;
        if( end != 0 && executed_cmd_lines < end) {
            command_counter = read_command_correction(&argvv, filev, &in_background, cmd_lines[executed_cmd_lines]);
        }
        else if( end != 0 && executed_cmd_lines == end){
            return 0;
        }
        else{
            command_counter = read_command(&argvv, filev, &in_background); //NORMAL MODE
        }
        //************************************************************************************************


  /************************ STUDENTS CODE ********************************/

        

        //tenemos que crear n-1 pipes, siendo n procesos. Como el numero de procesos coincidira con el valor de la variable num_commands, hacemos uso de dicha variable
        int fd[num_commands-1][2];
        pid_t pid;
        int ret, fd_in, fd_out, fd_err_out; //estos seran los descriptores de fichero asociados a las redirecciones de la entrada y salida estandar
        int check = 1;                      //variable utilizada como booleano para comprobar que las sintaxis de los comandos internos es la correcta
        int resultado, cociente, resto;     //variables donde se van a almacenar los datos calculados de mycalc


        //variables necesarias para el mandato interno mycp:
        int fd1,fd2;
        #define BUFFER_SIZE 1024
        char buffer[1024];
        

        //PRIMERO HACEMOS LA COMPROBACION DE SI LO QUE SE QUIERE EJECUTAR ES EL MANDATO INTERNO MYCALC MEDIANTE LA FUNCION strcmp.
        if (strcmp(argvv[0][0],"mycalc")==0) {

            //Si no existen alguno de los parametros necesarios se pone la variable check a 0 para que no se ejecute el mandato
            if (argvv[0][1]==NULL || argvv[0][2]==NULL || argvv[0][3]==NULL) {
                check=0;
            }

            //comprobamos que el segundo argumento es correcto. El segundo argumento debe ser un numero, por eso recorremos cada caracter del string mediante el bucle con la condicion j<strlen y vamos comprobando
            //si alguno de esos caracteres no es un digito mediante la funcion isdigit. Si alguno de ellos no es un digito se pone la variable check a 0, sale del bucle y no se ejecuta despues el mandato.

            int j=0;
            
            //con este if comprobamos si el numero es negativo. De ser asi, empieza el bucle para buscar los digitos en la posicion j=1 
            //(evitando asi que entre en la condicion y que esta se haga false al intentar comprobar que "-" sea un digito)
            if (argvv[0][1][0]=='-') {
                j=1;
            }

            
            while(check!=0 && j<strlen(argvv[0][1])) 
            {
                if (isdigit(argvv[0][1][j])==0) {
                    check=0;
                }
                j++;
            }

            //Comprobamos que el tercer argumento es correcto (tiene que ser obligatoriamente add o mod):

            if (check!=0 && strcmp(argvv[0][2],"mod")!=0 && strcmp(argvv[0][2],"add")!=0) {
                    check=0;
            }

            //Finalmente, comprobamos que el ultimo argumento que tiene que ser un numero, es correcto, tal y como lo haciamos con el segundo argumento que tambien tenia que ser un numero

            j=0;

            if (argvv[0][3][0]=='-') {
                j=1;
            }

            while(check!=0 && j<strlen(argvv[0][3])) 
            {
                if (isdigit(argvv[0][3][j])==0) {
                    check=0;
                }
                j++;
            }

            //SI ALGUNA DE LAS PRUEBAS PARA COMPROBAR LA SINTAXIS HA SALIDO MAL, SE PONE LA VARIABLE CHECK A 0 E IMPRIME EL SIGUIENTE ERROR POR PANTALLA:
            if (check==0) {
                dprintf(2,"[ERROR] La estructura del comando es mycalc <operando_1> <add/mod> <operando_2>\n");
            }


            //AHORA QUE HEMOS COMPROBADO QUE LA SINTAXIS ES CORRECTA,PROCEDEMOS A CODIFICAR EL MANADATO

            if (check==1) {

                //MEDIANTE UN STRCMP, COMPROBAMOS SI LO QUE QUEREMOS ES SUMAR O HACER EL MODULO. EN ESTE CASO VAMOS A HACER LA COMPROBACION DE QUE QUERAMOS SUMAR:
                if(strcmp(argvv[0][2],"add")==0) {
                    resultado=atoi(argvv[0][1])+atoi(argvv[0][3]);                                      //hacemos uso de la funcion atoi para transformar el string en un int y poder hacer la suma
                    acc+=resultado;                                                                     //vamos acumulando la suma en la variable acc
                    dprintf(2,"[OK] %s + %s = %d; Acc %d\n",argvv[0][1],argvv[0][3],resultado,acc);     //hacemos uso de la funcion dprintf, que te imprime el mensaje en el descriptor de fichero pasado por parametro 
                                                                                                                            // (en este caso como queremos imprimir en salida estandar pasamos como fd el 2)

                    /*
                    Para imprimir en la salida estandar de error, tambien se podria hacer de la siguiente manera:
                    
                    char error_output[50];

                    sprintf(error_output,"[OK] %s + %s = %d; Acc %d\n",argvv[0][1],argvv[0][3],resultado,acc);
                    write(2,error_output,strlen(error_output));
                    */
                }

                if(strcmp(argvv[0][2],"mod")==0) {
                    resto=atoi(argvv[0][1])%atoi(argvv[0][3]);
                    cociente=atoi(argvv[0][1])/atoi(argvv[0][3]);
                    dprintf(2,"[OK] %s %c %s = %d; Cociente %d\n",argvv[0][1],'%',argvv[0][3],resto,cociente);


                }
            
            }
        }




        check=1;
        if (strcmp(argvv[0][0],"mycp")==0) {

            //igual que en mycalc, si falta alguno de los argumentos ponemos el booleano check a 0
            if(argvv[0][1]==NULL || argvv[0][2]==NULL) {
                printf("[ERROR] La estructura del comando es mycp <fichero_origen><fichero_destino>\n");
                check=0;
            }


            if (check!=0) {

                //abrimos el fichero que corresponde con el primer argumento (el fichero origen)
                fd1=open(argvv[0][1],O_RDONLY);

                if (fd1<0) {
                    printf("[ERROR] Error al abrir el fichero origen\n");
                    check=0;
                }


                //abrimos el fichero que corresponde con el primer argumento (el fichero destino)
                fd2=open(argvv[0][2],O_WRONLY | O_TRUNC);

                if (fd2<0) {
                    printf("[ERROR] Error al abrir el fichero destino\n");
                    check=0;
                }
            }

            if (check!=0) {

                //vamos a leer del fichero origen y a escribir en el fichero destino en bucle hasta que la variable ret devuelva 0 bytes leidos
                do {
                    ret=read(fd1,buffer,BUFFER_SIZE);

                    if (ret<0) {
                        check=0;
                    }

                    ret=write(fd2,buffer,ret);

                    if (ret<0) {
                        check=0;
                    }

                } while (ret>0);



                //VAMOS A COMPROBAR QUE SE CIERRRAN BIEN LOS FICHEROS:

                ret=close(fd1);

                if (ret<0) {
                    check=0;
                }

                ret=close(fd2);

                if (ret<0) {
                    check=0;
                }

            }

            if (check!=0) {
                dprintf(2,"[OK] Copiado con exito el fichero %s a %s",argvv[0][1],argvv[0][2]);
            }
        }

        //AHORA COMENZAMOS CON EL DESARROLLO DE LA MINISHELL EN SI. PARA ELLO PRIMERO INDICAMOS QUE QUEREMOS EJECUTAR UN MANDATO SOLO SI NO SE TRATA DE UN MANDATO INTERNO:
        if (strcmp(argvv[0][0],"mycp")!=0 && strcmp(argvv[0][0],"mycalc")!=0) {



            



            //como vamos a hacer una implementacion no limitada a ningun numero de procesos ni de pipes, es necesario hacer un bucle desde i=0 que representa el primer proceso hasta el numero metido en la variable
            //command_counter que representa el numero total de procesos
            for (int i=0;i<command_counter;i++) {

                //por cada iteraccion del bucle necesitaremos crear un pipe que conecte con el siguiente proceso, hasta que llegemos al ultimo proceso dado que la salida de este sera la salida estandar
                if (i!=command_counter-1) {

                    ret=pipe(fd[i]);

                    if (ret<0) {
                        perror("Hubo un error al crear el pipe");

                    }

                }

                
                //en cada iteraccion del bucle el proceso padre ira creando un proceso nuevo que se sustituira despues con un exec con la imagen del proceso que queramos
                pid=fork();



                //Cuando es el proceso hijo, es decir el que queremos sustituir con un exec:

                if (pid==0) {


                    //PRIMERO HACEMOS LAS COMPROBACIONES PARA HACER LA REDIRECCION DE SALIDA ESTANDAR, lo hacemos en cada iteraccion del bucle para todos los procesos porque todos tienen que tener la salida estandar de
                    //error
                    if (strcmp(filev[2],"0")!=0) {
                   
                        //cerramos la salida estandar de error
                        ret=close(2);

                        if (ret<0) {
                            perror("no se ha cerrado correctamente el fichero de salida estándar de error\n");
                            exit(-1);
                        }
                        
                        //abrimos el descriptor de fichero asociado a la salida estandar de error, SI no existe se crea, y si existe se trunca mediante los flags O_CREAT y O_TRUNC
                        fd_err_out= open(filev[2], O_WRONLY | O_CREAT | O_TRUNC,0744);
                        
                        if (fd_err_out<0) {
                            perror("El fichero no se ha abierto o creado correctamente");
                            exit(-1);
                        }

                        //ret=dup2(fd_out,1);
                        ret=dup(fd_err_out);

                        if (ret==-1) {
                            perror("Ha habido un problema al duplicar el descriptor de fichero\n");
                            exit(-1);
                        }
                    

                    }

                    //LA COMPROBACION i==0 SIMPLEMENTE LA HACEMOS PARA LAS REDIRECCIONES DE ENTRADA. 
                    //DURANTE LA PRIMERA ITERACCION QUE CORRESPONDERÁ AL PRIMER PROCESO DE LA SECUENCIA, saltará dentro del if (i==0),
                    //y hara una comprobacion de si existe un fichero de entrada, para hacer dicha redireccion del fichero a este primer mandato (tal y como se indica en las diapositivas de la practica).
                    if (i==0) {

                        if (strcmp(filev[0],"0")!=0) {
                   
                            //cerramos la entrada estandar:
                            ret=close(0);

                            if (ret<0) {
                                perror("no se ha cerrado correctamente el fichero de entrada estándar\n");
                                exit(-1); 
                            }
                            
                            //Abrimos el fichero al que vamos a redirigir la entrada estandar:
                            fd_in= open(filev[0], O_RDONLY,0744);

                            if (fd_in<0) {
                                perror("El fichero no se ha abierto correctamente");
                                exit(-1);
                            }

                            //ret=dup2(fd_in,0);
                            ret=dup(fd_in);

                            if (ret==-1) {
                                perror("Ha habido un problema al duplicar el descriptor de fichero\n");
                                exit(-1);
                            }    

                        }

                    }

                    //si el proceso de la secuencia que queremos ejecutar no es el primer proceso, necesitara cerrar la entrada estandar y duplicar el descriptor de fichero del pipe asociado a la entrada.
                    //La entrada del pipe x-1 se corresponde con la entrada del proceso x
                    if(i!=0) {

                    close(0);
                    dup(fd[i-1][0]);
                    close(fd[i-1][0]);  //hacemos limpieza

                    }
                    //si el proceso de la secuencia que queremos ejecutar no es el ultimo proceso, necesitara cerrar la salida estandar y duplicar el descriptor de fichero del pipe asociado a la salida
                    //La salida del pipe x se corresponde con la salida del proceso x
                    if (i!=command_counter-1) {

                    close(1);
                    dup(fd[i][1]);

                    //hacemos limpieza:
                    close(fd[i][1]);
                    close(fd[i][0]);

                    }

                    //PARA EL ULTIMO PROCESO HACEMOS UNA COMPROBACION DE SI HAY REDIRECCION DE SALIDA. SI ES EL ULTIMO PROCESO:
                    if (i==command_counter-1) {

                        //Y SI HAY UN FICHERO ASOCIADO A LA REDIRECCION DE SALIDA:
                        if (strcmp(filev[1],"0")!=0) {
                   
                            //cerramos la salida estandar:
                            ret=close(STDOUT_FILENO);

                            if (ret<0) {
                                perror("no se ha cerrado correctamente el fichero de salida estándar\n");
                                exit(-1); 
                            }
                            
                            //abrimos el descriptor de fichero de redireccion de la salida
                            fd_out= open(filev[1], O_WRONLY | O_CREAT | O_TRUNC,0744);

                            if (fd_out<0) {
                                perror("El fichero no se ha abierto o creado correctamente");
                                exit(-1);
                            }

                            //ret=dup2(fd_out,1);
                            
                            //duplicamos el descriptor de fichero de redireccion de la salida
                            ret=dup(fd_out);

                            if (ret==-1) {
                                perror("Ha habido un problema al duplicar el descriptor de fichero\n");
                                exit(-1);
                            }



                        }
                        
                    
                        

                    }
               

                    //Por ultimo sustituimos la imagen con un exevp pasando como parametro los argumentos almacenados en argvv
                    execvp(argvv[i][0],argvv[i]);
                    perror("Error en el exec");
                    exit(-1);

                }


                //FINALMENTE HACEMOS LAS LIMPIEZAS DEL PROCESO PADRE:

                //si no es el primer proceso de la secuencoa, necesitaremos limpiar el descriptor del pipe asociado a la entrada 
                //(ya que ya habremos redirigido el mismo en el proceso hijo antes de ser sustituido por un exec)
                if (i!=0) {
                    close (fd[i-1][0]); 
                }
                //si no es el ultimo proceso de la secuencia, necesitaremos limpiar el descriptor del pipe asociado a la salida 
                //(ya que ya habremos redirigido el mismo en el proceso hijo antes de ser sustituido por un exec)
                if (i!=command_counter-1) {
                    close (fd[i][1]);    
                }
                
                
            }
            
            //esto es lo que nos permitira ejecutar o no en modo background. Si ejecutamos en modo background el padre no hara un wait y por tanto podremos ejecutar otros mandatos
            if (in_background==0) {

                while(pid!=wait(&status));
            
            }
            else {
                printf("[%d]\n",pid);
            }

        }


            
        
        
    }
    return 0;
}
