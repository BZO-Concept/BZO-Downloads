/********************************************************************************************
 * 
 * Code pour simulateur BZO sur base STM32F103C8T6 (BluePill)
 * avec écran OLED SSD1306 pour affichage des constantes de simulation
 * Sniffer CAN embarqué
 * Contact: www.bzo-concept.fr
 * v1.0 01/06/26
 * 
 *******************************************************************************************/

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "hal_conf_extra.h"
#include "STM32_CAN.h"
#include <EEPROM.h>

STM32_CAN Can( CAN1, DEF, RX_SIZE_64, TX_SIZE_16 );

static CAN_message_t CAN_RX_msg;
static CAN_message_t CAN_TX_msg;

HardwareTimer *timCRANK = new HardwareTimer(TIM2); //On choisis le timer2 pour timCRANK
HardwareTimer *timCAM   = new HardwareTimer(TIM3); //On choisis le timer3 pour timCAM

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define OLED_DC     PB11
#define OLED_CS     PB14
#define OLED_RESET  PB10
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT,
  &SPI, OLED_DC, OLED_RESET, OLED_CS);
  
#define FREQ_SCREEN 150 //ms

//************************************************************************************************************
//Images
const unsigned char BZO_bitmap[] PROGMEM = {
  // 'LOGO_JPG, 89x64px
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xe0, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x06, 0x18, 0x00, 0x03, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x18, 0x00, 
  0x07, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x38, 0x00, 0x06, 0x30, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xf0, 0x00, 0x0c, 0x18, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00, 
  0x00, 0x01, 0xc0, 0x00, 0x0c, 0x18, 0x00, 0x00, 0x3b, 0x80, 0x00, 0x00, 0x00, 0x01, 0x80, 0x00, 
  0x06, 0x30, 0x00, 0x00, 0x30, 0xc0, 0x00, 0x00, 0x00, 0x01, 0x80, 0x00, 0x07, 0xf0, 0x20, 0x00, 
  0x20, 0xc0, 0x00, 0x00, 0x00, 0x01, 0x00, 0xc0, 0x03, 0xc0, 0xf8, 0x00, 0x30, 0xc0, 0x00, 0x00, 
  0x00, 0x03, 0x01, 0xf0, 0x01, 0x81, 0xd8, 0x00, 0x31, 0xc0, 0x00, 0x00, 0x00, 0x03, 0x03, 0x30, 
  0x03, 0x01, 0x88, 0x00, 0x1f, 0x80, 0x00, 0x00, 0x00, 0x03, 0x03, 0x30, 0x03, 0x01, 0x98, 0x00, 
  0x0e, 0x00, 0x00, 0x00, 0x00, 0xff, 0xc3, 0x30, 0x03, 0x00, 0xf8, 0x00, 0x0c, 0x00, 0x00, 0x00, 
  0x00, 0xff, 0xc1, 0xe0, 0x03, 0x00, 0x70, 0x00, 0x08, 0x00, 0x00, 0x00, 0x01, 0xff, 0xc1, 0xc0, 
  0x06, 0x00, 0x60, 0x00, 0x18, 0x00, 0x00, 0x00, 0x01, 0xc1, 0xc1, 0x80, 0x06, 0x00, 0xc0, 0x00, 
  0x18, 0x00, 0x00, 0x00, 0x01, 0x81, 0xff, 0x81, 0xff, 0xff, 0xfc, 0x0f, 0xff, 0x00, 0x00, 0x00, 
  0x01, 0x81, 0xff, 0xe1, 0xff, 0xff, 0xfc, 0x3f, 0xff, 0xc0, 0x00, 0x00, 0x01, 0x81, 0xff, 0xf1, 
  0xff, 0xff, 0xf8, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x03, 0x80, 0x00, 0x79, 0x80, 0x00, 0x19, 0xe0, 
  0x00, 0xf0, 0x00, 0x00, 0x03, 0x00, 0x00, 0x3b, 0x00, 0x00, 0x1b, 0x80, 0x00, 0x70, 0x00, 0x00, 
  0x03, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x38, 0x00, 0x00, 0x03, 0x00, 0x00, 0x1f, 
  0x00, 0x00, 0x3e, 0x00, 0x00, 0x18, 0x78, 0x00, 0x03, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x3c, 0x00, 
  0x00, 0x1c, 0xfc, 0x00, 0x07, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x38, 0x00, 0x00, 0x1f, 0xcc, 0x00, 
  0x06, 0x07, 0xf8, 0x0f, 0xff, 0x80, 0x78, 0x0f, 0xf0, 0x1f, 0xcc, 0x00, 0x06, 0x07, 0xfc, 0x0f, 
  0xff, 0x80, 0xf0, 0x1f, 0xf8, 0x1c, 0xfc, 0x00, 0x06, 0x0f, 0xfc, 0x0f, 0xfe, 0x01, 0xf0, 0x3f, 
  0xf8, 0x1c, 0x78, 0x00, 0x0e, 0x0e, 0x0c, 0x0e, 0x78, 0x07, 0xf0, 0x70, 0x1c, 0x18, 0x00, 0x00, 
  0x0e, 0x0c, 0x0c, 0x0f, 0xf0, 0x1f, 0x60, 0x60, 0x18, 0x18, 0x00, 0x00, 0x0c, 0x0c, 0x1c, 0x0f, 
  0xc0, 0x3e, 0x60, 0x70, 0x38, 0x18, 0x00, 0x00, 0x0c, 0x0f, 0xfc, 0x1f, 0x80, 0xff, 0xe0, 0x7f, 
  0xf0, 0x38, 0x00, 0x00, 0x1c, 0x0f, 0xf8, 0x1e, 0x01, 0xff, 0xe0, 0x7f, 0xf0, 0x30, 0x00, 0x00, 
  0x1c, 0x0f, 0xe0, 0x3c, 0x03, 0xff, 0xe0, 0x1f, 0xc0, 0x70, 0x3e, 0x00, 0x18, 0x00, 0x00, 0x38, 
  0x00, 0x00, 0xe0, 0x00, 0x00, 0x60, 0x67, 0x00, 0x18, 0x00, 0x00, 0x78, 0x00, 0x01, 0xf0, 0x00, 
  0x00, 0xe0, 0xc3, 0x00, 0x18, 0x00, 0x00, 0xf8, 0x00, 0x01, 0xf0, 0x00, 0x01, 0xff, 0xc3, 0x00, 
  0x38, 0x00, 0x01, 0xf8, 0x00, 0x01, 0xf8, 0x00, 0x03, 0xff, 0xc3, 0x00, 0x30, 0x00, 0x03, 0xb8, 
  0x00, 0x01, 0xb8, 0x00, 0x07, 0x00, 0xe3, 0x00, 0x30, 0x00, 0x0f, 0x30, 0x00, 0x03, 0x9e, 0x00, 
  0x1e, 0x00, 0x7e, 0x00, 0x3f, 0xff, 0xfe, 0x3f, 0xff, 0xff, 0x8f, 0xff, 0xfc, 0x00, 0x18, 0x00, 
  0x3f, 0xff, 0xf8, 0x3f, 0xff, 0xff, 0x07, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xe0, 0x3f, 
  0xff, 0xff, 0x01, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x02, 0x01, 0x80, 0x00, 0x30, 
  0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x06, 0x01, 0x80, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 
  0x06, 0x00, 0x00, 0x06, 0x01, 0x80, 0x01, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x06, 
  0x03, 0x00, 0x03, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x1e, 0x03, 0x00, 0x03, 0x18, 
  0x00, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x3f, 0x03, 0x00, 0x06, 0x08, 0x00, 0x00, 0x00, 0x00, 
  0x41, 0x00, 0x00, 0x33, 0x03, 0x00, 0x06, 0x08, 0x00, 0x00, 0x00, 0x00, 0xc1, 0x00, 0x00, 0x33, 
  0x06, 0x00, 0x03, 0x18, 0x00, 0x00, 0x00, 0x00, 0x61, 0x00, 0x00, 0x3f, 0x06, 0x00, 0x03, 0xf8, 
  0x00, 0x00, 0x00, 0x00, 0x77, 0x00, 0x00, 0x1e, 0x1e, 0x00, 0x01, 0xf0, 0x00, 0x00, 0x00, 0x00, 
  0x3e, 0x00, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x71, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x80, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x61, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x3f, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
//************************************************************************************************************

//************************************************************************************************************
//Pinout
#define PUSH1    PB13
#define PUSH2    PB12
#define POT_RPM   PB1
#define POT_O2    PB0
#define POT_TPS   PA6
#define POT_MAP   PA4
#define VIN_ADC   PA3
#define I3V3_ADC  PA2
#define I5V_ADC   PA1
#define IVIN_ADC  PA0
#define CRANK_OUT PB3
#define CAM_OUT   PB4
//************************************************************************************************************

//************************************************************************************************************
//Variables
#define NORMAL_PUSH_TIME 50 //ms
#define LONG_PUSH_TIME 1500 //ms
bool Menu_Open = false;
bool temp_binary1 = false;
bool temp_binary2 = false;
bool last_temp_binary1 = false;
bool last_temp_binary2 = false;
uint32_t count_binary = 0;
bool statePush1 = false;
bool statePush2 = false;
bool NormalPush1 = false;
bool NormalPush2 = false;
bool LongPush1 = false;
bool LongPush2 = false;

uint8_t Input_Number_ADC = 0;
uint16_t POT_RPM_value1 = 0;
uint16_t POT_RPM_value2 = 0;
uint16_t POT_O2_value1 = 0;
uint16_t POT_O2_value2 = 0;
uint16_t POT_TPS_value1 = 0;
uint16_t POT_TPS_value2 = 0;
uint16_t POT_MAP_value1 = 0;
uint16_t POT_MAP_value2 = 0;
uint16_t VIN_ADC_value1 = 0;
uint16_t VIN_ADC_value2 = 0;
uint16_t I3V3_ADC_value1 = 0;
uint16_t I3V3_ADC_value2 = 0;
uint16_t I5V_ADC_value1 = 0;
uint16_t I5V_ADC_value2 = 0;
uint16_t IVIN_ADC_value1 = 0;
uint16_t IVIN_ADC_value2 = 0;

uint16_t temp_ADC = 0;   //utilisé dans les calculs valeurs ADC
uint16_t RPM_value = 0;  //0-16000RPM
uint16_t O2_value = 0;   //0-500centivolts
uint16_t TPS_value = 0;  //0-500centivolts (imprécise, MISO)
uint16_t MAP_value = 0;  //0-500centivolts
uint16_t VIN_value = 0;  //0-360décivolts
uint16_t I3V3_value = 0; //0-165dA
uint16_t I5V_value = 0;  //0-165dA
uint16_t IVIN_value = 0; //0-165dA
//!!Lectures d'intensité provisoire, imprécise (changer les CI, mauvaise ref)
bool MAX_Mode = false;

uint16_t Memory_Number = 0;
#define CAMOFFSET_MEMORY 10
#define CRANKOFFSET_MEMORY 200
#define MAXSIZE_MEMORY 1000
uint8_t Screen = 0;
int Speed_of_CAN = 0; //(1 = 250000, 2 = 500000, 3 = 1000000)
uint8_t line_CAN[10][7] = {0};
bool Can_Pause = false;
uint8_t Menu_Selector = 0;
bool CamWheel_Generator = false;
bool CrankWheel_Generator = false;
uint8_t Draw_Cursor = 0;
uint8_t Page = 0;
uint8_t Page_Finale = 0;
uint8_t NB_FinalPage_Value = 0;
//************************************************************************************************************

//************************************************************************************************************
// Simulation des roues phoniques:
uint8_t SELECT_CrankWheel = 0;
uint8_t SIZE_CrankWheel = 0;
uint8_t SIZE_CrankWheel_Custom = 0;
bool DRAWING_CrankWheel[255] = {0};
unsigned long Interval_Value = 0; //intervale entre chaque changement d'état de la roue.
unsigned long Interval_Value_Cam = 0; //intervale entre chaque changement d'état de la roue.
volatile uint8_t Crank_counter = 0;
bool Cam_Speed = false;
bool Cam_Custom = false;
#define SIZE_CamWheel 72
bool DRAWING_CamWheel[72] = {0};
uint8_t Cam_counter = 0;
bool Cycle_Flag = false;

/*******************************************************************
 * Ajouter une roue phonique CRANK dans ce code:
 * 
 * 1- definir un nom de code pour la roue (optionnel, le chiffre brut peux etre utilisé)
 * 2- Wheelenum(): affiche les roues phoniques en fonction de leur numéro 
 * 3- dessiner la roue dans un nouveau bool, "[x]" représente sa taille (nombre de dents x2)
 * 4- CrankWheelSelector(): ajouter le case pour sa nouvelle roue en respectant la syntaxe des autres roues présentes (copier 36-1 ou 60-2)
 * 
 *******************************************************************/

#define SELECT_Crank_Custom   0
#define SELECT_Crank_36moins1 1 //36-1
#define SELECT_Crank_60moins2 2 //60-2
  //Ajouter ici les #define des CrankWheel supplémentaires (optionnel)
 
void Wheelenum(){
  if(SELECT_CrankWheel == SELECT_Crank_Custom){display.print("CrankWheel: Custom");}
  if(SELECT_CrankWheel == SELECT_Crank_36moins1){display.print("CrankWheel:   36-1");}
  if(SELECT_CrankWheel == SELECT_Crank_60moins2){display.print("CrankWheel:   60-2");}
    //Ajouter ici les noms des CrankWheel supplémentaires (affiché dans le menu)
}

#define NB_CRANKWHEEL 3   //Nombre de CrankWheel disponibles

//Ont utilise le nombre de dents x2, pour pouvoir définir a la fois l'état des dents et l'état de l'écart entre les dents.
//36-1
bool Crank_36moins1[72] = {true, false, true, false, true, false, true, false, true, false, true, false, true, false, true, false, true, false, true, false, //x10
                           true, false, true, false, true, false, true, false, true, false, true, false, true, false, true, false, true, false, true, false, //x10
                           true, false, true, false, true, false, true, false, true, false, true, false, true, false, true, false, true, false, true, false, //x10
                           true, false, true, false, true, false, true, false, true, false, false, false};
                       
//60-2
bool Crank_60moins2[120] = {true, false, true, false, true, false, true, false, true, false, true, false, true, false, true, false, true, false, true, false, //x10
                            true, false, true, false, true, false, true, false, true, false, true, false, true, false, true, false, true, false, true, false, //x10
                            true, false, true, false, true, false, true, false, true, false, true, false, true, false, true, false, true, false, true, false, //x10
                            true, false, true, false, true, false, true, false, true, false, true, false, true, false, true, false, true, false, true, false, //x10
                            true, false, true, false, true, false, true, false, true, false, true, false, true, false, true, false, true, false, true, false, //x10
                            true, false, true, false, true, false, true, false, true, false, true, false, true, false, true, false, false, false, false, false};

  //Ajouter ici les dessins des CrankWheel supplémentaires

void CrankWheelSelector(){
  
  switch(SELECT_CrankWheel){

    case SELECT_Crank_Custom: 
      SIZE_CrankWheel = SIZE_CrankWheel_Custom;
      Memory_Number = CRANKOFFSET_MEMORY;
      Crank_counter = 0;
      while(Memory_Number < SIZE_CrankWheel + CRANKOFFSET_MEMORY){
        DRAWING_CrankWheel[Crank_counter] = EEPROM.read(Memory_Number);
        Crank_counter++;
        Memory_Number++;
      } 
      Crank_counter = 0;
    break;

    case SELECT_Crank_36moins1:
      SIZE_CrankWheel = sizeof(Crank_36moins1);
      memcpy(DRAWING_CrankWheel, Crank_36moins1, SIZE_CrankWheel);
    break;

    case SELECT_Crank_60moins2:
      SIZE_CrankWheel = sizeof(Crank_60moins2);
      memcpy(DRAWING_CrankWheel, Crank_60moins2, SIZE_CrankWheel);
    break;

  //ajouter ici une case pour chaque CrankWheel suplémentaire

    default:
      SELECT_CrankWheel = SELECT_Crank_36moins1;
    break;
  }
}

void CamWheelCustom(){
  Memory_Number = CAMOFFSET_MEMORY;
  Cam_counter = 0;
  while(Memory_Number < SIZE_CamWheel + CAMOFFSET_MEMORY){
    DRAWING_CamWheel[Cam_counter] = EEPROM.read(Memory_Number);
    Cam_counter++;
    Memory_Number++;
  }
  Cam_counter = 0;
}
  
void CrankISR(){
  
  digitalWrite(CRANK_OUT, DRAWING_CrankWheel[Crank_counter]);
  Crank_counter++;
  if(Crank_counter >= SIZE_CrankWheel){
    Crank_counter = 0; 
    Cycle_Flag = !Cycle_Flag; 
    if(Cycle_Flag == false && Crank_counter == 0){timCAM->refresh(); Cam_counter = 0;}     //timCAM->refresh(); remet a 0 le timer CAM
  }  
  if(Cam_Custom == false && Cam_Speed == false){
    if(Cycle_Flag == true && (Crank_counter == 10 || Crank_counter == 11)){digitalWrite(CAM_OUT, HIGH);}
    if(Cycle_Flag == true && Crank_counter == 12){digitalWrite(CAM_OUT, LOW);}
  }
}

void CamISR(){
  
  digitalWrite(CAM_OUT, DRAWING_CamWheel[Cam_counter]);
  Cam_counter++;
}

void InitISR(){
  
  timCRANK->setOverflow(1000, MICROSEC_FORMAT);  //initialise le timer a une vitesse par défaut (mis a la bonne vitesse dans la lecture du potentiometre)
  timCRANK->attachInterrupt(CrankISR);           //attache l'interruption a timCRANK (l'interruption s'exectute a chaque bip timer)
  timCRANK->pause();                             //On stoppe le timer pour l'instant

  timCAM->setOverflow(1000, MICROSEC_FORMAT);
  timCAM->attachInterrupt(CamISR);
  timCAM->pause();
}
//************************************************************************************************************

//************************************************************************************************************
//CANBUS
void InitCAN(){
  
  Can.begin();    
  if(Speed_of_CAN == 1){Can.setBaudRate(250000);}
  if(Speed_of_CAN == 2){Can.setBaudRate(500000);}
  if(Speed_of_CAN == 3){Can.setBaudRate(1000000);}
}
//************************************************************************************************************

//************************************************************************************************************
//Setup
void setup() {
  
  pinMode(PUSH1, INPUT_PULLUP);
  pinMode(PUSH2, INPUT_PULLUP);
  pinMode(POT_RPM, INPUT_ANALOG);
  pinMode(POT_O2, INPUT_ANALOG);
  pinMode(POT_TPS, INPUT_ANALOG);
  pinMode(POT_MAP, INPUT_ANALOG);
  pinMode(VIN_ADC, INPUT_ANALOG);
  pinMode(I3V3_ADC, INPUT_ANALOG);
  pinMode(I5V_ADC, INPUT_ANALOG);
  pinMode(IVIN_ADC, INPUT_ANALOG);
  pinMode(CRANK_OUT, OUTPUT);
  pinMode(CAM_OUT, OUTPUT);

  while(Memory_Number < MAXSIZE_MEMORY){
    uint8_t temp = EEPROM.read(Memory_Number);
    if(temp == 255){EEPROM.write(Memory_Number, 0);}
    Memory_Number++;
  }

  Screen = EEPROM.read(0);
  if(Screen == 0){Screen = 1;}
  SELECT_CrankWheel = EEPROM.read(1);
  Cam_Speed = EEPROM.read(2);
  Cam_Custom = EEPROM.read(3);
  Speed_of_CAN = EEPROM.read(4);
  SIZE_CrankWheel_Custom = EEPROM.read(5);
  CamWheelCustom();
  CrankWheelSelector();

  InitCAN();

  InitISR();

  //Serial.begin(115200); //Debug
  //image d'introduction:
  display.begin(SSD1306_SWITCHCAPVCC);
  display.clearDisplay();
  display.drawBitmap(20, 0, BZO_bitmap, 89, 64, 1);
  display.display();
  delay(2000);
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.display();
  delay(500);
}
//************************************************************************************************************

//************************************************************************************************************
//Lecture boutons
void Read_Inputs_Push(){

  NormalPush1 = false;
  NormalPush2 = false;
  LongPush1 = false;
  LongPush2 = false;
  
  last_temp_binary1 = temp_binary1;
  last_temp_binary2 = temp_binary2;
  temp_binary1 = digitalRead(PUSH1);
  temp_binary2 = digitalRead(PUSH2);
  
  if(temp_binary1 == false && last_temp_binary1 == true && statePush1 == false && statePush2 == false){
    statePush1 = true;
    count_binary = millis();
  } 
 
  if(temp_binary2 == false && last_temp_binary2 == true && statePush1 == false && statePush2 == false){
    statePush2 = true;
    count_binary = millis();
  } 

  if(temp_binary1 == true && last_temp_binary1 == false && (millis() - count_binary <= NORMAL_PUSH_TIME) && statePush1 == true){statePush1 = false; count_binary = 0;}
  if(temp_binary1 == true && last_temp_binary1 == false && (millis() - count_binary > NORMAL_PUSH_TIME) && (millis() - count_binary < LONG_PUSH_TIME) && statePush1 == true){NormalPush1 = true; statePush1 = false; count_binary = 0;}
  if(temp_binary1 == false && last_temp_binary1 == false && (millis() - count_binary >= LONG_PUSH_TIME) && statePush1 == true){LongPush1 = true; statePush1 = false; count_binary = 0;}

  if(temp_binary2 == true && last_temp_binary2 == false && (millis() - count_binary <= NORMAL_PUSH_TIME) && statePush2 == true){statePush2 = false; count_binary = 0;}
  if(temp_binary2 == true && last_temp_binary2 == false && (millis() - count_binary > NORMAL_PUSH_TIME) && (millis() - count_binary < LONG_PUSH_TIME) && statePush2 == true){NormalPush2 = true; statePush2 = false; count_binary = 0;}
  if(temp_binary2 == false && last_temp_binary2 == false && (millis() - count_binary >= LONG_PUSH_TIME) && statePush2 == true){LongPush2 = true; statePush2 = false; count_binary = 0;}
}
//************************************************************************************************************

//************************************************************************************************************
//Lectures ADC (une par loop, avec filtrage par duplication)
void Read_Inputs_ADC(){

  if(NormalPush2 == true && Screen == 2){MAX_Mode = !MAX_Mode; NormalPush2 = false;}
  
  switch(Input_Number_ADC){
    
    case 0:
      analogReadResolution(12);
      POT_RPM_value1 = analogRead(POT_RPM);
    break;

    case 1:   //0-16000RPM
      POT_RPM_value2 = analogRead(POT_RPM);
      temp_ADC = (POT_RPM_value1 + POT_RPM_value2) >> 1; // /2
      RPM_value = (4090-temp_ADC) *4;
      if(RPM_value <= 100){RPM_value = 0; timCRANK->pause(); timCAM->pause(); Cycle_Flag = false; Crank_counter = 0; digitalWrite(CRANK_OUT, LOW); digitalWrite(CAM_OUT, LOW);}
      if(RPM_value > 16000){RPM_value = 16000;}
      if(RPM_value > 100 && RPM_value <= 16000){
        Interval_Value = 60000000 / ((uint32_t)RPM_value * SIZE_CrankWheel);    
        if(Cam_Speed == false && Cam_Custom == false){
          timCRANK->setOverflow(Interval_Value, MICROSEC_FORMAT);
          timCRANK->resume();
        }
        if(Cam_Speed == true && Cam_Custom == false){
          Interval_Value_Cam = Interval_Value*2;
          timCRANK->setOverflow(Interval_Value_Cam, MICROSEC_FORMAT);
          timCRANK->resume();
        }        
        if(Cam_Custom == true){
          Interval_Value_Cam = 60000000 / ((uint32_t)RPM_value * (SIZE_CamWheel >> 1));
          timCRANK->setOverflow(Interval_Value, MICROSEC_FORMAT);
          timCAM->setOverflow(Interval_Value_Cam, MICROSEC_FORMAT);
          timCRANK->resume();
          timCAM->resume();
        }
      } 
      analogReadResolution(10);
    break;

    case 2:
      POT_O2_value1 = analogRead(POT_O2);
    break;
    
    case 3:   //0-500centivolts
      POT_O2_value2 = analogRead(POT_O2);
      temp_ADC = (POT_O2_value1 + POT_O2_value2) >> 1;
      O2_value = map(temp_ADC, 0, 775, 0, 500);
    break;

    case 4:
      POT_TPS_value1 = analogRead(POT_TPS);
    break;
    
    case 5:   //0-500centivolts
      POT_TPS_value2 = analogRead(POT_TPS);
      temp_ADC = (POT_TPS_value1 + POT_TPS_value2) >> 1;
      TPS_value = map(temp_ADC, 0, 1023, 0, 500);
    break;

    case 6:
      POT_MAP_value1 = analogRead(POT_MAP);
    break;
    
    case 7:   //0-500centivolts
      POT_MAP_value2 = analogRead(POT_MAP);
      temp_ADC = (POT_MAP_value1 + POT_MAP_value2) >> 1;
      MAP_value = map(temp_ADC, 0, 775, 0, 500);
    break;

    case 8:
      VIN_ADC_value1 = analogRead(VIN_ADC);
    break;
    
    case 9:   //0-3600centivolts
      VIN_ADC_value2 = analogRead(VIN_ADC);
      temp_ADC = (VIN_ADC_value1 + VIN_ADC_value2) >> 1;
      VIN_value = map(temp_ADC, 0, 1023, 0, 360);
    break;
        
    case 10:
      I3V3_ADC_value1 = analogRead(I3V3_ADC);
    break;

    case 11:   //0-165dA
      I3V3_ADC_value2 = analogRead(I3V3_ADC);
      temp_ADC = (I3V3_ADC_value1 + I3V3_ADC_value2) >> 1;
      if(temp_ADC < 510){temp_ADC = 510;}
      if(MAX_Mode == false){I3V3_value = map(temp_ADC, 510, 1023, 0, 450);}
      if(MAX_Mode == true){
        temp_ADC = map(temp_ADC, 510, 1023, 0, 450);
        if(temp_ADC > I3V3_value){I3V3_value = temp_ADC;}
      }
    break;

    case 12:
      I5V_ADC_value1 = analogRead(I5V_ADC);
    break;

    case 13:   //0-165dA
      I5V_ADC_value2 = analogRead(I5V_ADC);
      temp_ADC = (I5V_ADC_value1 + I5V_ADC_value2) >> 1;
      if(temp_ADC < 510){temp_ADC = 510;}
      if(MAX_Mode == false){I5V_value = map(temp_ADC, 510, 1023, 0, 450);}
      if(MAX_Mode == true){
        temp_ADC = map(temp_ADC, 510, 1023, 0, 450);
        if(temp_ADC > I5V_value){I5V_value = temp_ADC;}
      }
    break;
   
    case 14:
      IVIN_ADC_value1 = analogRead(IVIN_ADC);
    break;
       
    case 15:   //0-165dA
      IVIN_ADC_value2 = analogRead(IVIN_ADC);
      temp_ADC = (IVIN_ADC_value1 + IVIN_ADC_value2) >> 1;
      if(temp_ADC < 510){temp_ADC = 510;}
      if(MAX_Mode == false){IVIN_value = map(temp_ADC, 510, 1023, 0, 450);}
      if(MAX_Mode == true){
        temp_ADC = map(temp_ADC, 510, 1023, 0, 450);
        if(temp_ADC > IVIN_value){IVIN_value = temp_ADC;}
      }
    break;

    default:
      Input_Number_ADC = 0;
    break;
  }
  Input_Number_ADC ++;
  if(Input_Number_ADC > 15){Input_Number_ADC = 0;}
}
//************************************************************************************************************

//************************************************************************************************************
//Lecture CAN
void CAN_Viewer(){

  if(NormalPush2 == true && Screen == 3){Can_Pause = !Can_Pause; NormalPush2 = false;}
  
  if (Can.read(CAN_RX_msg) && Can_Pause == false){
    memmove(line_CAN, line_CAN[1], 60);
    line_CAN[6][0] = lowByte(CAN_RX_msg.id);
    line_CAN[6][1] = highByte(CAN_RX_msg.id);    
    memcpy(&line_CAN[6][2], CAN_RX_msg.buf, 8);
  }
}
//************************************************************************************************************

//************************************************************************************************************
//écriture sur l'écran
void Write_Screen(){

  if(NormalPush1 == true){
    Screen++; 
    if(Screen >= 4){Screen = 1;} 
    NormalPush1 = false;
  }
  if(LongPush1 == true){Menu_Open = true; LongPush1 = false;}

  if((millis()%FREQ_SCREEN) == 0){
    
    display.clearDisplay();
  
    if(Screen == 1){
      display.setTextSize(2);
      display.setCursor(0,0);
      display.print("RPM:");
      display.setCursor(68,0);
      temp_ADC = RPM_value/10000;      
      if(temp_ADC > 0){display.print(temp_ADC);}
      display.setCursor(80,0);
      temp_ADC = (RPM_value %10000)/1000; 
      if(RPM_value > 999){display.print(temp_ADC);}
      display.setCursor(92,0);
      temp_ADC = (RPM_value %1000)/100; 
      if(RPM_value > 99){display.print(temp_ADC);}
      display.setCursor(104,0);
      temp_ADC = (RPM_value %100)/10; 
      if(RPM_value > 9){display.print(temp_ADC);}
      display.setCursor(116,0);
      temp_ADC = RPM_value %10; 
      display.print(temp_ADC);

      display.setCursor(0,16);
      display.print("TPS:");
      display.setCursor(70,16);
      temp_ADC = TPS_value/100;
      display.print(temp_ADC);
      display.setCursor(80,16);
      display.print(".");
      display.setCursor(92,16);
      temp_ADC = (TPS_value %100)/10; 
      display.print(temp_ADC);    
      display.setCursor(104,16);
      temp_ADC = TPS_value %10; 
      display.print(temp_ADC);
      display.setCursor(116,16);
      display.print("v");

      display.setCursor(0,32);
      display.print("MAP:");
      display.setCursor(70,32);
      temp_ADC = MAP_value/100;
      display.print(temp_ADC);
      display.setCursor(80,32);
      display.print(".");
      display.setCursor(92,32);
      temp_ADC = (MAP_value %100)/10; 
      display.print(temp_ADC);    
      display.setCursor(104,32);
      temp_ADC = MAP_value %10; 
      display.print(temp_ADC);
      display.setCursor(116,32);
      display.print("v");

      display.setCursor(0,48);
      display.print("O2:");
      display.setCursor(70,48);
      temp_ADC = O2_value/100;
      display.print(temp_ADC);
      display.setCursor(80,48);
      display.print(".");
      display.setCursor(92,48);
      temp_ADC = (O2_value %100)/10; 
      display.print(temp_ADC);    
      display.setCursor(104,48);
      temp_ADC = O2_value %10; 
      display.print(temp_ADC);
      display.setCursor(116,48);
      display.print("v");
    }
  
    if(Screen == 2){
      display.setTextSize(2);
      display.setCursor(0,0);
      display.print("Vin:");
      display.setCursor(70,0);
      temp_ADC = VIN_value/100;
      if(temp_ADC > 0){display.print(temp_ADC);}
      display.setCursor(82,0);   
      temp_ADC = (VIN_value %100)/10; 
      display.print(temp_ADC); 
      display.setCursor(92,0);
      display.print(".");
      display.setCursor(104,0);
      temp_ADC = VIN_value %10; 
      display.print(temp_ADC);
      display.setCursor(116,0);
      display.print("v");

      display.setCursor(0,16);
      if(MAX_Mode == false){display.print("Iin:");}
      if(MAX_Mode == true){display.print("<Iin: ");}
      display.setCursor(70,16);
      temp_ADC = IVIN_value/100;
      if(temp_ADC > 0){display.print(temp_ADC);}
      display.setCursor(82,16);   
      temp_ADC = (IVIN_value %100)/10; 
      display.print(temp_ADC); 
      display.setCursor(92,16);
      display.print(".");
      display.setCursor(104,16);
      temp_ADC = IVIN_value %10; 
      display.print(temp_ADC);
      display.setCursor(116,16);
      display.print("A");

      display.setCursor(0,32);
      if(MAX_Mode == false){display.print("I5v:");}
      if(MAX_Mode == true){display.print("<I5v:");}
      display.setCursor(70,32);
      temp_ADC = I5V_value/100;
      if(temp_ADC > 0){display.print(temp_ADC);}
      display.setCursor(82,32);   
      temp_ADC = (I5V_value %100)/10; 
      display.print(temp_ADC); 
      display.setCursor(92,32);
      display.print(".");
      display.setCursor(104,32);
      temp_ADC = I5V_value %10; 
      display.print(temp_ADC);
      display.setCursor(116,32);
      display.print("A");

      display.setCursor(0,48);
      if(MAX_Mode == false){display.print("I3v3:");}
      if(MAX_Mode == true){display.print("<I3v3:");}
      display.setCursor(70,48);
      temp_ADC = I3V3_value/100;
      if(temp_ADC > 0){display.print(temp_ADC);}
      display.setCursor(82,48);   
      temp_ADC = (I3V3_value %100)/10; 
      display.print(temp_ADC); 
      display.setCursor(92,48);
      display.print(".");
      display.setCursor(104,48);
      temp_ADC = I3V3_value %10; 
      display.print(temp_ADC);
      display.setCursor(116,48);
      display.print("A");
    }
  
    if(Screen == 3){
      display.setTextSize(1);
      if(Can_Pause == false){display.setCursor(30,0); display.print("Lecture CAN");}
      if(Can_Pause == true){display.setCursor(10,0); display.print("Lecture CAN (Pause)");}     
      display.setCursor(0,8); 
      uint16_t IDtemp = line_CAN[0][1] <<8 | line_CAN[0][0];
      if(IDtemp != 0){
        display.print(IDtemp, HEX);
        display.print("  ");
        if(line_CAN[0][2] < 16){display.print("0");}display.print(line_CAN[0][2], HEX);
        if(line_CAN[0][3] < 16){display.print("0");}display.print(line_CAN[0][3], HEX);
        if(line_CAN[0][4] < 16){display.print("0");}display.print(line_CAN[0][4], HEX);
        if(line_CAN[0][5] < 16){display.print("0");}display.print(line_CAN[0][5], HEX);
        if(line_CAN[0][6] < 16){display.print("0");}display.print(line_CAN[0][6], HEX);
        if(line_CAN[0][7] < 16){display.print("0");}display.print(line_CAN[0][7], HEX);
        if(line_CAN[0][8] < 16){display.print("0");}display.print(line_CAN[0][8], HEX);
        if(line_CAN[0][9] < 16){display.print("0");}display.print(line_CAN[0][9], HEX);
      }
      display.setCursor(0,16);     
      IDtemp = line_CAN[1][1] <<8 | line_CAN[1][0];
      if(IDtemp != 0){
        display.print(IDtemp, HEX);
        display.print("  ");
        if(line_CAN[1][2] < 16){display.print("0");}display.print(line_CAN[1][2], HEX);
        if(line_CAN[1][3] < 16){display.print("0");}display.print(line_CAN[1][3], HEX);
        if(line_CAN[1][4] < 16){display.print("0");}display.print(line_CAN[1][4], HEX);
        if(line_CAN[1][5] < 16){display.print("0");}display.print(line_CAN[1][5], HEX);
        if(line_CAN[1][6] < 16){display.print("0");}display.print(line_CAN[1][6], HEX);
        if(line_CAN[1][7] < 16){display.print("0");}display.print(line_CAN[1][7], HEX);
        if(line_CAN[1][8] < 16){display.print("0");}display.print(line_CAN[1][8], HEX);
        if(line_CAN[1][9] < 16){display.print("0");}display.print(line_CAN[1][9], HEX);
      }
      display.setCursor(0,24);
      IDtemp = line_CAN[2][1] <<8 | line_CAN[2][0];
      if(IDtemp != 0){
        display.print(IDtemp, HEX);
        display.print("  ");
        if(line_CAN[2][2] < 16){display.print("0");}display.print(line_CAN[2][2], HEX);
        if(line_CAN[2][3] < 16){display.print("0");}display.print(line_CAN[2][3], HEX);
        if(line_CAN[2][4] < 16){display.print("0");}display.print(line_CAN[2][4], HEX);
        if(line_CAN[2][5] < 16){display.print("0");}display.print(line_CAN[2][5], HEX);
        if(line_CAN[2][6] < 16){display.print("0");}display.print(line_CAN[2][6], HEX);
        if(line_CAN[2][7] < 16){display.print("0");}display.print(line_CAN[2][7], HEX);
        if(line_CAN[2][8] < 16){display.print("0");}display.print(line_CAN[2][8], HEX);
        if(line_CAN[2][9] < 16){display.print("0");}display.print(line_CAN[2][9], HEX);
      }
      display.setCursor(0,32);    
      IDtemp = line_CAN[3][1] <<8 | line_CAN[3][0];
      if(IDtemp != 0){
        display.print(IDtemp, HEX);
        display.print("  ");
        if(line_CAN[3][2] < 16){display.print("0");}display.print(line_CAN[3][2], HEX);
        if(line_CAN[3][3] < 16){display.print("0");}display.print(line_CAN[3][3], HEX);
        if(line_CAN[3][4] < 16){display.print("0");}display.print(line_CAN[3][4], HEX);
        if(line_CAN[3][5] < 16){display.print("0");}display.print(line_CAN[3][5], HEX);
        if(line_CAN[3][6] < 16){display.print("0");}display.print(line_CAN[3][6], HEX);
        if(line_CAN[3][7] < 16){display.print("0");}display.print(line_CAN[3][7], HEX);
        if(line_CAN[3][8] < 16){display.print("0");}display.print(line_CAN[3][8], HEX);
        if(line_CAN[3][9] < 16){display.print("0");}display.print(line_CAN[3][9], HEX);
      }
      display.setCursor(0,40);    
      IDtemp = line_CAN[4][1] <<8 | line_CAN[4][0];
      if(IDtemp != 0){
        display.print(IDtemp, HEX);
        display.print("  ");
        if(line_CAN[4][2] < 16){display.print("0");}display.print(line_CAN[4][2], HEX);
        if(line_CAN[4][3] < 16){display.print("0");}display.print(line_CAN[4][3], HEX);
        if(line_CAN[4][4] < 16){display.print("0");}display.print(line_CAN[4][4], HEX);
        if(line_CAN[4][5] < 16){display.print("0");}display.print(line_CAN[4][5], HEX);
        if(line_CAN[4][6] < 16){display.print("0");}display.print(line_CAN[4][6], HEX);
        if(line_CAN[4][7] < 16){display.print("0");}display.print(line_CAN[4][7], HEX);
        if(line_CAN[4][8] < 16){display.print("0");}display.print(line_CAN[4][8], HEX);
        if(line_CAN[4][9] < 16){display.print("0");}display.print(line_CAN[4][9], HEX);
      }
      display.setCursor(0,48);     
      IDtemp = line_CAN[5][1] <<8 | line_CAN[5][0];
      if(IDtemp != 0){
        display.print(IDtemp, HEX);
        display.print("  ");
        if(line_CAN[5][2] < 16){display.print("0");}display.print(line_CAN[5][2], HEX);
        if(line_CAN[5][3] < 16){display.print("0");}display.print(line_CAN[5][3], HEX);
        if(line_CAN[5][4] < 16){display.print("0");}display.print(line_CAN[5][4], HEX);
        if(line_CAN[5][5] < 16){display.print("0");}display.print(line_CAN[5][5], HEX);
        if(line_CAN[5][6] < 16){display.print("0");}display.print(line_CAN[5][6], HEX);
        if(line_CAN[5][7] < 16){display.print("0");}display.print(line_CAN[5][7], HEX);
        if(line_CAN[5][8] < 16){display.print("0");}display.print(line_CAN[5][8], HEX);
        if(line_CAN[5][9] < 16){display.print("0");}display.print(line_CAN[5][9], HEX);
      }
      display.setCursor(0,56);     
      IDtemp = line_CAN[6][1] <<8 | line_CAN[6][0];
      if(IDtemp != 0){
        display.print(IDtemp, HEX);
        display.print("  ");
        if(line_CAN[6][2] < 16){display.print("0");}display.print(line_CAN[6][2], HEX);
        if(line_CAN[6][3] < 16){display.print("0");}display.print(line_CAN[6][3], HEX);
        if(line_CAN[6][4] < 16){display.print("0");}display.print(line_CAN[6][4], HEX);
        if(line_CAN[6][5] < 16){display.print("0");}display.print(line_CAN[6][5], HEX);
        if(line_CAN[6][6] < 16){display.print("0");}display.print(line_CAN[6][6], HEX);
        if(line_CAN[6][7] < 16){display.print("0");}display.print(line_CAN[6][7], HEX);
        if(line_CAN[6][8] < 16){display.print("0");}display.print(line_CAN[6][8], HEX);
        if(line_CAN[6][9] < 16){display.print("0");}display.print(line_CAN[6][9], HEX);
      }
    }
  display.display();
  }
}
//************************************************************************************************************

//************************************************************************************************************
void Setup_Menu(){
  
  if(LongPush1 == true){display.clearDisplay(); display.display(); Menu_Open = false; LongPush1 = false; Menu_Selector = 0; CamWheel_Generator = false; CrankWheel_Generator = false;}

  display.clearDisplay();
  
  if(CamWheel_Generator == false && CrankWheel_Generator == false){
    
    if(NormalPush1 == true){
      Menu_Selector++; NormalPush1 = false; 
      if(Menu_Selector >= 5){Menu_Selector = 0;}
    }

    display.setTextSize(1);       
    //display.setTextColor(SSD1306_WHITE);   
    display.setCursor(20,0);
    display.print("Menu Reglages");
    
    display.setCursor(0,12);
    display.print("Def Page:     ");
    display.print(Screen);
    if(Menu_Selector == 0){
      display.setCursor(120,12);
      display.print("-");
      if(NormalPush2 == true){Screen++; NormalPush2 = false; if(Screen >= 4){Screen = 1;} EEPROM.write(0, Screen);}
    }

    display.setCursor(0,22);
    if(Speed_of_CAN == 0){Speed_of_CAN = 1;}
    if(Speed_of_CAN == 1){display.print("CAN Baudrate: 250k");}
    if(Speed_of_CAN == 2){display.print("CAN Baudrate: 500k");}
    if(Speed_of_CAN == 3){display.print("CAN Baudrate: 1000k");}
    if(Menu_Selector == 1){
      display.setCursor(120,22);
      display.print("-");
      if(NormalPush2 == true){Speed_of_CAN++; NormalPush2 = false; if(Speed_of_CAN >= 4){Speed_of_CAN = 1;} EEPROM.write(4, Speed_of_CAN); InitCAN();}
    }

    display.setCursor(0,32);
    Wheelenum();
    if(Menu_Selector == 2){
      display.setCursor(120,32);
      display.print("-");     
      if(NormalPush2 == true){SELECT_CrankWheel++; NormalPush2 = false; if(SELECT_CrankWheel >= NB_CRANKWHEEL){SELECT_CrankWheel = 0;} EEPROM.write(1, SELECT_CrankWheel); CrankWheelSelector();}
      if(LongPush2 == true && SELECT_CrankWheel == SELECT_Crank_Custom){CrankWheel_Generator = true; LongPush2 = false; Menu_Selector = 0;}
    }

    display.setCursor(0,42);
    if(Cam_Speed == false){display.print("sur vilebrequin");}
    if(Cam_Speed == true){display.print("sur arbre a cames");}
    if(Menu_Selector == 3){
      display.setCursor(120,42);
      display.print("-");
      if(NormalPush2 == true){Cam_Speed = !Cam_Speed; NormalPush2 = false; EEPROM.write(2, Cam_Speed);}
    }

    display.setCursor(0,52);
    if(Cam_Custom == false){display.print("CamWheel 1 dent");}
    if(Cam_Custom == true){display.print("CamWheel Custom");}
    if(Menu_Selector == 4 && Cam_Speed == false){
      display.setCursor(120,52);
      display.print("-");
      if(NormalPush2 == true){Cam_Custom = !Cam_Custom; NormalPush2 = false; EEPROM.write(3, Cam_Custom);}
      if(LongPush2 == true && Cam_Custom == true){CamWheel_Generator = true; LongPush2 = false; Menu_Selector = 0;}
    }
    if(Menu_Selector == 4 && Cam_Speed == true){
      display.setCursor(120,52);
      display.print("X");
    }
  }

  if(CamWheel_Generator == true){
    
    if(NormalPush1 == true){
      Menu_Selector++; NormalPush1 = false; 
      if(Menu_Selector >= 72){Menu_Selector = 0;} 
      Page = Menu_Selector/14;
    }

    display.setTextSize(1);             // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE);        // Draw white text
    display.setCursor(0,0);
    display.print("CamWheel Generator 36");
    if(Page == 0){display.setCursor(0,20); display.print("1  2  3  4  5  6  7");}
    if(Page == 1){display.setCursor(51,12); display.print("11"); display.setCursor(88,12); display.print("13");
                  display.setCursor(0,20); display.print("8  9"); display.setCursor(33,20); display.print("10"); display.setCursor(69,20); display.print("12"); display.setCursor(106,20); display.print("14");}
    if(Page >= 2 && Page < 5){display.setCursor(0,12); display.print(Page*7+1); display.setCursor(34,12); display.print(Page*7+3); display.setCursor(70,12); display.print(Page*7+5); display.setCursor(106,12); display.print(Page*7+7);
                  display.setCursor(16,20); display.print(Page*7+2); display.setCursor(52,20); display.print(Page*7+4); display.setCursor(88,20); display.print(Page*7+6);}
    if(Page == 5){display.setCursor(0,12); display.print("36");}
    display.setCursor(((Menu_Selector%14)*9),30);
    display.print("|");
    if(Menu_Selector == 20 || Menu_Selector == 24 || Menu_Selector == 28 || Menu_Selector == 32 || Menu_Selector == 36 || Menu_Selector == 40 || Menu_Selector == 42 || Menu_Selector == 46 || Menu_Selector == 50
       || Menu_Selector == 54 || Menu_Selector == 56 || Menu_Selector == 60 || Menu_Selector == 64 || Menu_Selector == 68 || Menu_Selector == 70){
      display.setCursor(((Menu_Selector%14)*9),22); display.print("|");
    }
    if(NormalPush2 == true){DRAWING_CamWheel[Menu_Selector] = !DRAWING_CamWheel[Menu_Selector]; EEPROM.write((CAMOFFSET_MEMORY + Menu_Selector), DRAWING_CamWheel[Menu_Selector]); NormalPush2 = false; CamWheelCustom();}
    Draw_Cursor = 0;
    if(Page < 5){
      while(Draw_Cursor < 14){
        if(DRAWING_CamWheel[Draw_Cursor+(Page*14)] == false){
          display.drawLine((Draw_Cursor*9+1), 64, (Draw_Cursor*9+1), 54, SSD1306_WHITE);
          display.drawLine((Draw_Cursor*9+2), 64, (Draw_Cursor*9+2), 54, SSD1306_WHITE);
          display.drawLine((Draw_Cursor*9+3), 64, (Draw_Cursor*9+3), 54, SSD1306_WHITE);
        }
        if(DRAWING_CamWheel[Draw_Cursor+(Page*14)] == true){
          display.drawLine((Draw_Cursor*9+1), 64, (Draw_Cursor*9+1), 40, SSD1306_WHITE);
          display.drawLine((Draw_Cursor*9+2), 64, (Draw_Cursor*9+2), 40, SSD1306_WHITE);
          display.drawLine((Draw_Cursor*9+3), 64, (Draw_Cursor*9+3), 40, SSD1306_WHITE);
        }
      Draw_Cursor ++;
      }
    }
    if(Page == 5){
      while(Draw_Cursor < 2){
        if(DRAWING_CamWheel[Draw_Cursor+(Page*14)] == false){
          display.drawLine((Draw_Cursor*9+1), 64, (Draw_Cursor*9+1), 54, SSD1306_WHITE);
          display.drawLine((Draw_Cursor*9+2), 64, (Draw_Cursor*9+2), 54, SSD1306_WHITE);
          display.drawLine((Draw_Cursor*9+3), 64, (Draw_Cursor*9+3), 54, SSD1306_WHITE);
        }
        if(DRAWING_CamWheel[Draw_Cursor+(Page*14)] == true){
          display.drawLine((Draw_Cursor*9+1), 64, (Draw_Cursor*9+1), 40, SSD1306_WHITE);
          display.drawLine((Draw_Cursor*9+2), 64, (Draw_Cursor*9+2), 40, SSD1306_WHITE);
          display.drawLine((Draw_Cursor*9+3), 64, (Draw_Cursor*9+3), 40, SSD1306_WHITE);
        }
      Draw_Cursor ++;
      }
    }
  }

  if(CrankWheel_Generator == true){
    if(NormalPush1 == true){
      Menu_Selector++; 
      NormalPush1 = false; 
      if(Menu_Selector-1 >= SIZE_CrankWheel_Custom){Menu_Selector = 0;} 
      Page = (Menu_Selector-1)/14;
      Page_Finale = SIZE_CrankWheel_Custom/14;
      NB_FinalPage_Value = (SIZE_CrankWheel_Custom%14)/2;
    }
    
    display.setTextSize(1);             // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE);        // Draw white text
    display.setCursor(0,0);
    display.print("Crank Generator   "); display.print(SIZE_CrankWheel_Custom/2);
    if(Menu_Selector == 0){
      display.drawLine(107, 9, 125, 9, SSD1306_WHITE);
      if(NormalPush2 == true){NormalPush2 = false; SIZE_CrankWheel_Custom += 2; if(SIZE_CrankWheel_Custom < 28){SIZE_CrankWheel_Custom = 28;} EEPROM.write(5, SIZE_CrankWheel_Custom); CrankWheelSelector();}
    }
    if(Page == 0){display.setCursor(0,20); display.print("1  2  3  4  5  6  7");}
    if(Page == 1){display.setCursor(51,12); display.print("11"); display.setCursor(88,12); display.print("13");
                  display.setCursor(0,20); display.print("8  9"); display.setCursor(33,20); display.print("10"); display.setCursor(69,20); display.print("12"); display.setCursor(106,20); display.print("14");}
    if(Page >= 2 && (Page < Page_Finale || NB_FinalPage_Value == 0)){display.setCursor(0,12); display.print(Page*7+1); display.setCursor(34,12); display.print(Page*7+3); display.setCursor(70,12); display.print(Page*7+5); display.setCursor(106,12); display.print(Page*7+7);
                  display.setCursor(16,20); display.print(Page*7+2); display.setCursor(52,20); display.print(Page*7+4); display.setCursor(88,20); display.print(Page*7+6);}
    if(Page == Page_Finale && NB_FinalPage_Value > 0){
      display.setCursor(0,12); display.print(Page*7+1); 
      if(NB_FinalPage_Value >= 2){display.setCursor(16,20); display.print(Page*7+2);}
      if(NB_FinalPage_Value >= 3){display.setCursor(34,12); display.print(Page*7+3);} 
      if(NB_FinalPage_Value >= 4){display.setCursor(52,20); display.print(Page*7+4);}
      if(NB_FinalPage_Value >= 5){display.setCursor(70,12); display.print(Page*7+5);}
      if(NB_FinalPage_Value == 6){display.setCursor(88,20); display.print(Page*7+6);} 
    }
    display.setCursor((((Menu_Selector-1)%14)*9),30); display.print("|");
    if(Menu_Selector-1 == 20 || Menu_Selector-1 == 24 || (Page >= 2 && ((Menu_Selector/2 == Page*7 && Menu_Selector%2 == 1) || (Menu_Selector/2 == Page*7+2 && Menu_Selector%2 == 1) 
       || (Menu_Selector/2 == Page*7+4 && Menu_Selector%2 == 1) || (Menu_Selector/2 == Page*7+6 && Menu_Selector%2 == 1)))){
      display.setCursor((((Menu_Selector-1)%14)*9),22); display.print("|"); 
    }
    if(NormalPush2 == true){DRAWING_CrankWheel[Menu_Selector-1] = !DRAWING_CrankWheel[Menu_Selector-1]; EEPROM.write((CRANKOFFSET_MEMORY + Menu_Selector-1), DRAWING_CrankWheel[Menu_Selector-1]); NormalPush2 = false; CrankWheelSelector();}
    Draw_Cursor = 0;
    if(Page < Page_Finale || NB_FinalPage_Value == 0){
      while(Draw_Cursor < 14){
        if(DRAWING_CrankWheel[Draw_Cursor+(Page*14)] == false){
          display.drawLine((Draw_Cursor*9+1), 64, (Draw_Cursor*9+1), 54, SSD1306_WHITE);
          display.drawLine((Draw_Cursor*9+2), 64, (Draw_Cursor*9+2), 54, SSD1306_WHITE);
          display.drawLine((Draw_Cursor*9+3), 64, (Draw_Cursor*9+3), 54, SSD1306_WHITE);
        }
        if(DRAWING_CrankWheel[Draw_Cursor+(Page*14)] == true){
          display.drawLine((Draw_Cursor*9+1), 64, (Draw_Cursor*9+1), 40, SSD1306_WHITE);
          display.drawLine((Draw_Cursor*9+2), 64, (Draw_Cursor*9+2), 40, SSD1306_WHITE);
          display.drawLine((Draw_Cursor*9+3), 64, (Draw_Cursor*9+3), 40, SSD1306_WHITE);
        }
      Draw_Cursor ++;
      }
    }
    if(Page == Page_Finale && NB_FinalPage_Value > 0){
      while(Draw_Cursor < NB_FinalPage_Value*2){
        if(DRAWING_CrankWheel[Draw_Cursor+(Page*14)] == false){
          display.drawLine((Draw_Cursor*9+1), 64, (Draw_Cursor*9+1), 54, SSD1306_WHITE);
          display.drawLine((Draw_Cursor*9+2), 64, (Draw_Cursor*9+2), 54, SSD1306_WHITE);
          display.drawLine((Draw_Cursor*9+3), 64, (Draw_Cursor*9+3), 54, SSD1306_WHITE);
        }
        if(DRAWING_CrankWheel[Draw_Cursor+(Page*14)] == true){
          display.drawLine((Draw_Cursor*9+1), 64, (Draw_Cursor*9+1), 40, SSD1306_WHITE);
          display.drawLine((Draw_Cursor*9+2), 64, (Draw_Cursor*9+2), 40, SSD1306_WHITE);
          display.drawLine((Draw_Cursor*9+3), 64, (Draw_Cursor*9+3), 40, SSD1306_WHITE);
        }
      Draw_Cursor ++;
      }
    }                      
  }
  display.display();
}
//************************************************************************************************************

//************************************************************************************************************
//Boucle principale
void loop() {

  Read_Inputs_Push();
  
  if(Menu_Open == false){
    
    Read_Inputs_ADC();
    if(Screen == 3){CAN_Viewer();}
    Write_Screen();
  }
  
  if(Menu_Open == true){
    
  Setup_Menu();
  }
}
//************************************************************************************************************
//END
