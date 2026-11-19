/***********************************************************************************************************
 * Outputs_management.h - pilotage des 16 sorties de puissance
 *
 *   - broches OUT_CH1 à OUT_CH16 (HIGH = voie allumée) et InitOutputs()
 *   - liste des modes (MODE_...) : valeur de configPage2.OutMode[voie - 1]
 *   - une fonction par mode (Mode_Latch, Mode_Pwm...), la temporisation, les groupes
 *   - DriveChannel() : décide de l'état d'UNE voie (demande -> mode -> tempo -> fusible -> broche)
 *   - ManageOutputs() : FONCTION MÈRE, appelée dans loop(), une ligne par voie
 *
 * La fonction mère ne fait rien en mode test (currentStatus.TestMode == 1).
 * Une voie dont le fusible est coupé (FuseStatus1/2) reste toujours OFF.
 *
 * POUR AJOUTER UN MODE : un #define MODE_..., une fonction Mode_...(), un case dans DriveChannel().
 **********************************************************************************************************/

// Sorties de puissance (HIGH = voie allumée)
#define OUT_CH1   PD2
#define OUT_CH2   PD1
#define OUT_CH3   PD0
#define OUT_CH4   PC12
#define OUT_CH5   PC11
#define OUT_CH6   PC10
#define OUT_CH7   PA15
#define OUT_CH8   PA14
#define OUT_CH9   PD5
#define OUT_CH10  PD6
#define OUT_CH11  PD7
#define OUT_CH12  PB3
#define OUT_CH13  PD3
#define OUT_CH14  PD4
#define OUT_CH15  PC1
#define OUT_CH16  PC0

void InitOutputs(){
    pinMode(OUT_CH1, OUTPUT);   digitalWrite(OUT_CH1, LOW);
    pinMode(OUT_CH2, OUTPUT);   digitalWrite(OUT_CH2, LOW);
    pinMode(OUT_CH3, OUTPUT);   digitalWrite(OUT_CH3, LOW);
    pinMode(OUT_CH4, OUTPUT);   digitalWrite(OUT_CH4, LOW);
    pinMode(OUT_CH5, OUTPUT);   digitalWrite(OUT_CH5, LOW);
    pinMode(OUT_CH6, OUTPUT);   digitalWrite(OUT_CH6, LOW);
    pinMode(OUT_CH7, OUTPUT);   digitalWrite(OUT_CH7, LOW);
    pinMode(OUT_CH8, OUTPUT);   digitalWrite(OUT_CH8, LOW);
    pinMode(OUT_CH9, OUTPUT);   digitalWrite(OUT_CH9, LOW);
    pinMode(OUT_CH10, OUTPUT);  digitalWrite(OUT_CH10, LOW);
    pinMode(OUT_CH11, OUTPUT);  digitalWrite(OUT_CH11, LOW);
    pinMode(OUT_CH12, OUTPUT);  digitalWrite(OUT_CH12, LOW);
    pinMode(OUT_CH13, OUTPUT);  digitalWrite(OUT_CH13, LOW);
    pinMode(OUT_CH14, OUTPUT);  digitalWrite(OUT_CH14, LOW);
    pinMode(OUT_CH15, OUTPUT);  digitalWrite(OUT_CH15, LOW);
    pinMode(OUT_CH16, OUTPUT);  digitalWrite(OUT_CH16, LOW);
}


//************************************************************************************************************
// MODES DES SORTIES (valeur de configPage2.OutMode[])
// Pour ajouter un mode : un #define ici, une fonction Mode_...() plus bas, et un "case" dans DriveChannel()

#define MODE_OFF      0     // toujours coupée
#define MODE_ON       1     // toujours allumée
#define MODE_INPUT    2     // suit sa source
#define MODE_LATCH    3     // un appui allume, un appui coupe
#define MODE_PWM      4     // clignote (période et rapport cyclique réglables) tant que la source est active
#define MODE_LATCH_PWM 5    // un appui lance le clignotement, un appui l'arrête
// #define MODE_PERSO_1 20  // fonctions personnalisées à partir de 20

#define LATCH_PRESS   100   // ms d'appui mini pour basculer un LATCH

// Mémoires de chaque voie (case 0 inutilisée : voie 1 = case 1)
bool     latchState[17];    // LATCH : voie allumée ou non
uint32_t pressStart[17];    // LATCH : début de l'appui en cours
bool     pressDone[17];     // LATCH : appui déjà pris en compte
uint32_t onStart[17];       // tempo : instant où la voie s'est mise en service
bool     timedOut[17];      // tempo : voie coupée par la tempo

//************************************************************************************************************
// SOURCES ET GROUPES

// Une source est-elle active ? (0 = aucune, 1-16 = entrée physique, 17-32 = entrée CAN virtuelle)
bool SourceActive(uint8_t source){
  if (source == 0)  {return false;}
  if (source <= 8)  {return BIT_CHECK(currentStatus.DigitalStatus1, source - 1);}
  if (source <= 16) {return BIT_CHECK(currentStatus.DigitalStatus2, source - 9);}
  if (source <= 24) {return BIT_CHECK(currentStatus.CANInputs1, source - 17);}
  if (source <= 32) {return BIT_CHECK(currentStatus.CANInputs2, source - 25);}
  return false;
}

// Un groupe (0 à 7) est actif si son entrée est active ou si le CAN l'a activé
bool GroupActive(uint8_t g){
  if (BIT_CHECK(currentStatus.CANGroups, g) == true) {return true;}
  return SourceActive(configPage2.GroupInput[g]);
}

// La voie fait-elle partie d'un groupe actif ?
bool GroupDemand(uint8_t voie){
  if ((configPage2.GroupOutA[0] == voie || configPage2.GroupOutB[0] == voie) && GroupActive(0) == true) {return true;}
  if ((configPage2.GroupOutA[1] == voie || configPage2.GroupOutB[1] == voie) && GroupActive(1) == true) {return true;}
  if ((configPage2.GroupOutA[2] == voie || configPage2.GroupOutB[2] == voie) && GroupActive(2) == true) {return true;}
  if ((configPage2.GroupOutA[3] == voie || configPage2.GroupOutB[3] == voie) && GroupActive(3) == true) {return true;}
  if ((configPage2.GroupOutA[4] == voie || configPage2.GroupOutB[4] == voie) && GroupActive(4) == true) {return true;}
  if ((configPage2.GroupOutA[5] == voie || configPage2.GroupOutB[5] == voie) && GroupActive(5) == true) {return true;}
  if ((configPage2.GroupOutA[6] == voie || configPage2.GroupOutB[6] == voie) && GroupActive(6) == true) {return true;}
  if ((configPage2.GroupOutA[7] == voie || configPage2.GroupOutB[7] == voie) && GroupActive(7) == true) {return true;}
  return false;
}

//************************************************************************************************************
// UNE FONCTION PAR MODE

// LATCH : un appui de plus de LATCH_PRESS ms bascule la voie (ON -> OFF, OFF -> ON)
bool Mode_Latch(uint8_t voie, bool demande){
  if (demande == true){
    if (pressStart[voie] == 0) {pressStart[voie] = millis(); if (pressStart[voie] == 0) {pressStart[voie] = 1;}}
    if (pressDone[voie] == false && millis() - pressStart[voie] >= LATCH_PRESS){
      latchState[voie] = !latchState[voie];     // bascule
      pressDone[voie] = true;                   // une seule bascule par appui
    }
  }
  else {pressStart[voie] = 0; pressDone[voie] = false;}
  return latchState[voie];
}

// PWM : calé sur l'horloge commune, donc deux voies réglées pareil clignotent en phase (warning)
bool Mode_Pwm(uint8_t voie, bool demande){
  if (demande == false) {return false;}
  uint8_t per  = configPage2.OutPWMPeriod[voie - 1];
  uint8_t duty = configPage2.OutPWMDuty[voie - 1];
  if (per == 0) {return true;}                                  // 0 = allumage fixe
  uint32_t periode = per * 200UL;                               // ms (période en 0.2 s)
  return (millis() % periode) < (periode * duty / 100);
}

//************************************************************************************************************
// TEMPORISATION : vrai si la voie doit être coupée parce qu'elle est en service depuis trop longtemps.
// Elle ne repart qu'après une nouvelle demande (source relâchée puis réactivée, ou nouvel appui en LATCH).
bool TimeoutReached(uint8_t voie, bool enService){
  uint8_t tempo = configPage2.OutTimeout[voie - 1];
  if (tempo == 0 || enService == false) {onStart[voie] = 0; timedOut[voie] = false; return false;}
  if (timedOut[voie] == true) {return true;}
  if (onStart[voie] == 0) {onStart[voie] = millis(); if (onStart[voie] == 0) {onStart[voie] = 1;}}
  if (millis() - onStart[voie] >= (uint32_t)tempo * 500UL){
    timedOut[voie] = true;
    latchState[voie] = false;                 // un LATCH revient à OFF
    return true;
  }
  return false;
}

//************************************************************************************************************
// PILOTAGE D'UNE VOIE
void DriveChannel(uint8_t voie, uint32_t pin){
  uint8_t mode = configPage2.OutMode[voie - 1];
  bool entree = SourceActive(configPage2.OutSource[voie - 1] & 0x3F);      // bits 0-5 : numéro de source
  if (BIT_CHECK(configPage2.OutSource[voie - 1], 7) == true) {entree = !entree;}   // bit 7 : sens inversé
  bool demande = entree || GroupDemand(voie);
  bool enService = false;      // voie demandée (pour la tempo)
  bool etat = false;           // ce qu'on envoie sur la broche

  switch (mode){
    case MODE_OFF:   etat = false;                                             break;
    case MODE_ON:    etat = true;                                              break;
    case MODE_INPUT: enService = demande;  etat = demande;                     break;
    case MODE_LATCH: enService = Mode_Latch(voie, demande);  etat = enService; break;
    case MODE_PWM:   enService = demande;  etat = Mode_Pwm(voie, demande);     break;
    case MODE_LATCH_PWM: enService = Mode_Latch(voie, demande);  etat = Mode_Pwm(voie, enService); break;
    // case MODE_PERSO_1: ... break;
  }

  // tempo (pas en ON ni en OFF)
  if (mode != MODE_ON && mode != MODE_OFF && TimeoutReached(voie, enService) == true) {etat = false;}

  // fusible coupé : jamais ON
  if (voie <= 8 && BIT_CHECK(currentStatus.FuseStatus1, voie - 1) == true) {etat = false;}
  if (voie > 8  && BIT_CHECK(currentStatus.FuseStatus2, voie - 9) == true) {etat = false;}

  digitalWrite(pin, etat);

  // état réel de la voie pour TS et le CAN
  if (voie <= 8) {if (etat == true) {BIT_SET(currentStatus.OutputStatus1, voie - 1);} else {BIT_CLEAR(currentStatus.OutputStatus1, voie - 1);}}
  else           {if (etat == true) {BIT_SET(currentStatus.OutputStatus2, voie - 9);} else {BIT_CLEAR(currentStatus.OutputStatus2, voie - 9);}}
}

//************************************************************************************************************
// FONCTION MÈRE (dans loop)
void ManageOutputs(){
  if (currentStatus.TestMode == 1) {return;}   // mode test : les commandes TS ont la main
  DriveChannel(1,  OUT_CH1);
  DriveChannel(2,  OUT_CH2);
  DriveChannel(3,  OUT_CH3);
  DriveChannel(4,  OUT_CH4);
  DriveChannel(5,  OUT_CH5);
  DriveChannel(6,  OUT_CH6);
  DriveChannel(7,  OUT_CH7);
  DriveChannel(8,  OUT_CH8);
  DriveChannel(9,  OUT_CH9);
  DriveChannel(10, OUT_CH10);
  DriveChannel(11, OUT_CH11);
  DriveChannel(12, OUT_CH12);
  DriveChannel(13, OUT_CH13);
  DriveChannel(14, OUT_CH14);
  DriveChannel(15, OUT_CH15);
  DriveChannel(16, OUT_CH16);
}
