/***********************************************************************************************************
 * DigitalReadings.h - lecture des 16 entrées logiques
 *
 * Chaque entrée a deux broches : pinDH (entrée côté +12V) et pinDL (entrée côté masse).
 * configPage1.DigitalLevel1/2 (1 bit par entrée) choisit laquelle est lue - lu au démarrage (InitDigitals).
 * Filtre : 3 lectures successives identiques avant de changer l'état.
 * Résultat : currentStatus.DigitalStatus1 (entrées 1-8) et DigitalStatus2 (9-16), 1 = entrée active.
 **********************************************************************************************************/

const uint32_t pinDH[17] = {NC,
                            PA8,  PC9,  PC8,  PC7,  PC6,  PD15, PD14, PD13,
                          // 1     2     3     4     5     6     7     8
                          
                            PB12, PB11, PB10, PE15, PE14, PE13, PE12, PE11};
                          // 9     10    11    12    13    14    15    16
                            
const uint32_t pinDL[17] = {NC,
                            PB13, PB14, PB15, PD8,  PD9,  PD10, PD11, PD12,
                          // 1     2     3     4     5     6     7     8
                            
                            PC4,  PC5,  PB0,  PB1,  PE7,  PE8,  PE9,  PE10};
                          // 9     10    11    12    13    14    15    16

bool DStates[17][4]; //17 lignes, 4 colonnes
uint8_t counterDCH = 0; //0 NC, 1-16 channels, la broche actuelle
uint8_t counterBIN = 0;
bool counterOCT = false; // false = configPage1.DigitalLevel1, true = configPage1.DigitalLevel2
uint8_t counterDPhase = 0;

void InitDigitals(){

counterDCH = 1;
counterBIN = 0;
counterOCT = false;

  while(counterDCH < 17){
    if (counterOCT == false){
      if (BIT_CHECK(configPage1.DigitalLevel1, counterBIN) == true){pinMode(pinDH[counterDCH], INPUT_PULLUP); pinMode(pinDL[counterDCH], OUTPUT); digitalWrite(pinDL[counterDCH], LOW);}
      if (BIT_CHECK(configPage1.DigitalLevel1, counterBIN) == false){pinMode(pinDH[counterDCH], INPUT); pinMode(pinDL[counterDCH], INPUT_PULLUP);}
    }
    if (counterOCT == true){
      if (BIT_CHECK(configPage1.DigitalLevel2, counterBIN) == true){pinMode(pinDH[counterDCH], INPUT_PULLUP); pinMode(pinDL[counterDCH], OUTPUT); digitalWrite(pinDL[counterDCH], LOW);}
      if (BIT_CHECK(configPage1.DigitalLevel2, counterBIN) == false){pinMode(pinDH[counterDCH], INPUT); pinMode(pinDL[counterDCH], INPUT_PULLUP);}
    }
    counterDCH ++;
    counterBIN ++;
    if(counterBIN > 7){counterBIN = 0; counterOCT = true;}
  }
}

void ReadDigitals(){

counterDCH = 1;
counterBIN = 0;
counterOCT = false; 

  if (counterDPhase < 3){
    while(counterDCH < 17){
      if (counterOCT == false){
        if (BIT_CHECK(configPage1.DigitalLevel1, counterBIN) == true){DStates[counterDCH][counterDPhase] = !digitalRead(pinDH[counterDCH]);}
        if (BIT_CHECK(configPage1.DigitalLevel1, counterBIN) == false){DStates[counterDCH][counterDPhase] = !digitalRead(pinDL[counterDCH]);}
      }
      if (counterOCT == true){
        if (BIT_CHECK(configPage1.DigitalLevel2, counterBIN) == true){DStates[counterDCH][counterDPhase] = !digitalRead(pinDH[counterDCH]);}
        if (BIT_CHECK(configPage1.DigitalLevel2, counterBIN) == false){DStates[counterDCH][counterDPhase] = !digitalRead(pinDL[counterDCH]);}
      } 
      counterDCH ++;
      counterBIN ++;
      if(counterDCH > 16){counterDPhase ++;}
      if(counterBIN > 7){counterBIN = 0; counterOCT = !counterOCT;}
    }
  }
  if (counterDPhase == 3){
    counterDCH = 1;
    counterBIN = 0;
    counterOCT = false;
    
    while(counterDCH < 17){
      if (DStates[counterDCH][0] == DStates[counterDCH][1] && DStates[counterDCH][1] == DStates[counterDCH][2]){DStates[counterDCH][3] = DStates[counterDCH][2];}
      if (counterOCT == false){
        if (DStates[counterDCH][3] == true){BIT_SET(currentStatus.DigitalStatus1, counterBIN);}
        if (DStates[counterDCH][3] == false){BIT_CLEAR(currentStatus.DigitalStatus1, counterBIN);}
      }
      if (counterOCT == true){
        if (DStates[counterDCH][3] == true){BIT_SET(currentStatus.DigitalStatus2, counterBIN);}
        if (DStates[counterDCH][3] == false){BIT_CLEAR(currentStatus.DigitalStatus2, counterBIN);}
      }      
      counterDCH ++;
      counterBIN ++;
      if(counterBIN > 7){counterBIN = 0; counterOCT = !counterOCT;}
    }
  counterDPhase = 0;
  }
}
