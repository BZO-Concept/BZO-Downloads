/***********************************************************************************************************
 * Security.h - protections de la carte
 *
 *   1. Seuils ABSOLUS fixes (MAX_...) : survie de la carte, volontairement non réglables dans TS.
 *      Dépassement (après filtre FRAMES_...) et TS non connecté : mémoire de l'erreur en EEPROM,
 *      voies coupées, 16 LEDs rouges allumées DISTRESS_DELAY ms, puis MCU_OK à LOW -> le superviseur d'alim coupe
 *      toute la logique 2 s et relance. TS connecté : pas de coupure (diagnostic).
 *   2. FUSIBLES par voie (réglables, configPage1.Fuse_chN / FuseDelay / FuseRearm) : voie coupée,
 *      LED rouge allumée, bit dans FuseStatus1/2, réarmement automatique ou par bouton TS.
 *   3. LED STATUS D1 : rapide = proche d'un seuil, double flash = TS connecté, lente = normal.
 *
 * InitSecurity() doit rester la PREMIÈRE ligne de setup() : le superviseur d'alim ne laisse que 2 s pour monter MCU_OK.
 **********************************************************************************************************/

#define MCU_OK           PA13    // ligne de confirmation vers le superviseur d'alim (HIGH = tout va bien)
#define LED_STATUS       PB2     // LED D1

// LEDs rouges des voies (HIGH = rouge allumée, via IC14/IC15). Attention : les drivers inversent l'ordre
// par groupe de 8, la ligne schéma LED_R_CH1 allume la LED de la voie 8, LED_R_CH9 celle de la voie 16, etc.
// (le vert est piloté directement par la sortie du driver)
#define LED_R_CH1   PC15    // schéma : LED_R_CH8
#define LED_R_CH2   PC14    // schéma : LED_R_CH7
#define LED_R_CH3   PC13    // schéma : LED_R_CH6
#define LED_R_CH4   PE6     // schéma : LED_R_CH5
#define LED_R_CH5   PE2     // schéma : LED_R_CH4
#define LED_R_CH6   PE3     // schéma : LED_R_CH3
#define LED_R_CH7   PE4     // schéma : LED_R_CH2
#define LED_R_CH8   PE5     // schéma : LED_R_CH1
#define LED_R_CH9   PE1     // schéma : LED_R_CH16
#define LED_R_CH10  PE0     // schéma : LED_R_CH15
#define LED_R_CH11  PB9     // schéma : LED_R_CH14
#define LED_R_CH12  PB8     // schéma : LED_R_CH13
#define LED_R_CH13  PB7     // schéma : LED_R_CH12
#define LED_R_CH14  PB6     // schéma : LED_R_CH11
#define LED_R_CH15  PB5     // schéma : LED_R_CH10
#define LED_R_CH16  PB4     // schéma : LED_R_CH9

// Seuils de coupure FIXES (survie de la carte, pas reglables dans TS)
#define MAX_CURRENT_1_12   100    // x0.1 A  -> 10,0 A  (voies 1-12)
#define MAX_CURRENT_13_16  500    // x0.1 A  -> 50,0 A  (voies 13-16)
#define MAX_BOARD_TEMP   120     // °C
#define MAX_BATTERY      180     // x0.1 V  -> 18,0 V
#define FRAMES_CURRENT   10      // trames ADC tolerees au-dessus du seuil avant coupure (x50 ms)
#define FRAMES_SLOW      20      // pareil pour temperature et tension
#define SAFETY_WARNING   80      // % du seuil a partir duquel on est "proche"
#define DISTRESS_DELAY   500     // ms : toutes les LEDs rouges allumées avant de couper MCU_OK

#define ERR_ADDR         (EEPROM.length() - 3)   // derniere erreur : 3 octets en fin d'EEPROM, hors zone TS

uint8_t countCurrent = 0;
uint8_t countSlow = 0;
uint32_t fuseTimer[17];          // fusibles : instant où la voie N a dépassé son seuil (0 = sous le seuil)
uint32_t fuseTrip[17];           // fusibles : instant où la voie N a été coupée
bool fuseResetRequest = false;   // bouton TS "réarmer les fusibles"

// Memorise la derniere erreur (une seule ecriture flash par nouvelle erreur)
void SaveError(uint8_t code, uint16_t value){
  if (code == currentStatus.LastError) {return;}      // deja memorisee : pas de nouvelle ecriture
  currentStatus.LastError = code;
  currentStatus.LastErrorValue = value;
  eeprom_buffer_fill();
  eeprom_buffered_write_byte(ERR_ADDR, code);
  eeprom_buffered_write_byte(ERR_ADDR + 1, lowByte(value));
  eeprom_buffered_write_byte(ERR_ADDR + 2, highByte(value));
  eeprom_buffer_flush();
}

void InitSecurity(){
  pinMode(MCU_OK, OUTPUT);
  digitalWrite(MCU_OK, HIGH);
  pinMode(LED_STATUS, OUTPUT);
  digitalWrite(LED_STATUS, LOW);

  pinMode(LED_R_CH1, OUTPUT);   digitalWrite(LED_R_CH1, LOW);
  pinMode(LED_R_CH2, OUTPUT);   digitalWrite(LED_R_CH2, LOW);
  pinMode(LED_R_CH3, OUTPUT);   digitalWrite(LED_R_CH3, LOW);
  pinMode(LED_R_CH4, OUTPUT);   digitalWrite(LED_R_CH4, LOW);
  pinMode(LED_R_CH5, OUTPUT);   digitalWrite(LED_R_CH5, LOW);
  pinMode(LED_R_CH6, OUTPUT);   digitalWrite(LED_R_CH6, LOW);
  pinMode(LED_R_CH7, OUTPUT);   digitalWrite(LED_R_CH7, LOW);
  pinMode(LED_R_CH8, OUTPUT);   digitalWrite(LED_R_CH8, LOW);
  pinMode(LED_R_CH9, OUTPUT);   digitalWrite(LED_R_CH9, LOW);
  pinMode(LED_R_CH10, OUTPUT);  digitalWrite(LED_R_CH10, LOW);
  pinMode(LED_R_CH11, OUTPUT);  digitalWrite(LED_R_CH11, LOW);
  pinMode(LED_R_CH12, OUTPUT);  digitalWrite(LED_R_CH12, LOW);
  pinMode(LED_R_CH13, OUTPUT);  digitalWrite(LED_R_CH13, LOW);
  pinMode(LED_R_CH14, OUTPUT);  digitalWrite(LED_R_CH14, LOW);
  pinMode(LED_R_CH15, OUTPUT);  digitalWrite(LED_R_CH15, LOW);
  pinMode(LED_R_CH16, OUTPUT);  digitalWrite(LED_R_CH16, LOW);

  eeprom_buffer_fill();
  currentStatus.LastError      = eeprom_buffered_read_byte(ERR_ADDR);
  currentStatus.LastErrorValue = eeprom_buffered_read_byte(ERR_ADDR + 1) | (eeprom_buffered_read_byte(ERR_ADDR + 2) << 8);
  if (currentStatus.LastError == 0xFF) {currentStatus.LastError = 0; currentStatus.LastErrorValue = 0;}   // flash vierge
}

// Signal de détresse : les 16 LEDs rouges allumées
void AllRedON(){
  digitalWrite(LED_R_CH1, HIGH);
  digitalWrite(LED_R_CH2, HIGH);
  digitalWrite(LED_R_CH3, HIGH);
  digitalWrite(LED_R_CH4, HIGH);
  digitalWrite(LED_R_CH5, HIGH);
  digitalWrite(LED_R_CH6, HIGH);
  digitalWrite(LED_R_CH7, HIGH);
  digitalWrite(LED_R_CH8, HIGH);
  digitalWrite(LED_R_CH9, HIGH);
  digitalWrite(LED_R_CH10, HIGH);
  digitalWrite(LED_R_CH11, HIGH);
  digitalWrite(LED_R_CH12, HIGH);
  digitalWrite(LED_R_CH13, HIGH);
  digitalWrite(LED_R_CH14, HIGH);
  digitalWrite(LED_R_CH15, HIGH);
  digitalWrite(LED_R_CH16, HIGH);
}

// Fusible d'une voie : vrai si le courant est resté au-dessus du seuil pendant FuseDelay
// (seuil 0 = fusible désactivé)
bool Fuse(uint16_t current, uint16_t seuil, uint8_t voie){
  if (seuil == 0 || current < seuil) {fuseTimer[voie] = 0; return false;}
  if (fuseTimer[voie] == 0) {fuseTimer[voie] = millis(); if (fuseTimer[voie] == 0) {fuseTimer[voie] = 1;}}   // départ du chrono (jamais 0)
  return (millis() - fuseTimer[voie]) >= (uint32_t)configPage1.FuseDelay * 100UL;
}

// Gère le fusible d'une voie :
//  - pas coupé : si le courant dépasse Fuse_chN (en A) pendant FuseDelay -> voie coupée, LED rouge, bit à 1
//  - coupé     : au bout de FuseRearm (ou bouton TS) -> bit à 0, LED rouge éteinte, la voie peut repartir
//                (en mode test, la voie se rallume si elle était demandée ON ; sinon c'est la fonction mère)
void FuseChannel(uint16_t current, uint8_t seuilA, uint8_t voie, uint32_t outPin, uint32_t ledPin,
                 uint8_t &fuseByte, uint8_t &manByte, uint8_t rang){
  if (BIT_CHECK(fuseByte, rang) == false){
    if (Fuse(current, (uint16_t)seuilA * 10, voie)){
      digitalWrite(outPin, LOW);
      digitalWrite(ledPin, HIGH);
      BIT_SET(fuseByte, rang);
      fuseTimer[voie] = 0;
      fuseTrip[voie] = millis();
    }
  }
  else if (fuseResetRequest == true || (configPage1.FuseRearm != 0 && (millis() - fuseTrip[voie]) >= (uint32_t)configPage1.FuseRearm * 100UL)){
    BIT_CLEAR(fuseByte, rang);
    digitalWrite(ledPin, LOW);
    if (currentStatus.TestMode == 1 && BIT_CHECK(manByte, rang) == true) {digitalWrite(outPin, HIGH);}
  }
}

// Teste les 16 fusibles (une fois par trame ADC)
void CheckFuses(){
  FuseChannel(currentStatus.Current_ch1,  configPage1.Fuse_ch1,  1,  OUT_CH1,  LED_R_CH1,  currentStatus.FuseStatus1, ManualStatus1, 0);
  FuseChannel(currentStatus.Current_ch2,  configPage1.Fuse_ch2,  2,  OUT_CH2,  LED_R_CH2,  currentStatus.FuseStatus1, ManualStatus1, 1);
  FuseChannel(currentStatus.Current_ch3,  configPage1.Fuse_ch3,  3,  OUT_CH3,  LED_R_CH3,  currentStatus.FuseStatus1, ManualStatus1, 2);
  FuseChannel(currentStatus.Current_ch4,  configPage1.Fuse_ch4,  4,  OUT_CH4,  LED_R_CH4,  currentStatus.FuseStatus1, ManualStatus1, 3);
  FuseChannel(currentStatus.Current_ch5,  configPage1.Fuse_ch5,  5,  OUT_CH5,  LED_R_CH5,  currentStatus.FuseStatus1, ManualStatus1, 4);
  FuseChannel(currentStatus.Current_ch6,  configPage1.Fuse_ch6,  6,  OUT_CH6,  LED_R_CH6,  currentStatus.FuseStatus1, ManualStatus1, 5);
  FuseChannel(currentStatus.Current_ch7,  configPage1.Fuse_ch7,  7,  OUT_CH7,  LED_R_CH7,  currentStatus.FuseStatus1, ManualStatus1, 6);
  FuseChannel(currentStatus.Current_ch8,  configPage1.Fuse_ch8,  8,  OUT_CH8,  LED_R_CH8,  currentStatus.FuseStatus1, ManualStatus1, 7);
  FuseChannel(currentStatus.Current_ch9,  configPage1.Fuse_ch9,  9,  OUT_CH9,  LED_R_CH9,  currentStatus.FuseStatus2, ManualStatus2, 0);
  FuseChannel(currentStatus.Current_ch10, configPage1.Fuse_ch10, 10, OUT_CH10, LED_R_CH10, currentStatus.FuseStatus2, ManualStatus2, 1);
  FuseChannel(currentStatus.Current_ch11, configPage1.Fuse_ch11, 11, OUT_CH11, LED_R_CH11, currentStatus.FuseStatus2, ManualStatus2, 2);
  FuseChannel(currentStatus.Current_ch12, configPage1.Fuse_ch12, 12, OUT_CH12, LED_R_CH12, currentStatus.FuseStatus2, ManualStatus2, 3);
  FuseChannel(currentStatus.Current_ch13, configPage1.Fuse_ch13, 13, OUT_CH13, LED_R_CH13, currentStatus.FuseStatus2, ManualStatus2, 4);
  FuseChannel(currentStatus.Current_ch14, configPage1.Fuse_ch14, 14, OUT_CH14, LED_R_CH14, currentStatus.FuseStatus2, ManualStatus2, 5);
  FuseChannel(currentStatus.Current_ch15, configPage1.Fuse_ch15, 15, OUT_CH15, LED_R_CH15, currentStatus.FuseStatus2, ManualStatus2, 6);
  FuseChannel(currentStatus.Current_ch16, configPage1.Fuse_ch16, 16, OUT_CH16, LED_R_CH16, currentStatus.FuseStatus2, ManualStatus2, 7);
  fuseResetRequest = false;
}

// Bouton TS : réarme tous les fusibles à la prochaine trame ADC
void ResetFuses(){ fuseResetRequest = true; }

// 0 = normal, 1 = au-dessus de SAFETY_WARNING % du seuil, 2 = seuil atteint
uint8_t Level(uint16_t value, uint16_t seuil){
  if (value >= seuil) {return 2;}
  if ((uint32_t)value * 100UL >= (uint32_t)seuil * SAFETY_WARNING) {return 1;}
  return 0;
}

void CheckSecurity(){

//test des seuils, une fois par trame ADC
  if (newADC == true){
    newADC = false;
    CheckFuses();
    uint8_t lvlI = 0;  uint8_t codeI = 0;  uint16_t valI = 0;     // courants
    uint8_t lvlS = 0;  uint8_t codeS = 0;  uint16_t valS = 0;     // temperature et tension
    uint8_t l;

    l = Level(currentStatus.Current_ch1, MAX_CURRENT_1_12); if (l > lvlI) {lvlI = l; codeI = 1; valI = currentStatus.Current_ch1;}
    l = Level(currentStatus.Current_ch2, MAX_CURRENT_1_12); if (l > lvlI) {lvlI = l; codeI = 2; valI = currentStatus.Current_ch2;}
    l = Level(currentStatus.Current_ch3, MAX_CURRENT_1_12); if (l > lvlI) {lvlI = l; codeI = 3; valI = currentStatus.Current_ch3;}
    l = Level(currentStatus.Current_ch4, MAX_CURRENT_1_12); if (l > lvlI) {lvlI = l; codeI = 4; valI = currentStatus.Current_ch4;}
    l = Level(currentStatus.Current_ch5, MAX_CURRENT_1_12); if (l > lvlI) {lvlI = l; codeI = 5; valI = currentStatus.Current_ch5;}
    l = Level(currentStatus.Current_ch6, MAX_CURRENT_1_12); if (l > lvlI) {lvlI = l; codeI = 6; valI = currentStatus.Current_ch6;}
    l = Level(currentStatus.Current_ch7, MAX_CURRENT_1_12); if (l > lvlI) {lvlI = l; codeI = 7; valI = currentStatus.Current_ch7;}
    l = Level(currentStatus.Current_ch8, MAX_CURRENT_1_12); if (l > lvlI) {lvlI = l; codeI = 8; valI = currentStatus.Current_ch8;}
    l = Level(currentStatus.Current_ch9, MAX_CURRENT_1_12); if (l > lvlI) {lvlI = l; codeI = 9; valI = currentStatus.Current_ch9;}
    l = Level(currentStatus.Current_ch10, MAX_CURRENT_1_12); if (l > lvlI) {lvlI = l; codeI = 10; valI = currentStatus.Current_ch10;}
    l = Level(currentStatus.Current_ch11, MAX_CURRENT_1_12); if (l > lvlI) {lvlI = l; codeI = 11; valI = currentStatus.Current_ch11;}
    l = Level(currentStatus.Current_ch12, MAX_CURRENT_1_12); if (l > lvlI) {lvlI = l; codeI = 12; valI = currentStatus.Current_ch12;}

    l = Level(currentStatus.Current_ch13, MAX_CURRENT_13_16); if (l > lvlI) {lvlI = l; codeI = 13; valI = currentStatus.Current_ch13;}
    l = Level(currentStatus.Current_ch14, MAX_CURRENT_13_16); if (l > lvlI) {lvlI = l; codeI = 14; valI = currentStatus.Current_ch14;}
    l = Level(currentStatus.Current_ch15, MAX_CURRENT_13_16); if (l > lvlI) {lvlI = l; codeI = 15; valI = currentStatus.Current_ch15;}
    l = Level(currentStatus.Current_ch16, MAX_CURRENT_13_16); if (l > lvlI) {lvlI = l; codeI = 16; valI = currentStatus.Current_ch16;}

    l = Level(currentStatus.BoardTemp1, MAX_BOARD_TEMP); if (l > lvlS) {lvlS = l; codeS = 17; valS = currentStatus.BoardTemp1;}
    l = Level(currentStatus.BoardTemp2, MAX_BOARD_TEMP); if (l > lvlS) {lvlS = l; codeS = 18; valS = currentStatus.BoardTemp2;}
    l = 0; if (currentStatus.Battery10 >= MAX_BATTERY) {l = 2;}   // batterie : pas d'alerte "proche", seulement au seuil
    if (l > lvlS) {lvlS = l; codeS = 19; valS = currentStatus.Battery10;}

    if (lvlI > lvlS) {currentStatus.SafetyStatus = lvlI;}   // le pire des deux niveaux
    else             {currentStatus.SafetyStatus = lvlS;}
    if (lvlI == 2) {countCurrent++;} else {countCurrent = 0;}
    if (lvlS == 2) {countSlow++;}    else {countSlow = 0;}
    if (countCurrent > 250) {countCurrent = 250;}   // pas de debordement si TS reste connecte
    if (countSlow > 250)    {countSlow = 250;}

    bool coupure = false;
    if (countCurrent > FRAMES_CURRENT) {SaveError(codeI, valI); coupure = true;}
    if (countSlow > FRAMES_SLOW)       {SaveError(codeS, valS); coupure = true;}
    if (coupure == true && TS.connected() == false && digitalRead(MCU_OK) == HIGH) {   // une seule fois, pas si TS connecte (diag)
      AllOutputsOFF();                                  // toutes les voies coupées tout de suite
      AllRedON();                                       // signal de détresse
      delay(DISTRESS_DELAY);
      digitalWrite(MCU_OK, LOW);                       // le superviseur d'alim coupe tout
    }
  }

//LED status : rapide si proche ou au-dessus d'un seuil, double flash si TS connecte, lente sinon
  uint16_t t = millis() % 1000;
  if      (currentStatus.SafetyStatus != 0) {digitalWrite(LED_STATUS, (millis() / 100) % 2);}
  else if (TS.connected())                  {digitalWrite(LED_STATUS, (t < 100) || (t >= 200 && t < 300));}
  else                                      {digitalWrite(LED_STATUS, (millis() / 500) % 2);}
}
