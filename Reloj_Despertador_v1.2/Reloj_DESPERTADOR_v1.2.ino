/*
#       _\|/_   A ver..., ¿que tenemos por aqui?
#       (O-O)        
# ---oOO-(_)-OOo---------------------------------
 
 
##########################################################
# ****************************************************** #
# *           DOMOTICA PARA PRINCIPIANTES              * #
# *               Reloj Despertador                    * #
# *          Autor:  Eulogio López Cayuela             * #
# *         Version v1.0   Fecha: 06/02/2016           * #
# *         Revisión 1.2   Fecha: 19/03/2020           * #
# ****************************************************** #
##########################################################
*/

#define __VERSION__ "Reloj Despertador v1.2\n"


/*
      ===== NOTAS DE LA VERSION ===== 
 - Sencillo reloj despertador con alarma programable y mantenida durante 10 minutos, (salvo interaccion del usuario)

      ===== OPCIONES DISPONIBLES ===== 
 - Programacion de alarma, activacion, desactivacion y parada de esta en funcionamiento. Encender luz para consultar la hora
 
  La pulsacion de cualquier tecla enciende la luz del display para facilitar la lectura.
  y no hace nada mas. Para cualquier accion de teclado se ha de pulsar una segunda vez 
  en este caso la tecla deseada. 
  Vease funcion de cada tecla:
  
  'A' --> Programar la alarma (solo si la luz esta encendida, de modo que un manotazo nocturno para consultar la hora
          impida que entremos en programacion).            
          El display mostrara en la parte derecha los caracteres 'ON' u 'OF' 
          segun el estado actual de la alarma. ¡Modificarla no significa activarla!.
          Para Activarla o desactivarla, debemos pulsar la tecla 'A' estando en modo 'Programacion Alarma', 
          esto conmutará el estado actual de la alarma. Debemos salir aceptando cambios pulsando 'D'. 
          Si no se aceptan los cambio explicitamente la ctivacion o desactivacion no será efectiva, 
          como tampoco lo serán los cambios en la hora de la alarma. 
          Al salir de la programacion se nos motrara una mensaje del estado en que ha quedado.

  'B' --> Programar la hora del reloj (solo si la luz esta encendida...)
          El display mostrara en la parte derecha los caracteres 'H?'

          ** Dentro de los modo de programacion 'A' y 'B' el digito objeto de modificacion 
             parpadeará con una cadencia de 750ms.Si se dejan de pulsar teclas durante 
             mas de 45 segundos, se abandona la programacion automaticamente 
             y se cancela cualquier cambio en progreso.
          
  'C' --> cancelar/anular una programacion de alarma u hora iniciadas.
  'D' --> Aceptar una programacion en curso, tanto del Reloj como de la Alarma o Apagar la luz (si esta encendida)
  '*' --> consultar hora de alarma programada 
  '1,2,3,4,5,6,7,8,9,0' --> digitos para programar tiempos.
  
  '*' y '#' actuan cono desplazamiento a izquierda y derecha respectivamente si estamos en modo programacion
 
   
   >> Version 1.0  Fecha: 06/02/2016 
      - Version inicial con todo lo que necesita para ser un buen despertador 
    
   >> Version 1.1  Fecha: 04/06/2016 
      - Añadida la opcion de luz de cortesia que ilumina el display al tocar una tecla 
        para facilitar la lectura de la hora durante la consultas nocturnas.
 
   >> Version 1.2  Fecha: 19/03/2020 
      - Aprovechando que he releido el codigo, modificada la operativa de programacion de la alarma 
        para hacerla mas sencilla.

   
   Tamaño actual compilado 8246 bytes de programa y 553 bytes de uso de RAM

*/


//------------------------------------------------------
//Importamos las librerias necesarias
//------------------------------------------------------
#include <Wire.h>                 //libreria para comunicaciones I2C
#include "RTClib.h"
#include <LiquidCrystal_I2C.h>    //Biblioteca para el control del LCD
#include <EEPROM.h>


//------------------------------------------------------
// Algunas definiciones personales para mi comodidad al escribir codigo
//------------------------------------------------------
#define AND &&
#define OR ||

//------------------------------------------------------
// Otras definiciones para pines y variables
//------------------------------------------------------

#define LCD_ADDR          0x3F    // Direccion I2C de nuestro LCD color verde
//#define LCD_ADDR        0x27    // los lcd de algunos fabricantes tienen esta otra Direccion I2C

#define PIN_ZUMBADOR         3    //patilla para el altavocillo
#define FRECUENCIA        2100    //frecuencia (Hz)del tono. Entre 2000 y 2100, muy cabrones y buenos para ALARMA
#define TIEMPO_SONIDO      450    //durarion del tono (milisegundos)
#define TIEMPO_SILENCIO    150    //durarion del silencio (milisegundos)
#define CICLOS               2    //numero de veces que se repite byte(1-255). con valor CERO no sonaria
#define LED_ONBOARD         13    //luz de corteria

//tono cabron,cabron: freuencia 2100, sonido 450, silencio 150, ciclos 2


//------------------------------------------------------
//DECLARACION DE CONSTANTES GLOBALES
//------------------------------------------------------

//   pinesTeclado[-] = {f,f,f,f,c,c, c, c};  //mapeado de filas y columnas
//   pinesTeclado[-] = {o,o,o,o,i,i, i, i};  //mapeado de entradas y salidas
byte pinesTeclado[8] = {4,5,6,7,8,9,10,11};  //pines arduino en los que esta conectado el teclado.


/* mapaeado de los caracteres del teclado tal como los ve el usuario */
char tecladoChar[4][4] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'},};


/* definicion de nuevos caracteres que seran bloques que nos ayuden a construir digitos de gran tamaño */
byte bloques[5][8] = {{B00000, B00000, B00000, B00000, B00000, B00000, B00000, B00000},
                      {B11111, B11111, B00000, B00000, B00000, B00000, B00000, B00000},
                      {B00000, B00000, B00000, B00000, B00000, B00000, B11111, B11111},
                      {B11111, B11111, B00000, B00000, B00000, B00000, B11111, B11111},
                      {B11111, B11111, B11111, B11111, B11111, B11111, B11111, B11111}};
                      
byte numeros[13][6] = {{4,1,4,4,2,4},  //0
                       {1,4,0,2,4,2},  //1
                       {3,3,4,4,2,2},  //2
                       {1,3,4,2,2,4},  //3
                       {4,2,4,0,0,4},  //4
                       {4,3,3,2,2,4},  //5
                       {4,3,3,4,2,4},  //6
                       {1,1,4,0,4,0},  //7
//                     {1,1,4,0,0,4},  //7 otra version del numero 7
                       {4,3,4,4,2,4},  //8
                       {4,3,4,2,2,4},  //9
                       {0,0,0,0,0,0},  //   caracter en 'blanco' (vacio)                        
                       {4,4,4,4,4,4},  //   caracter en 'negro'                       
                       {0,0,0,2,2,2}}; //   caracter en 'negro-medio'

unsigned long primeraTemporizacion;   //Para temporizar un rato de luz encendida al inicio
unsigned long tiempoActual;
unsigned long tiempoTeclaPulsada;

boolean FLAG_reinicio = true;
boolean FLAG_alarmaSonando = false;
boolean FLAG_alarmaActiva = true;
boolean FLAG_alarmaON  = true;

byte HORA_ALARMA = 8;
byte MINUTO_ALARMA = 30;
byte DURACION_ALARMA = 10;          //minutos que dura la alarma
#define MARCA_VALIDEZ           128  //si se graba informacion de una alarma, tambien se graba este dato
                                    //asi al recuperar datos de alarma verificamos que sean correctos      

//------------------------------------------------------
// Creamos las instancia de los objetos:
//------------------------------------------------------

//Creamos el objeto 'lcd' como una instancia del tipo "LiquidCrystal_I2C"
LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);
//creamos el objeto reloj
RTC_DS1307 rtc;



/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
   ****************************************************************************************************** 
                                    FUNCION DE CONFIGURACION
   ****************************************************************************************************** 
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm*/

void setup()  
{
  //Serial.begin(9600);  //Se inicia el puerto serie para depuracion
  //Serial.println(F(__VERSION__));
  
  pinMode(PIN_ZUMBADOR, OUTPUT);
  digitalWrite(PIN_ZUMBADOR, LOW);

  lcd.begin();                      //Inicializar el display
  rtc.begin();
  pinMode(LED_ONBOARD, OUTPUT);
  digitalWrite(LED_ONBOARD, LOW);
  
  /* Mensaje inicial para demostrar que el LCD esta bien conectado y comunicado */
  lcd.clear();                      //Reset del display 
  lcd.setBacklight(true);           //Activamos la retroiluminacion
  configurarTeclado(pinesTeclado);
  lcd.print("SISTEMA LISTO");  
  delay(1000);
  lcd.clear();                      //Reset del display

  /* cargamos en la memoria del lcd los caracteres personalizados que hemos diseñado */ 
  lcd.createChar(0, bloques[0]);  
  lcd.createChar(1, bloques[1]);
  lcd.createChar(2, bloques[2]);
  lcd.createChar(3, bloques[3]);
  lcd.createChar(4, bloques[4]);
  
  /* comprobamos que la eeprom contenga datos validos en cuyo caso usamos esos datos como hora de alarma */
  if (MARCA_VALIDEZ == leerEEPROM(2)){
    HORA_ALARMA = leerEEPROM(0);          //cargamos desde eeprom la hora de la alarma
    MINUTO_ALARMA = leerEEPROM(1);        //cargamos desde eeprom el minuto de la alarma 
  }
  /* comprobamos en la eeprom el estado de la alarma ON/OFF*/
  FLAG_alarmaON = leerEEPROM(3);
  mostarAlarmaProgramada(HORA_ALARMA, MINUTO_ALARMA);
  if(FLAG_alarmaON){
    delay(3000);
  }
  
  primeraTemporizacion = millis(); //Para dejar un rato la luz encendida al reinicio
}



/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
   ****************************************************************************************************** 
                                  BUCLE PRINCIPAL DEL PROGRAMA
   ****************************************************************************************************** 
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm*/

void loop()
{ 
  byte segundos;
  byte minutos;
  byte horas;

  char pulsacion = leerTeclado();   //buscamos pulsaciones en el teclado


  if(pulsacion != ' '){             //si hay pulsacion, vemos que hacer con ella
    tiempoTeclaPulsada = millis();  //'anotamos' el momento de la pulsacion para mantener la luz encendida unos segundos
    /* si estamos en reposo, luz apagada, simplemente la encendemos */
    if(lcd.getBacklight()==false){
      pulsacion = ' ';
      lcd.backlight(); 
    }
    /* si la alarma esta sonando, la desactivamos */
    if (FLAG_alarmaSonando == true){
      FLAG_alarmaSonando = false;
      FLAG_alarmaActiva = false;
      pulsacion = ' ';  //Dado que esta puede ser la segunda pulsacion, la eliminamos pra no entrar en zonas peligrosas)
      lcd.noBacklight();  //y nos aseguramos de apagar el display
      //delay (200);
    }

    delay(200);                     //Pausa apra estabilizar la pulsacion       
       
    //----------------------- 
    if(pulsacion == 'D'){
      lcd.noBacklight();  //apagamos la luz de display
    }
    //----------------------- 
    if (pulsacion == 'A' AND lcd.getBacklight()==true){ //CAMBIAR HORA DE LA ALARMA si ya esta la luz encendida
      modificarHoraIndicada(HORA_ALARMA, MINUTO_ALARMA, true); // 
      mostarAlarmaProgramada(HORA_ALARMA, MINUTO_ALARMA);   
      if(FLAG_alarmaON == true){
        delay(3000);
      }
    }
    //----------------------- 
    if (pulsacion == 'B'){ // CAMBIAR HORA DEL RELOJ
      DateTime now = rtc.now();
      byte horaReloj  = now.hour();
      byte minutoReloj  = now.minute();    
      modificarHoraIndicada(horaReloj, minutoReloj, false);  //modifica la hora del sistema
    }
    //----------------------- 
    if (pulsacion == '*'){ // MOSTRAR HORA DE LA ALARMA
      mostarAlarmaProgramada(HORA_ALARMA, MINUTO_ALARMA);
      if(FLAG_alarmaON){
        delay(4000); //pausa mostrando la hora programada para la alarma
      }
    }
  }    

  tiempoActual = millis();
  if (FLAG_reinicio == false AND lcd.getBacklight() == true AND tiempoActual > (tiempoTeclaPulsada + 10000)){
    lcd.noBacklight();
  }  
  if (FLAG_reinicio == true AND tiempoActual > (primeraTemporizacion +10000)){
    lcd.noBacklight();
    FLAG_reinicio = false;
  }
  DateTime now = rtc.now();
  minutos = now.minute();
  horas = now.hour();
  segundos = now.second();

  lcd.setCursor(15, 0); 
  lcd.print(segundos/10);    //mostrar 2 cifra de los segundos (mayor peso)
  lcd.setCursor(15, 1);
  lcd.print(segundos%10);    //mostrar 1 cifra de los segundos

  bigNumero(minutos%10, 12); //mostrar 1 cifra de los minutos 
  bigNumero(minutos/10, 8);  //mostrar 2 cifra de los minutos (mayor peso)

  bigNumero(horas%10, 4);    //mostrar 1 cifra de las horas 
  bigNumero(horas/10, 0);    //mostrar 2 cifra de las horas (mayor peso) 
  


  /* parte 1 del parpadeo de los (:) de separacion entre horas y minutos */ 
  if(tiempoActual%1000 < 500){
    lcd.setCursor(7, 0);
    lcd.write(165);
    lcd.setCursor(7, 1);  
    lcd.write(165); 
    /* parte 1 del parpadeo de la luz del lcd cuando suena la alarma */
    if (FLAG_alarmaSonando == true){
      lcd.backlight();
    }
  }
  /* parte 2 del parpadeo de los (:) de separacion entre horas y minutos */
  if(tiempoActual%1000 >= 500){
    lcd.setCursor(7, 0);
    lcd.write(32);
    lcd.setCursor(7, 1);  
    lcd.write(32);
    /* parte 2 del parpadeo de la luz del lcd cuando suena la alarma */
    if (FLAG_alarmaSonando == true){ 
      lcd.noBacklight();
    }
  }  

  /* comprobacion de si ha llegado la hora de hacer sonar la alarma */ 
  if (FLAG_alarmaON == true AND FLAG_alarmaActiva == true AND horas == HORA_ALARMA  
                              AND minutos >= MINUTO_ALARMA AND minutos < MINUTO_ALARMA + DURACION_ALARMA){
    activarAlarma(PIN_ZUMBADOR, FRECUENCIA, TIEMPO_SONIDO, TIEMPO_SILENCIO, CICLOS);
    FLAG_alarmaSonando = true;
  }
  /* comprobacion de si ha terminado la hora de hacer sonar la alarma */
  if ((horas != HORA_ALARMA OR minutos > MINUTO_ALARMA + DURACION_ALARMA)){
    FLAG_alarmaSonando = false;
    FLAG_alarmaActiva = true;
  }
}



/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
   ###################################################################################################### 
        BLOQUE DE FUNCIONES: LECTURAS DE SENSORES, COMUNICACION SERIE, INTERRUPCIONES...
   ###################################################################################################### 
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm*/

/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
//    DISPLAY
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm*/

//========================================================
//   Funcion para crear un DIGITO GRANDE
//========================================================

void bigNumero(byte i, byte posicion)
{
  //el parametro i es el numero que queremos representar
  //que conincide con el indice de la lista en la que se guardan 
  //los caracteres personalizados que forman cada numero
  lcd.setCursor(posicion, 0);
  lcd.write(numeros[i][0]); lcd.write(numeros[i][1]); lcd.write(numeros[i][2]);
  lcd.setCursor(posicion, 1);
  lcd.write(numeros[i][3]); lcd.write(numeros[i][4]); lcd.write(numeros[i][5]);   
  return;
}


//========================================================
//  FUNCION PARA MOSTAR LA HORA PROGRAMADA PARA LA ALARMA
//========================================================

void mostarAlarmaProgramada(byte HORA, byte MINUTO)
{
  if (FLAG_alarmaON == false){ 
    lcd.setCursor(0, 0); 
    lcd.print(" OJO, ALARMA    ");
    lcd.setCursor(0, 1);
    lcd.print("   DESACTIVADA  ");
    delay (2500);
    lcd.clear();
    return;
  }

  
  /* mostar hora de la alarma */
  bigNumero(HORA %10, 4);    //mostrar unidades de las horas 
  bigNumero(HORA /10, 0);    //mostrar decenas de las horas 
  
  /* mostar los dos puntos de separacion entre horas y minutos */
  lcd.setCursor(7, 0);
  lcd.write(165);
  lcd.setCursor(7, 1);  
  lcd.write(165);
  /* mostar minutos de la alarma */
  bigNumero(MINUTO %10, 12); //mostrar unidades los minutos
  bigNumero(MINUTO /10, 8);  //mostrar decenas de los minutos

  /* mostar indicador de ALARMA  APAGADA  */ 
  if (FLAG_alarmaON == false){    
    lcd.setCursor(15, 0); 
    lcd.print("O");
    lcd.setCursor(15, 1);
    lcd.print("F");
  }
    
  /* mostar indicador de ALARMA */ 
  if (FLAG_alarmaON == true){ 
    lcd.setCursor(15, 0); 
    lcd.print("O");
    lcd.setCursor(15, 1);
    lcd.print("N");
  }
}



/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
//    ALTAVOZ
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm*/

//========================================================
//  FUNCION PARA ACTIVAR EL ZUMBADOR
//========================================================

void activarAlarma(byte pin_Zumbador, int frecuencia, int tiempoON, int tiempoOFF, byte repeticiones)
{
  for (byte i=0; i<repeticiones; i++){
    tone(pin_Zumbador, frecuencia);
    delay (tiempoON);
    noTone(pin_Zumbador);
    delay (tiempoOFF);
  }
} 



/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
//    PROGRAMAR HORA / ALARMA
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm*/

//========================================================
// PROGRAMAR HORA (SISTEMA/ALARMA) ****
//========================================================

void modificarHoraIndicada(byte HORA, byte MINUTO, boolean ALARMA)
{
  byte  listaHora[4] = {0,0,0,0};
  float tiempoEntrada = millis();
  
  /* separar las horas y minutos en cifras de 1 digito */
  listaHora[0] = HORA /10; 
  listaHora[1] = HORA %10;
  listaHora[2] = MINUTO /10;
  listaHora[3] = MINUTO %10;
  
  /* Reset del display */
  lcd.clear(); 
  
  /* imprimir los 4 caracteres en tamaño grande */
  for (int i=0; i<4; i++){
    bigNumero(listaHora[i], i*4);  //bigNumero(numero, posicion);
  }
  
  /* imprimir los dos puntos de separacion entre horas y minutos */
  lcd.setCursor(7, 0);
  lcd.write(165);
  lcd.setCursor(7, 1);  
  lcd.write(165);

  if (ALARMA == true){ 
    /* Si estamos modificando ALARMA, mostrar su estado actual */ 
    if (FLAG_alarmaON == true){ 
      lcd.setCursor(15, 0); 
      lcd.print("O");
      lcd.setCursor(15, 1);
      lcd.print("N");
    }
    else{
      lcd.setCursor(15, 0); 
      lcd.print("O");
      lcd.setCursor(15, 1);
      lcd.print("F");
    }
  }
  if (ALARMA == false){  
    /* mostar indicador de que estamos modificando HORA DEL SISTEMA */  
    lcd.setCursor(15, 0); 
    lcd.print("H");
    lcd.setCursor(15, 1);
    lcd.print("?");
  }

  boolean estado_alarma = FLAG_alarmaON;
  
  boolean modo_PROGRAMACION = true;     //variable que nos mantiene en el bucle de modificacion de hora
  int cursorPOSh = 0;                   //posicion del cursor de escritura para los digitos introducidos
  byte CIFRA = 100;                     //valor no valido (cifra ha de ser un numero del 0 al 9)

  /* bucle para atencion del teclado y representacion de cifras en pantalla */
  while (modo_PROGRAMACION == true){
    char lectura = leerTeclado();   //Acceso simplificado al teclado matricial
    if (lectura != ' '){
      tiempoEntrada = millis();
      CIFRA = convertirPulsacionEnNumero(lectura);
      //delay(300);     //pequeño retraso apra evitar rebotes
    }

    if (CIFRA < 10){ //si se introduce un digito...
      //evitar que las horas sean mayor que 23 (parte 1)
      if (cursorPOSh == 0 AND CIFRA >=2){
        CIFRA = 2;
        if (listaHora[1]>3){ //autocorreccion de cifra para evitar horas mayor a 23
          listaHora[1]=3;
        }
      }
      /* evitar que las horas sean mayor que 23 (parte 2) */
      if (cursorPOSh == 1 AND listaHora[0] == 2 AND CIFRA >3){
        CIFRA = 3;
      } 
      /* evitar que los minutos sean mayor que 59 */
      if (cursorPOSh == 2 AND CIFRA >5){
        CIFRA = 5;
      } 
      /* actualizamos la cifra introducida y la mostramos en pantalla */
      listaHora[cursorPOSh] = CIFRA;     
      bigNumero(listaHora[cursorPOSh], cursorPOSh*4);
      CIFRA = 100;    // hacemos que cifra deje de ser dato correcto
      cursorPOSh+=1;  // y avanzamos el cursor
    }
    
    tiempoActual = millis();
    /* parte 1 del parpadeo del digito objetivo de posibles cambios */
    if(tiempoActual%1500 < 750){ 
      bigNumero(10, cursorPOSh*4); //borrar digito
    }
    /* parte 2 del parpadeo del digito objetivo de posibles cambios */
    if(tiempoActual%1500 >= 750){
      bigNumero(listaHora[cursorPOSh], cursorPOSh*4);  //reescribir el digito
    }


    /* control para salida si se excede un tiempo maximo inactivo */
    if (millis() > tiempoEntrada+45000){
      //si no se termino la edicion, dejamos las cosas como estaban
      modo_PROGRAMACION = false;
    }

    /* salimos cancelando voluntariamente la edicion */
    if (lectura ==  'C'){
      //si no cancela la edicion, dejamos las cosas como estaban
      modo_PROGRAMACION = false;
    }

    /* mover cursor y comprobar que esta en los limites permitidos */
    if (lectura == '*'){
      bigNumero(listaHora[cursorPOSh], cursorPOSh*4);  //reescribir el digito
      cursorPOSh += -1; //izquierda;     
    }
    if (cursorPOSh <= 0){
      cursorPOSh = 0; 
    } 

    if (lectura == '#'){
      bigNumero(listaHora[cursorPOSh], cursorPOSh*4);  //reescribir el digito
      cursorPOSh += 1; //derecha;
    } 
    if (cursorPOSh >= 3){
      cursorPOSh = 3; 
    }

    if (lectura == 'A' AND ALARMA == true){
      estado_alarma = !estado_alarma;
      if (estado_alarma == true){ 
        lcd.setCursor(15, 1);
        lcd.print("N");
      }
      else{
        lcd.setCursor(15, 1);
        lcd.print("F");
      }
      //modo_PROGRAMACION = false;
      //advertencia_estado_alarma();   
    }
  
    if (lectura == 'D'){ //salir del menu aceptando cambios
      modo_PROGRAMACION = false;
      /* comprobar la validez de los datos introducidos ...... */
      if (ALARMA == true){
        FLAG_alarmaON = estado_alarma;     
        HORA_ALARMA = listaHora[0]*10 + listaHora[1];
        MINUTO_ALARMA = listaHora[2]*10 + listaHora[3];
        grabarEEPROM(0, HORA_ALARMA);    //salvamos a eeprom la hora de la alarma
        grabarEEPROM(1, MINUTO_ALARMA);  //salvamos a eeprom el minuto de la alarma
        grabarEEPROM(2, MARCA_VALIDEZ);   //grabamos un dato reconocible como marca de validez de la eeprom
        if(FLAG_alarmaON){
          grabarEEPROM(3, 1);   //grabamos un dato reconocible como marca de alarma activa
        }
        else{
          grabarEEPROM(3, 0);
        }
        advertencia_estado_alarma();
      }
      if (ALARMA == false){
        grabarEEPROM(3, 0);   //grabamos un dato reconocible como marca de alarma inactiva
        DateTime now = rtc.now();
        byte hora = listaHora[0]*10 + listaHora[1];
        byte minuto = listaHora[2]*10 + listaHora[3]; 
        byte dia  = now.day();
        byte mes  = now.month();
        int anno = now.year(); 
        //poner el reloj RTC en hora/fecha (formato)
        //                  year, mes, dia, hora, min,    seg
        rtc.adjust(DateTime(anno, mes, dia, hora, minuto, 0));
      }
    }
    if (lectura != ' '){
      delay(300);     //pequeño retraso apra evitar rebotes
    }
  }
}


//========================================================
// AVISO DE ESTADO DE LA ALARMA
//========================================================

void advertencia_estado_alarma()
{
  lcd.clear();
  if (FLAG_alarmaON == false){
    lcd.setCursor(0, 0); 
    lcd.print("     ALARMA     ");
    lcd.setCursor(0, 1);
    lcd.print("   DESACTIVADA  ");
    delay (2000);      
  }
  if (FLAG_alarmaON == true){
    lcd.setCursor(0, 0); 
    lcd.print("ALARMA ACTIVADA ");
    lcd.setCursor(0, 1);
    lcd.print("REVISA PROGRAMA ");
    delay (2500);     
  }
  lcd.clear();  
}



/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
//    TECLADO
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm*/

//===================================================================================
// CONFIGURAR TECLADO 
//===================================================================================
byte configurarTeclado(byte pines[])
{
  for (int i=0; i<4; i++){          //filas del teclado
    pinMode(pines[i], OUTPUT);      //configurar el pin como salida pines[i]
    digitalWrite(pines[i], LOW);
  }
  for (int i=4; i<8; i++){          //columnas del teclado
    pinMode(pines[i], INPUT);       //configurar el pin como entrada
  }
} 


//===================================================================================
// Acceso Simplificado al Teclado(Es el usado en el programa) 
//===================================================================================
char leerTeclado()
{
  return (leerTecladoMatricial(pinesTeclado, tecladoChar));
}


//===================================================================================
// Acceso REAL al teclado 
//===================================================================================
char leerTecladoMatricial(byte pines[], char tecladoChar[4][4])
{
  byte lectura = 0;  //establecemos una variable para almacenar la lectura del teclado
  byte fila = 0;
  byte columna = 0;
  
  for (int f=0; f<4; f++){  //recorremos las filas del teclado escribiendo en ellas
    digitalWrite(pines[f], HIGH); //ponemos valor alto y comprobamos las salidas (columnas)
    for (int c=7; c>3; c--){ //recorremos las columnas para comprobar si hay tecla pulsada
      if (digitalRead(pines[c]) == 1){
        bitSet(lectura, f);
        bitSet(lectura, c);
        //guardamos por separado la fila y la columna en la que hay pulsacion
        fila = f+1;
        columna = c-3;
      }
    }
    digitalWrite(pines[f], LOW);
  }

  if (fila > 0 AND columna > 0){
    return(tecladoChar[fila-1][columna-1]);
  }
  return ' ';
} 


//===================================================================================
//  Convertir la tecla pulsada en un valor numerico  
//===================================================================================
int convertirPulsacionEnNumero(char pulsacion)
{
  char caracteres[16]={'0','1','2','3','4','5','6','7','8','9','A','B','C','D','*','#'};
  for (int i=0; 1<=15; i++){
    if (pulsacion == caracteres[i]){
      return(i);
    }
  }
}



/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
//    EEPROM
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm*/

//========================================================
// GRABAR DATOS EN LA EEPROM (direccion, valor)
//========================================================

void grabarEEPROM(int direccion, byte dato)
{
  EEPROM.write(direccion, dato); //grabar un dato en una direccion dada de la memoria EEPROM 
}


//========================================================
// LEER DATOS DE LA EEPROM  (direccion)
//========================================================
byte leerEEPROM(int direccion) //tambien lee la hora desde la eeprom
{
  byte dato   = EEPROM.read(direccion);  //recuperar dato desde la posicion indicada
  return dato;
}



//*******************************************************
//                    FIN DE PROGRAMA
//*******************************************************
