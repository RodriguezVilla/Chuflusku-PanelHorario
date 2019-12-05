#include <Wire.h>
#include <RTClib.h>
#include <Adafruit_NeoPixel.h>

/////////////////////////////////////
// Chuflusku: Panel Horario
// Rubén Rodríguez Villa
// V1.0.0 -  05/12/2019
/////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Demo for the Serial MP3 Player Catalex (YX5300 chip)
// Hardware: Serial MP3 Player *1
// Board:  Arduino UNO
// http://www.dx.com/p/uart-control-serial-mp3-music-player-module-for-arduino-avr-arm-pic-blue-silver-342439#.VfHyobPh5z0
// Fuente base MP3 Base player:  https://joanruedapauweb.com/blog/index.php/2017/02/07/arduino-serial-mp3-player-yx5300-es/
// Uncomment SoftwareSerial for Arduino Uno or Nano.
// #include <SoftwareSerial.h>
// #define ARDUINO_RX 5  //should connect to TX of the Serial MP3 Player module
// #define ARDUINO_TX 6  //connect to RX of the module


//SoftwareSerial mp3(ARDUINO_RX, ARDUINO_TX);
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
#define PIXEL_PIN 2    // Digital IO pin connected to the NeoPixels.
#define PIXEL_COUNT 7  // Designamos cuantos pixeles tenemos en nuestra cinta led RGB

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

uint32_t off = pixels.Color(0,0,0);
uint32_t rojoOscuro = pixels.Color(130,0,0);
uint32_t rojoPuro = pixels.Color(255,0,0);
uint32_t verdeOscuro = pixels.Color(0,130,0);
uint32_t verdePuro = pixels.Color(0,255,0);
uint32_t magenta = pixels.Color(255,0,255);


////////////////////////////////////////
// Tiempo
////////////////////////////////////////
String daysOfTheWeek[7] = { "Lunes", "Martes", "Miercoles", "Jueves", "Viernes", "Sabado","Domingo",  };
String monthsNames[12] = { "Enero", "Febrero", "Marzo", "Abril", "Mayo",  "Junio", "Julio","Agosto","Septiembre","Octubre","Noviembre","Diciembre" };
int diaSemana = 0;
int diaSemanaIngles = 0;



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

  Serial.begin(9600);
  mp3.begin(9600);

  pixels.begin(); 
  arrancandoLeds(PIXEL_COUNT, magenta);
  
  if (!rtc.begin()) {
    Serial.println(F("Couldn't find RTC"));
    while (1);
  }
  // Si se ha perdido la corriente, fijar fecha y hora
  if (rtc.lostPower()) {
    // Fijar a fecha y hora de compilacion
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  sendCommand(CMD_SEL_DEV, DEV_TF);
  arrancandoLeds(PIXEL_COUNT, off);
}


bool entroSoloUnaVez = false;

void loop()
{


  escuchandoPulsadores();
  
  char c = ' ';

  // If there a char on Serial call sendMP3Command to sendCommand
  if ( Serial.available() )
  {
    c = Serial.read();
    sendMP3Command(c);
  }
  // Check for the answer.
  if (mp3.available())
  {
    Serial.println(decodeMP3Answer());
  }

  delay(100);
}

//-----------------------------------
// Rubén R. V.
// 05/12/2019
//-----------------------------------
void arrancandoLeds(int numPixel,uint32_t color){
  for(int i=0; i<numPixel; i++){
    pixels.setPixelColor(i, color); // Brillo color
    pixels.show();   // Mostramos y actualizamos el color del pixel de nuestra cinta led RGB
    delay(100); // Pausa por un periodo de tiempo (en milisegundos).
  }
}


//-----------------------------------
// Rubén R. V.
// 05/12/2019
//-----------------------------------
void ilimunarDiaSemana(){
  for(int i=0; i<7; i++){
    if(diaSemana != i){
      pixels.setPixelColor(i, off);  
    }else{
      pixels.setPixelColor(i, verdeOscuro);  
    }
    pixels.show();   // Mostramos y actualizamos el color del pixel de nuestra cinta led RGB
  } 
}
//-----------------------------------
// Rubén R. V.
// 13/9/2019
// Actualización 05/12/2019
//-----------------------------------
void escuchandoPulsadores(){

  DateTime now = rtc.now();
  if(now.dayOfTheWeek()!= (diaSemanaIngles)){
    diaSemanaIngles = now.dayOfTheWeek();
    if(diaSemanaIngles==0){
      
    }
    diaSemana = diaSemanaIngles-1;
  }

  ilimunarDiaSemana();

  int tmp = 210;

  if(!entroSoloUnaVez){
    
  Serial.println(daysOfTheWeek[diaSemana]);
  Serial.println(diaSemana);
    sendCommand(CMD_PLAY_FOLDER_FILE, url[diaSemana][9]);
    sendCommand(CMD_QUERY_VOLUME, 0x1E);
    entroSoloUnaVez = true;
  }
  
  if(digitalRead(PUL_001)==LOW){
    Serial.println("Pulsado bt1");
    printDate(now);

    delay(tmp);
    sendCommand(CMD_PLAY_FOLDER_FILE, url[diaSemana][0]);
  }

  
  if(digitalRead(PUL_002)==LOW){
    Serial.println("Pulsado bt2");
      printDate(now);

     delay(tmp);
    sendCommand(CMD_PLAY_FOLDER_FILE, url[diaSemana][1]);

  }
  
  if(digitalRead(PUL_003)==LOW){
    Serial.println("Pulsado bt3");
    delay(tmp);
    sendCommand(CMD_PLAY_FOLDER_FILE, url[diaSemana][2]);
  }
    
  if(digitalRead(PUL_004)==LOW){
    Serial.println("Pulsado bt4");
    delay(tmp);
    sendCommand(CMD_PLAY_FOLDER_FILE, url[diaSemana][3]);
  }
     
  if(digitalRead(PUL_005)==LOW){
    Serial.println("Pulsado bt5");
    delay(tmp);
    sendCommand(CMD_PLAY_FOLDER_FILE, url[diaSemana][4]);

  } 
       
  if(digitalRead(PUL_006)==LOW){
    Serial.println("Pulsado bt6");
    delay(tmp);
    sendCommand(CMD_PLAY_FOLDER_FILE, url[diaSemana][5]);
  } 
      
  if(digitalRead(PUL_007)==LOW){
    Serial.println("Pulsado bt7");
    delay(tmp);
    sendCommand(CMD_PLAY_FOLDER_FILE, url[diaSemana][6]);
  }
     
  if(digitalRead(PUL_008)==LOW){
    Serial.println("Pulsado bt8");
    delay(tmp);
    sendCommand(CMD_PLAY_FOLDER_FILE, url[diaSemana][7]);

  } 
       
  if(digitalRead(PUL_009)==LOW){
    Serial.println("Pulsado bt9");
    delay(tmp);
    sendCommand(CMD_PLAY_FOLDER_FILE, url[diaSemana][8]);
  } 

  if(digitalRead(PUL_010)==LOW){
    Serial.println("Pulsado bt10");
    delay(tmp);
    sendCommand(CMD_PLAY_FOLDER_FILE, url[diaSemana][9]);
  } 

}


/********************************************************************************/
/*Function sendMP3Command: seek for a 'c' command and send it to MP3  */
/*Parameter: c. Code for the MP3 Command, 'h' for help.                                                                                                         */
/*Return:  void                                                                */

void sendMP3Command(char c) {
  switch (c) {
    case '?':
    case 'h':
      Serial.println("HELP  ");
      Serial.println(" p = Play");
      Serial.println(" P = Pause");
      Serial.println(" > = Next");
      Serial.println(" < = Previous");
      Serial.println(" + = Volume UP");
      Serial.println(" - = Volume DOWN");
      Serial.println(" c = Query current file");
      Serial.println(" q = Query status");
      Serial.println(" v = Query volume");
      Serial.println(" x = Query folder count");
      Serial.println(" t = Query total file count");
      Serial.println(" 1 = Play folder 1");
      Serial.println(" 2 = Play folder 2");
      Serial.println(" 3 = Play folder 3");
      Serial.println(" 4 = Play folder 4");
      Serial.println(" 5 = Play folder 5");
      Serial.println(" S = Sleep");
      Serial.println(" W = Wake up");
      Serial.println(" r = Reset");
      break;


    case 'p':
      Serial.println("Play ");
      sendCommand(CMD_PLAY, 0);
      break;

    case 'P':
      Serial.println("Pause");
      sendCommand(CMD_PAUSE, 0);
      break;


    case '>':
      Serial.println("Next");
      sendCommand(CMD_NEXT_SONG, 0);
      sendCommand(CMD_PLAYING_N, 0x0000); // ask for the number of file is playing
      break;


    case '<':
      Serial.println("Previous");
      sendCommand(CMD_PREV_SONG, 0);
      sendCommand(CMD_PLAYING_N, 0x0000); // ask for the number of file is playing
      break;

    case '+':
      Serial.println("Volume Up");
      sendCommand(CMD_VOLUME_UP, 0);
      break;

    case '-':
      Serial.println("Volume Down");
      sendCommand(CMD_VOLUME_DOWN, 0);
      break;

    case 'c':
      Serial.println("Query current file");
      sendCommand(CMD_PLAYING_N, 0);
      break;
      
    case 'q':
      Serial.println("Query status");
      sendCommand(CMD_QUERY_STATUS, 0);
      break;

    case 'v':
      Serial.println("Query volume");
      sendCommand(CMD_QUERY_VOLUME, 0);
      break;

    case 'x':
      Serial.println("Query folder count");
      sendCommand(CMD_QUERY_FLDR_COUNT, 0);
      break;

    case 't':
      Serial.println("Query total file count");
      sendCommand(CMD_QUERY_TOT_TRACKS, 0);
      break;

    case '1':
      Serial.println("Play folder 1");
      sendCommand(CMD_FOLDER_CYCLE, 0x0101);
      break;

    case '2':
      Serial.println("Play folder 2");
      sendCommand(CMD_FOLDER_CYCLE, 0x0201);
 
      break;

    case '3':
      Serial.println("Play folder 3");
      sendCommand(CMD_PLAY_FOLDER_FILE, 0x0301);
      break;

    case '4':
      Serial.println("Play folder 4");
      sendCommand(CMD_FOLDER_CYCLE, 0x0401);
      break;

    case '5':
      Serial.println("Play folder 5");
      sendCommand(CMD_FOLDER_CYCLE, 0x0501);
      break;
    case '6':
      Serial.println("Play folder 6");
      sendCommand(CMD_FOLDER_CYCLE, 0x0601);
      break;

    case '7':
      Serial.println("Play folder 7");
      sendCommand(CMD_FOLDER_CYCLE, 0x0701);
      break;

    case '8':
      Serial.println("Play folder 8");
      sendCommand(CMD_FOLDER_CYCLE, 0x0801);
      break;

    case '9':
      Serial.println("Play folder 9");
      sendCommand(CMD_FOLDER_CYCLE, 0x0901);
      break;

    case 'S':
      Serial.println("Sleep");
      sendCommand(CMD_SLEEP_MODE, 0x00);
      break;

    case 'W':
      Serial.println("Wake up");
      sendCommand(CMD_WAKE_UP, 0x00);
      break;

    case 'r':
      Serial.println("Reset");
      sendCommand(CMD_RESET, 0x00);
      break;
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

void printDate(DateTime date)
{
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
