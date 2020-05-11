#include <Wire.h>
#include <RTClib.h>
#include <Adafruit_NeoPixel.h>

//////////////////////////////////////////////////////////////////////////////////////////////////////
// Chuflusku - Panel Horario
// Rubén Rodríguez Villa
// v2.0.0 - 11/05/2020 -> Se añade funcionalidad. En determinadas franjas horarias se iluminaran los leds para indicar el transcurso del tiempo
// v1.1.3 - 10/05/2020 -> Ordenar y simplifcar código.
// V1.1.2 - 04/05/2020 -> Ajustes en comentarios. Corrección Domingo.
// V1.1.1 - 28/04/2020 -> Al entrar por primera vez reproduce el mp3 001**.mp3 (Archivo asociado al dia de la semana que corresponda)
// V1.1.0 - 06/12/2019
// V1.0.0 - 05/12/2019
//
//  Repositorio:
//    - https://github.com/RodriguezVilla/Chuflusku-PanelHorario
//
//  Funciones:
//    - Cada pulsador reproduce sonidos distinto que varia en función del día de la semana que sea.
//    - Una tira de leds indicara el día de la semana que es.
//    - Un detector de movimiento activa tira de leds al percibir movimiento.
//    - En determinadas franjas horarias se iran encendiendo y apagando leds para indicar el transcurso del tiempo.
//
//  Consideraciones Previas:
//    - Si utiliza un modulo reloj en el que no estan fijada la fecha y hora,
//      ver instruccciones de línea 192.
//
//////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Para la gestión de la reproducción de audio utilizamos codigo de  http://www.dx.com/p/uart-control-serial-mp3-music-player-module-for-arduino-avr-arm-pic-blue-silver-342439#.VfHyobPh5z0
// Demo for the Serial MP3 Player Catalex (YX5300 chip)
// Hardware: Serial MP3 Player *1
// Fuente base MP3 Base player:  https://joanruedapauweb.com/blog/index.php/2017/02/07/arduino-serial-mp3-player-yx5300-es/
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define mp3 Serial3    // Connect the MP3 Serial Player to the Arduino MEGA Serial3 (14 TX3 -> RX, 15 RX3 -> TX)

static int8_t Send_buf[8] = {0}; // Buffer for Send commands.  // BETTER LOCALLY
static uint8_t ansbuf[10] = {0}; // Buffer for the answers.    // BETTER LOCALLY

String mp3Answer;           // Answer from the MP3.

boolean autoResume = true;

/************ Command byte **************************/
#define CMD_NEXT_SONG     0X01  // Play next song.
#define CMD_PREV_SONG     0X02  // Play previous song.
#define CMD_PLAY_W_INDEX  0X03
#define CMD_VOLUME_UP     0X04
#define CMD_VOLUME_DOWN   0X05
#define CMD_SET_VOLUME    0X06

#define CMD_SNG_CYCL_PLAY 0X08  // Single Cycle Play.
#define CMD_SEL_DEV       0X09
#define CMD_SLEEP_MODE    0X0A
#define CMD_WAKE_UP       0X0B
#define CMD_RESET         0X0C
#define CMD_PLAY          0X0D
#define CMD_PAUSE         0X0E
#define CMD_PLAY_FOLDER_FILE 0X0F

#define CMD_STOP_PLAY     0X16
#define CMD_FOLDER_CYCLE  0X17
#define CMD_SHUFFLE_PLAY  0X18 //
#define CMD_SET_SNGL_CYCL 0X19 // Set single cycle.

#define CMD_SET_DAC 0X1A
#define DAC_ON  0X00
#define DAC_OFF 0X01

#define CMD_PLAY_W_VOL    0X22
#define CMD_PLAYING_N     0x4C
#define CMD_QUERY_STATUS      0x42
#define CMD_QUERY_VOLUME      0x43
#define CMD_QUERY_FLDR_TRACKS 0x4e
#define CMD_QUERY_TOT_TRACKS  0x48
#define CMD_QUERY_FLDR_COUNT  0x4f

/************ Opitons **************************/
#define DEV_TF            0X02
/*********************************************************************/
//////////////////////////////////////////
// Rubén
// 04/12/19
///////////////////////////////////////////
RTC_DS3231 rtc;

////////////////////////////////////////
// Pulsadores
////////////////////////////////////////
#define PUL_001 22
#define PUL_002 23
#define PUL_003 24
#define PUL_004 25
#define PUL_005 26
#define PUL_006 27
#define PUL_007 28
#define PUL_008 29
#define PUL_009 30
#define PUL_010 31

////////////////////////////////////////
// Leds
////////////////////////////////////////
// Pines en los que conectamos los NeoPixels
#define TIRAS_LED_DIAS_SEMANA_PIN 2    
#define TIRAS_LED_DETECTOR_PIN 3
#define TIRAS_LED_FRANJA_HORARIA_PIN 5

//Designamos cuantos pixeles/leds componten nuestra tira
#define TIRAS_LED_DIAS_SEMANA_CANTIDAD 7  
#define TIRAS_LED_DETECTOR_CANTIDAD 13
#define TIRAS_LED_FRANJA_HORARIA_CANTIDAD 55

//Objeto TirasLed
Adafruit_NeoPixel tirasLedDiasSemana = Adafruit_NeoPixel(TIRAS_LED_DIAS_SEMANA_CANTIDAD, TIRAS_LED_DIAS_SEMANA_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel tirasLedDetector = Adafruit_NeoPixel(TIRAS_LED_DETECTOR_CANTIDAD, TIRAS_LED_DETECTOR_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel tirasLedFranjaHoraria = Adafruit_NeoPixel(TIRAS_LED_FRANJA_HORARIA_CANTIDAD, TIRAS_LED_FRANJA_HORARIA_PIN, NEO_GRB + NEO_KHZ800);

//Colores TiraDiaSemana
uint32_t offDias =           tirasLedDiasSemana.Color(0,0,0);
uint32_t verdeOscuro =       tirasLedDiasSemana.Color(0,130,0);
uint32_t verdePuro =         tirasLedDiasSemana.Color(0,255,0);
uint32_t magentaDiaSem =     tirasLedDiasSemana.Color(255,0,255);
uint32_t magentaOscDiaSem =  tirasLedDiasSemana.Color(130,0,130);

//Colores TiraDetecto
uint32_t offDetector =    tirasLedDetector.Color(0,0,0);
uint32_t rojoOscuro =     tirasLedDetector.Color(130,0,0);
uint32_t rojoPuro =       tirasLedDetector.Color(255,0,0);
uint32_t magentaDet =     tirasLedDetector.Color(255,0,255);
uint32_t magentaOscDet =  tirasLedDetector.Color(130,0,130);

//Colores TiraFranjaHoraria
uint32_t offFranjaHoraria         = tirasLedFranjaHoraria.Color(0,0,0);
uint32_t azulFranjaHoraria        = tirasLedFranjaHoraria.Color(0,0,255);
uint32_t azulOscFranjaHoraria      = tirasLedFranjaHoraria.Color(0,0,130);
uint32_t magentaFranjaHoraria     = tirasLedFranjaHoraria.Color(255,0,255);
uint32_t magentaOscFranjaHoraria  = tirasLedFranjaHoraria.Color(130,0,130);

////////////////////////////////////////
// Detector de movimiento
////////////////////////////////////////
#define PIN_PIR 4
boolean sensorPIR;
boolean lectura1PIR = false;

////////////////////////////////////////
// Tiempo
////////////////////////////////////////
String daysOfTheWeek[7] = { "Lunes", "Martes", "Miercoles", "Jueves", "Viernes", "Sabado","Domingo",  };
String monthsNames[12] = { "Enero", "Febrero", "Marzo", "Abril", "Mayo",  "Junio", "Julio","Agosto","Septiembre","Octubre","Noviembre","Diciembre" };
int diaSemana = 0;
int diaSemanaIngles = 0;

//////////////////////////////////////////
// Franjas Horarias
//////////////////////////////////////////
float intervalosHorarios [][3] = {{0.0, 9.5, 0.0},
                              {9.5, 10.5, 1.0},
                              {10.5, 11.5, 2.0},
                              {11.5, 12.5, 3.0},
                              {12.5, 13.5, 4.0},
                              {13.5, 14.5, 5.0},
                              {14.5, 23.99, 6.0}};
bool interHorariosLeds [][55] = {{false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,false, false, false, false, false, false, false, false, false, false, false,false, false, false, false, false, false, false, false, false, false, false,false, false, false, false, false, false, false, false, false, false, false},
                              {true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true,true, true, true, true, true, true, true, true, true, true, true,true, true, true, true, true, true, true, true, true, true, true,true, true, true, true, true, true, true, true, true, true, true},
                              {false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true,true, true, true, true, true, true, true, true, true, true, true,true, true, true, true, true, true, true, true, true, true, true,true, true, true, true, true, true, true, true, true, true, true},
                              {false, false, false, false, false, false, false, false, false, false, false,false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true,true, true, true, true, true, true, true, true, true, true, true,true, true, true, true, true, true, true, true, true, true, true},
                              {false, false, false, false, false, false, false, false, false, false, false,false, false, false, false, false, false, false, false, false, false, false,false, false, false, false, false, false, false, false, false, false, false,true, true, true, true, true, true, true, true, true, true, true,true, true, true, true, true, true, true, true, true, true, true},
                              {false, false, false, false, false, false, false, false, false, false, false,false, false, false, false, false, false, false, false, false, false, false,false, false, false, false, false, false, false, false, false, false, false,false, false, false, false, false, false, false, false, false, false, false,true, true, true, true, true, true, true, true, true, true, true},
                              {false, false, false, false, false, false, false, false, false, false, false,false, false, false, false, false, false, false, false, false, false, false,false, false, false, false, false, false, false, false, false, false, false,false, false, false, false, false, false, false, false, false, false, false,false, false, false, false, false, false, false, false, false, false, false}};                              

////////////////////////////////////////
// URI directorios - sonidos 
////////////////////////////////////////
int16_t url[][10]={
                {0x0101, 0x0102, 0x0103, 0x0104, 0x0105, 0x0106, 0x0107, 0x0108, 0x0109, 0x010A},
                {0x0201, 0x0202, 0x0203, 0x0204, 0x0205, 0x0206, 0x0207, 0x0208, 0x0209, 0x020A},
                {0x0301, 0x0302, 0x0303, 0x0304, 0x0305, 0x0306, 0x0307, 0x0308, 0x0309, 0x030A},
                {0x0401, 0x0402, 0x0403, 0x0404, 0x0405, 0x0406, 0x0407, 0x0408, 0x0409, 0x040A},
                {0x0501, 0x0502, 0x0503, 0x0504, 0x0505, 0x0506, 0x0507, 0x0508, 0x0509, 0x050A},
                {0x0601, 0x0602, 0x0603, 0x0604, 0x0605, 0x0606, 0x0607, 0x0608, 0x0609, 0x060A},
                {0x0701, 0x0702, 0x0703, 0x0704, 0x0705, 0x0706, 0x0707, 0x0708, 0x0709, 0x070A}
};

void setup()
{
  // Defino entradas pulsadores Panel
  pinMode(PUL_001, INPUT_PULLUP);
  pinMode(PUL_002, INPUT_PULLUP);
  pinMode(PUL_003, INPUT_PULLUP);
  pinMode(PUL_004, INPUT_PULLUP);  
  pinMode(PUL_005, INPUT_PULLUP);
  pinMode(PUL_006, INPUT_PULLUP); 
  pinMode(PUL_007, INPUT_PULLUP);  
  pinMode(PUL_008, INPUT_PULLUP);
  pinMode(PUL_009, INPUT_PULLUP); 
  pinMode(PUL_010, INPUT_PULLUP);

  // Defino Sensor PIR
  pinMode(PIN_PIR, INPUT);

  Serial.begin(9600);
  mp3.begin(9600);

  sendCommand(CMD_PLAY_W_INDEX, 0x01);
  delay(3000);

  tirasLedDiasSemana.begin(); 
  tirasLedDetector.begin();
  tirasLedFranjaHoraria.begin();
  
  arrancandoLeds(true);

  delay(500);

  sendCommand(CMD_SEL_DEV, DEV_TF);
  
  arrancandoLeds(false);
  
    
  if (!rtc.begin()) {
    Serial.println(F("Couldn't find RTC"));
    while (1);
  }
  // Si se ha perdido la corriente, fijar fecha y hora
  if (rtc.lostPower()) {
    // Fijar a fecha y hora de compilacion
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  ////////////////////////////////////////////////////////////////////
  // Si modulo de reloj no esta ajustado en fecha ni hora:
  // PASO1: descomentar la siguiente línea y cargar código al arduino
  //        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  // PASO2: Comentar la anterior línea y cargar código al arduino
  ///////////////////////////////////////////////////////////////////
}

bool entroSoloUnaVez = false;

void loop()
{
  comprobandoFecha();
  ilimunarDiaSemana();
  escuchandoPulsadores();
  escuchandoPIR();
  for(int i=0; i<7; i++){
        alarmaLeds(!esFinDeSemana(),franjaHoraria(intervalosHorarios[i][0], intervalosHorarios[i][1]), intervalosHorarios[i][2]);
  }
   
  delay(100);
}

//-----------------------------------
// Rubén R. V.
// 11/05/2020
//-----------------------------------
void alarmaLeds(bool diaLaboral,bool franja, float codigo){  
  if(diaLaboral){
    Serial.println("Es dia laborable");
    printDate();
    if(franja){
      Serial.println("Codigo: ");
      Serial.println(codigo);
      for(int i=0; i<TIRAS_LED_FRANJA_HORARIA_CANTIDAD; i++){
        if(interHorariosLeds[(int)codigo][i]){
          tirasLedFranjaHoraria.setPixelColor(i, azulOscFranjaHoraria); 
          tirasLedFranjaHoraria.show();
          Serial.print("1, ");
        }else{
          tirasLedFranjaHoraria.setPixelColor(i, offFranjaHoraria); 
          tirasLedFranjaHoraria.show();
          Serial.print("0, ");
        }    
      }
    }
  }
}

//-----------------------------------
// Rubén R. V.
// 11/05/2020
//-----------------------------------
bool esFinDeSemana(){
  bool retorno;
  (diaSemana == (5 || 6 )) ? retorno=true : retorno=false;
  return retorno;
}


//-----------------------------------
// Rubén R. V.
// 11/05/2020
//-----------------------------------
bool franjaHoraria(float horaInicio, float horaFinal){
  DateTime now = rtc.now();
  float horaActual = now.hour()+now.minute()/60.0;
  bool franjaHoraria = (horaActual > horaInicio && horaActual < horaFinal);
  return franjaHoraria;
}

//-----------------------------------
// Rubén R. V.
// 10/05/2020
//-----------------------------------
void comprobandoFecha(){
  DateTime now = rtc.now();
  if(now.dayOfTheWeek()!= (diaSemanaIngles)){
    diaSemanaIngles = now.dayOfTheWeek();
  }else{
      if(diaSemanaIngles==0){
        diaSemana = 6;  // Determino el caso para el domingo
      }else {
        diaSemana = diaSemanaIngles-1;
      }
  } 
}

//-----------------------------------
// Rubén R. V.
// 06/12/2019
//-----------------------------------
void escuchandoPIR(){
  sensorPIR = digitalRead(PIN_PIR);
  if((sensorPIR == HIGH)&&(lectura1PIR == false)){
    lectura1PIR = true;
    Serial.println("Sensor Movimiento ON");
    detectorLeds(rojoOscuro);
  } else if ((sensorPIR == LOW)&&(lectura1PIR == true)){
    lectura1PIR = false;
    Serial.println("Sensor Movimiento OFF");
    detectorLeds(offDetector);
  }
}

//-----------------------------------
// Rubén R. V.
// 05/12/2019 - 06/12/1019
//-----------------------------------
void arrancandoLeds(bool arranque){
  if(arranque){
    for(int i=0; i<7; i++){
    tirasLedDiasSemana.setPixelColor(i, magentaOscDiaSem); 
    tirasLedDiasSemana.show();
    tirasLedDetector.setPixelColor(i, magentaOscDet); 
    tirasLedDetector.show();
    tirasLedFranjaHoraria.setPixelColor(i, magentaOscFranjaHoraria); 
    tirasLedFranjaHoraria.show();  
    delay(100); 
    }
  }else{
    for(int i=0; i<7; i++){
    tirasLedDiasSemana.setPixelColor(i, offDias); 
    tirasLedDiasSemana.show();
    tirasLedDetector.setPixelColor(i, offDetector); 
    tirasLedDetector.show();
    tirasLedFranjaHoraria.setPixelColor(i, offFranjaHoraria); 
    tirasLedFranjaHoraria.show();  
    delay(100); 
    }
  }
}
void detectorLeds(uint32_t color){
  for(int i=0; i<TIRAS_LED_DETECTOR_CANTIDAD; i++){
    tirasLedDetector.setPixelColor(i, color); // Brillo color
    tirasLedDetector.show();   // Mostramos y actualizamos el color del pixel de nuestra cinta led RGB
  }
}

//-----------------------------------
// Rubén R. V.
// 05/12/2019
//-----------------------------------
void ilimunarDiaSemana(){
  for(int i=0; i<TIRAS_LED_DIAS_SEMANA_CANTIDAD; i++){
    if(diaSemana != i){
      tirasLedDiasSemana.setPixelColor(i, offDias);  
    }else{
      tirasLedDiasSemana.setPixelColor(i, verdeOscuro);  
    }
    tirasLedDiasSemana.show();   // Mostramos y actualizamos el color del pixel de nuestra cinta led RGB
  } 
}

//-----------------------------------
// Rubén R. V.
// 13/9/2019
// Actualizaciónes: 05/12/2019, 06/12/2019
// 28/04/2020 -> Al entrar por primera vez reproduce el mp3 001**.mp3 (Archivo asociado al dia de la semana que corresponda)
// 04/05/2020 -> Corrección error del dia de la semana del domingo
// 10/05/2020 -> Simplifico la función, pasando la comprobación de fecha a una nueva funón
//-----------------------------------
void escuchandoPulsadores(){

  int tmp = 210;

  if(!entroSoloUnaVez){  
    Serial.println(daysOfTheWeek[diaSemana]);
    Serial.println(diaSemana);
    sendCommand(CMD_PLAY_FOLDER_FILE, url[diaSemana][0]);
    sendCommand(CMD_QUERY_VOLUME, 0x1E);
    entroSoloUnaVez = true;
  }
  
  if(digitalRead(PUL_001)==LOW){
    Serial.println("Pulsado bt1");
    printDate();
    Serial.println(daysOfTheWeek[diaSemana]);
    Serial.println(diaSemana);
    delay(tmp);
    sendCommand(CMD_PLAY_FOLDER_FILE, url[diaSemana][0]);
  }

  
  if(digitalRead(PUL_002)==LOW){
    Serial.println("Pulsado bt2");
    printDate();
    delay(tmp);
    sendCommand(CMD_PLAY_FOLDER_FILE, url[diaSemana][1]);
  }
  
  if(digitalRead(PUL_003)==LOW){
    Serial.println("Pulsado bt3");
    printDate();
    delay(tmp);
    sendCommand(CMD_PLAY_FOLDER_FILE, url[diaSemana][2]);
  }
    
  if(digitalRead(PUL_004)==LOW){
    Serial.println("Pulsado bt4");
    printDate();
    delay(tmp);
    sendCommand(CMD_PLAY_FOLDER_FILE, url[diaSemana][3]);
  }
     
  if(digitalRead(PUL_005)==LOW){
    Serial.println("Pulsado bt5");
    printDate();
    delay(tmp);
    sendCommand(CMD_PLAY_FOLDER_FILE, url[diaSemana][4]);
  } 
       
  if(digitalRead(PUL_006)==LOW){
    Serial.println("Pulsado bt6");
    printDate();
    delay(tmp);
    sendCommand(CMD_PLAY_FOLDER_FILE, url[diaSemana][5]);
  } 
      
  if(digitalRead(PUL_007)==LOW){
    Serial.println("Pulsado bt7");
    printDate();
    delay(tmp);
    sendCommand(CMD_PLAY_FOLDER_FILE, url[diaSemana][6]);
  }
     
  if(digitalRead(PUL_008)==LOW){
    Serial.println("Pulsado bt8");
    printDate();
    delay(tmp);
    sendCommand(CMD_PLAY_FOLDER_FILE, url[diaSemana][7]);

  } 
       
  if(digitalRead(PUL_009)==LOW){
    Serial.println("Pulsado bt9");
    printDate();
    delay(tmp);
    sendCommand(CMD_PLAY_FOLDER_FILE, url[diaSemana][8]);
  } 

  if(digitalRead(PUL_010)==LOW){
    Serial.println("Pulsado bt10");
    printDate();
    delay(tmp);
    sendCommand(CMD_PLAY_FOLDER_FILE, url[diaSemana][9]);
  } 

}

/********************************************************************************/
/*Function decodeMP3Answer: Decode MP3 answer.                                  */
/*Parameter:-void                                                               */
/*Return: The                                                  */
String decodeMP3Answer() {
  String decodedMP3Answer = "";
  decodedMP3Answer += sanswer();
  switch (ansbuf[3]) {
    case 0x3A:
      decodedMP3Answer += " -> Memory card inserted.";
      break;
    case 0x3D:
      decodedMP3Answer += " -> Completed play num " + String(ansbuf[6], DEC);
      break;
    case 0x40:
      decodedMP3Answer += " -> Error";
      break;
    case 0x41:
      decodedMP3Answer += " -> Data recived correctly. ";
      break;
    case 0x42:
      decodedMP3Answer += " -> Status playing: " + String(ansbuf[6], DEC);
      break;
    case 0x48:
      decodedMP3Answer += " -> File count: " + String(ansbuf[6], DEC);
      break;
    case 0x4C:
      decodedMP3Answer += " -> Playing: " + String(ansbuf[6], DEC);
      break;
    case 0x4E:
      decodedMP3Answer += " -> Folder file count: " + String(ansbuf[6], DEC);
      break;
    case 0x4F:
      decodedMP3Answer += " -> Folder count: " + String(ansbuf[6], DEC);
      break;
  }
  return decodedMP3Answer;
}

/********************************************************************************/
/*Function: Send command to the MP3                                         */
/*Parameter:-int8_t command                                                     */
/*Parameter:-int16_ dat  parameter for the command                              */
void sendCommand(int8_t command, int16_t dat)
{
  delay(20);
  Send_buf[0] = 0x7e;   //
  Send_buf[1] = 0xff;   //
  Send_buf[2] = 0x06;   // Len
  Send_buf[3] = command;//
  Send_buf[4] = 0x01;   // 0x00 NO, 0x01 feedback
  Send_buf[5] = (int8_t)(dat >> 8);  //datah
  Send_buf[6] = (int8_t)(dat);       //datal
  Send_buf[7] = 0xef;   //
  Serial.print("Sending: ");
  for (uint8_t i = 0; i < 8; i++)
  {
    mp3.write(Send_buf[i]) ;
    Serial.print(sbyte2hex(Send_buf[i]));
  }
  Serial.println();
}

/********************************************************************************/
/*Function: sbyte2hex. Returns a byte data in HEX format.                 */
/*Parameter:- uint8_t b. Byte to convert to HEX.                                */
/*Return: String                                                                */
String sbyte2hex(uint8_t b)
{
  String shex;
  shex = "0X";
  if (b < 16) shex += "0";
  shex += String(b, HEX);
  shex += " ";
  return shex;
}

/********************************************************************************/
/*Function: sanswer. Returns a String answer from mp3 UART module.          */
/*Parameter:- uint8_t b. void.                                                  */
/*Return: String. If the answer is well formated answer.                        */
String sanswer(void)
{
  uint8_t i = 0;
  String mp3answer = "";
  // Get only 10 Bytes
  while (mp3.available() && (i < 10))
  {
    uint8_t b = mp3.read();
    ansbuf[i] = b;
    i++;
    mp3answer += sbyte2hex(b);
  }
  // if the answer format is correct.
  if ((ansbuf[0] == 0x7E) && (ansbuf[9] == 0xEF))
  {
    return mp3answer;
  }
  return "???: " + mp3answer;
}

void printDate()
{
  DateTime date = rtc.now();

  Serial.print(date.year(), DEC);
  Serial.print('/');
  Serial.print(date.month(), DEC);
  Serial.print('/');
  Serial.print(date.day(), DEC);
  Serial.print(" (");
  Serial.print(daysOfTheWeek[date.dayOfTheWeek()-1]);
  Serial.print(") ");
  Serial.print(date.hour(), DEC);
  Serial.print(':');
  Serial.print(date.minute(), DEC);
  Serial.print(':');
  Serial.print(date.second(), DEC);
  Serial.println();
}
