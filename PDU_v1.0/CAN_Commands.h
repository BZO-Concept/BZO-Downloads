/***********************************************************************************************************
 * CAN_Commands.h - le CAN, organisé en bac à sable
 *
 *   - les modes CAN (configPage1.CANMode) et TOUS les ID de trames sont en #define ci-dessous (fixes)
 *   - ReadCAN() (dans loop) : UNE trame par tour, passe au sniffer, compte le trafic, puis la donne
 *     au décodeur du mode choisi (switch). Gère aussi la sécurité CAN (CANTimeout).
 *   - trames d'entrée (commandes reçues) : configPage1.CANMode ; trames de sortie (état, courants) : configPage1.CANOutMode
 *   - SendCAN() (dans loop) : trames de sortie en continu si CANOutMode = Broadcast (CANTxRate)
 *   - les réglages reçus par CAN (modes, tempos, PWM) sont écrits en RAM dans configPage2, sans sauvegarde
 *
 * Format BZO : 1 octet par voie ou par groupe, 0xFF = inchangé. Détail des trames : FICHE_TRAMES_CAN_BZO_PDU.md.
 *
 * POUR AJOUTER UN MODE CAN : un #define CANMODE_..., ses ID en #define, une fonction CAN_...(),
 * un case dans le switch de ReadCAN(). Les fonctions d'émission doivent être écrites avant celles qui
 * les appellent (ou avoir un prototype).
 **********************************************************************************************************/

#include <STM32_CAN.h>
STM32_CAN Can( CAN1, DEF, RX_SIZE_64, TX_SIZE_16 );
static CAN_message_t CAN_RX_msg;
static CAN_message_t CAN_TX_msg;

//************************************************************************************************************
// MODES CAN (configPage1.CANMode)
// Bac à sable : pour ajouter un mode (boîte à boutons, COM2000...) : un #define ici, ses ID plus bas,
// une fonction CAN_...() et un "case" dans ReadCAN().

#define CANMODE_LATCH      0     // entrées Demande/Réponse : commande -> action -> confirmation
#define CANMODE_BROADCAST  1     // entrées Broadcast : commande appliquée, pas de réponse
// #define CANMODE_PERSO_1 2

// MODES DES TRAMES DE SORTIE (configPage1.CANOutMode)
#define CANOUT_DEMANDE     0     // état / courants envoyés en réponse aux demandes 0x60E / 0x60F
#define CANOUT_BROADCAST   1     // état / courants envoyés en continu à CANTxRate

//************************************************************************************************************
// ID DES TRAMES BZO (fixes, à changer ici si besoin)

#define CAN_ID_ONOFF_1    0x600  // reçue : ON/OFF virtuels voies 1-8   (1 octet par voie : 0 = OFF, 1 = ON, 0xFF = inchangé)
#define CAN_ID_ONOFF_2    0x601  // reçue : ON/OFF virtuels voies 9-16
#define CAN_ID_MODE_1     0x602  // reçue : modes voies 1-8             (1 octet par voie, 0xFF = inchangé)
#define CAN_ID_MODE_2     0x603  // reçue : modes voies 9-16
#define CAN_ID_TEMPO_1    0x604  // reçue : tempos voies 1-8 (x0.5 s)
#define CAN_ID_TEMPO_2    0x605  // reçue : tempos voies 9-16
#define CAN_ID_PERIOD_1   0x606  // reçue : périodes PWM voies 1-8 (x0.2 s)
#define CAN_ID_PERIOD_2   0x607  // reçue : périodes PWM voies 9-16
#define CAN_ID_DUTY_1     0x608  // reçue : rapports cycliques voies 1-8 (%)
#define CAN_ID_DUTY_2     0x609  // reçue : rapports cycliques voies 9-16
#define CAN_ID_GROUPS     0x60A  // reçue : groupes 1-8 (1 octet par groupe : 0 = OFF, 1 = ON, 0xFF = inchangé)
#define CAN_ID_ASK_STATE  0x60E  // reçue : demande d'état      -> réponse CAN_ID_STATE
#define CAN_ID_ASK_CURR   0x60F  // reçue : demande des courants -> réponse CAN_ID_CURR_1 à 3
#define CAN_ID_STATE      0x610  // émise : état (confirmation des commandes / trame de sortie)
#define CAN_ID_CURR_1     0x611  // émise : courants voies 1-8 (x0.1 A)
#define CAN_ID_CURR_2     0x612  // émise : courants voies 9-12 (x0.1 A)
#define CAN_ID_CURR_3     0x613  // émise : courants voies 13-16 (x0.1 A, 2 octets par voie)

uint32_t lastCANRx = 0;       // dernière trame reçue (sécurité CAN)
uint32_t lastCANError = 0;    // dernière trame BZO refusée (0 = jamais)
#define CAN_ERROR_HOLD  2000  // ms : durée pendant laquelle le voyant CAN reste rouge
uint16_t trafficCount = 0;    // trames reçues dans la seconde en cours
uint32_t trafficTimer = 0;
uint32_t lastDashSend = 0;

//************************************************************************************************************
// ÉMISSION

// État : sorties, fusibles, sécurité, mode test, température max, tension
void SendState(){
  CAN_TX_msg.id = CAN_ID_STATE;
  CAN_TX_msg.len = 8;
  CAN_TX_msg.buf[0] = currentStatus.OutputStatus1;
  CAN_TX_msg.buf[1] = currentStatus.OutputStatus2;
  CAN_TX_msg.buf[2] = currentStatus.FuseStatus1;
  CAN_TX_msg.buf[3] = currentStatus.FuseStatus2;
  CAN_TX_msg.buf[4] = currentStatus.SafetyStatus;
  CAN_TX_msg.buf[5] = currentStatus.TestMode;
  if (currentStatus.BoardTemp1 > currentStatus.BoardTemp2) {CAN_TX_msg.buf[6] = currentStatus.BoardTemp1;}
  else                                                     {CAN_TX_msg.buf[6] = currentStatus.BoardTemp2;}
  CAN_TX_msg.buf[7] = currentStatus.Battery10;
  Can.write(CAN_TX_msg);
}

// Courants des 16 voies (3 trames)
void SendCurrents(){
  CAN_TX_msg.id = CAN_ID_CURR_1;
  CAN_TX_msg.len = 8;
  CAN_TX_msg.buf[0] = currentStatus.Current_ch1;
  CAN_TX_msg.buf[1] = currentStatus.Current_ch2;
  CAN_TX_msg.buf[2] = currentStatus.Current_ch3;
  CAN_TX_msg.buf[3] = currentStatus.Current_ch4;
  CAN_TX_msg.buf[4] = currentStatus.Current_ch5;
  CAN_TX_msg.buf[5] = currentStatus.Current_ch6;
  CAN_TX_msg.buf[6] = currentStatus.Current_ch7;
  CAN_TX_msg.buf[7] = currentStatus.Current_ch8;
  Can.write(CAN_TX_msg);

  CAN_TX_msg.id = CAN_ID_CURR_2;
  CAN_TX_msg.len = 4;
  CAN_TX_msg.buf[0] = currentStatus.Current_ch9;
  CAN_TX_msg.buf[1] = currentStatus.Current_ch10;
  CAN_TX_msg.buf[2] = currentStatus.Current_ch11;
  CAN_TX_msg.buf[3] = currentStatus.Current_ch12;
  Can.write(CAN_TX_msg);

  CAN_TX_msg.id = CAN_ID_CURR_3;
  CAN_TX_msg.len = 8;
  CAN_TX_msg.buf[0] = lowByte(currentStatus.Current_ch13);
  CAN_TX_msg.buf[1] = highByte(currentStatus.Current_ch13);
  CAN_TX_msg.buf[2] = lowByte(currentStatus.Current_ch14);
  CAN_TX_msg.buf[3] = highByte(currentStatus.Current_ch14);
  CAN_TX_msg.buf[4] = lowByte(currentStatus.Current_ch15);
  CAN_TX_msg.buf[5] = highByte(currentStatus.Current_ch15);
  CAN_TX_msg.buf[6] = lowByte(currentStatus.Current_ch16);
  CAN_TX_msg.buf[7] = highByte(currentStatus.Current_ch16);
  Can.write(CAN_TX_msg);
}

//************************************************************************************************************
// RECOPIE DES TRAMES REÇUES

// Recopie un octet reçu dans un réglage (0xFF = inchangé)
void CopyByte(uint8_t &reglage, uint8_t recu){
  if (recu != 0xFF) {reglage = recu;}
}

// Recopie une trame dans un tableau de réglages, à partir de la case "debut" (0 = voie 1, 8 = voie 9)
void CopyCAN(uint8_t tableau[], uint8_t debut){
  CopyByte(tableau[debut],     CAN_RX_msg.buf[0]);
  CopyByte(tableau[debut + 1], CAN_RX_msg.buf[1]);
  CopyByte(tableau[debut + 2], CAN_RX_msg.buf[2]);
  CopyByte(tableau[debut + 3], CAN_RX_msg.buf[3]);
  CopyByte(tableau[debut + 4], CAN_RX_msg.buf[4]);
  CopyByte(tableau[debut + 5], CAN_RX_msg.buf[5]);
  CopyByte(tableau[debut + 6], CAN_RX_msg.buf[6]);
  CopyByte(tableau[debut + 7], CAN_RX_msg.buf[7]);
}

// Met le bit "rang" selon l'octet reçu (0 = OFF, 1 = ON, autre = inchangé)
void CopyBit(uint8_t &octet, uint8_t rang, uint8_t recu){
  if (recu == 0) {BIT_CLEAR(octet, rang);}
  if (recu == 1) {BIT_SET(octet, rang);}
}

// Recopie une trame d'ON/OFF dans un octet de bits
void CopyCANBits(uint8_t &octet){
  CopyBit(octet, 0, CAN_RX_msg.buf[0]);
  CopyBit(octet, 1, CAN_RX_msg.buf[1]);
  CopyBit(octet, 2, CAN_RX_msg.buf[2]);
  CopyBit(octet, 3, CAN_RX_msg.buf[3]);
  CopyBit(octet, 4, CAN_RX_msg.buf[4]);
  CopyBit(octet, 5, CAN_RX_msg.buf[5]);
  CopyBit(octet, 6, CAN_RX_msg.buf[6]);
  CopyBit(octet, 7, CAN_RX_msg.buf[7]);
}

//************************************************************************************************************
// MODES BZO

// Contrôle de syntaxe : 8 octets, chacun = 0xFF (inchangé) ou entre 0 et maxi
bool ValeurOK(uint8_t recu, uint8_t maxi){
  if (recu == 0xFF) {return true;}
  if (recu <= maxi) {return true;}
  return false;
}

bool TrameOK(uint8_t maxi){
  if (CAN_RX_msg.len != 8) {return false;}
  if (ValeurOK(CAN_RX_msg.buf[0], maxi) == false) {return false;}
  if (ValeurOK(CAN_RX_msg.buf[1], maxi) == false) {return false;}
  if (ValeurOK(CAN_RX_msg.buf[2], maxi) == false) {return false;}
  if (ValeurOK(CAN_RX_msg.buf[3], maxi) == false) {return false;}
  if (ValeurOK(CAN_RX_msg.buf[4], maxi) == false) {return false;}
  if (ValeurOK(CAN_RX_msg.buf[5], maxi) == false) {return false;}
  if (ValeurOK(CAN_RX_msg.buf[6], maxi) == false) {return false;}
  if (ValeurOK(CAN_RX_msg.buf[7], maxi) == false) {return false;}
  return true;
}

// Trames de commande BZO, communes au Latch et au Broadcast. Renvoie true si la trame était une commande valide.
// Une trame sur un ID BZO mais avec une syntaxe incorrecte est ignorée en entier et allume le voyant CAN.
// (les modes, tempos et PWM reçus sont écrits en RAM, sans sauvegarde)
bool CAN_BZOCommands(){
  uint8_t maxi = 0;
  switch (CAN_RX_msg.id){
    case CAN_ID_ONOFF_1:  case CAN_ID_ONOFF_2:  case CAN_ID_GROUPS:   maxi = 1;               break;
    case CAN_ID_MODE_1:   case CAN_ID_MODE_2:                         maxi = MODE_LATCH_PWM;  break;
    case CAN_ID_TEMPO_1:  case CAN_ID_TEMPO_2:                        maxi = 254;             break;
    case CAN_ID_PERIOD_1: case CAN_ID_PERIOD_2:                       maxi = 254;             break;
    case CAN_ID_DUTY_1:   case CAN_ID_DUTY_2:                         maxi = 100;             break;
    default: return false;                                            // pas une commande BZO
  }
  if (TrameOK(maxi) == false){
    lastCANError = millis();
    if (lastCANError == 0) {lastCANError = 1;}
    return false;
  }

  switch (CAN_RX_msg.id){
    case CAN_ID_ONOFF_1: CopyCANBits(currentStatus.CANInputs1);  return true;
    case CAN_ID_ONOFF_2: CopyCANBits(currentStatus.CANInputs2);  return true;
    case CAN_ID_MODE_1:  CopyCAN(configPage2.OutMode, 0);        return true;
    case CAN_ID_MODE_2:  CopyCAN(configPage2.OutMode, 8);        return true;
    case CAN_ID_TEMPO_1: CopyCAN(configPage2.OutTimeout, 0);     return true;
    case CAN_ID_TEMPO_2: CopyCAN(configPage2.OutTimeout, 8);     return true;
    case CAN_ID_PERIOD_1: CopyCAN(configPage2.OutPWMPeriod, 0);  return true;
    case CAN_ID_PERIOD_2: CopyCAN(configPage2.OutPWMPeriod, 8);  return true;
    case CAN_ID_DUTY_1:  CopyCAN(configPage2.OutPWMDuty, 0);     return true;
    case CAN_ID_DUTY_2:  CopyCAN(configPage2.OutPWMDuty, 8);     return true;
    case CAN_ID_GROUPS:  CopyCANBits(currentStatus.CANGroups);   return true;
  }
  return false;
}

// Entrées en Demande/Réponse : commande -> action -> confirmation
void CAN_Latch(){
  if (CAN_BZOCommands() == true)         {SendState();}       // confirmation
}

// Mode Broadcast : on applique, on ne répond pas
void CAN_Broadcast(){
  CAN_BZOCommands();
}

//************************************************************************************************************
// SNIFFER : les 9 plus anciennes descendent d'un cran, la nouvelle prend la place 0
void Sniff(){
  if (currentStatus.SniffPause == 1) {return;}   // pause demandée depuis TS : on fige l'affichage
  currentStatus.CANSniff[9] = currentStatus.CANSniff[8];
  currentStatus.CANSniff[8] = currentStatus.CANSniff[7];
  currentStatus.CANSniff[7] = currentStatus.CANSniff[6];
  currentStatus.CANSniff[6] = currentStatus.CANSniff[5];
  currentStatus.CANSniff[5] = currentStatus.CANSniff[4];
  currentStatus.CANSniff[4] = currentStatus.CANSniff[3];
  currentStatus.CANSniff[3] = currentStatus.CANSniff[2];
  currentStatus.CANSniff[2] = currentStatus.CANSniff[1];
  currentStatus.CANSniff[1] = currentStatus.CANSniff[0];

  currentStatus.CANSniff[0].id  = CAN_RX_msg.id;
  currentStatus.CANSniff[0].len = CAN_RX_msg.len;
  currentStatus.CANSniff[0].data[0] = CAN_RX_msg.buf[0];
  currentStatus.CANSniff[0].data[1] = CAN_RX_msg.buf[1];
  currentStatus.CANSniff[0].data[2] = CAN_RX_msg.buf[2];
  currentStatus.CANSniff[0].data[3] = CAN_RX_msg.buf[3];
  currentStatus.CANSniff[0].data[4] = CAN_RX_msg.buf[4];
  currentStatus.CANSniff[0].data[5] = CAN_RX_msg.buf[5];
  currentStatus.CANSniff[0].data[6] = CAN_RX_msg.buf[6];
  currentStatus.CANSniff[0].data[7] = CAN_RX_msg.buf[7];
}

//************************************************************************************************************
void InitCAN(){
  if(configPage1.CANspeed > 0){
    Can.begin();
    if(configPage1.CANspeed == 1){Can.setBaudRate(125000);}
    if(configPage1.CANspeed == 2){Can.setBaudRate(250000);}
    if(configPage1.CANspeed == 3){Can.setBaudRate(500000);}
    if(configPage1.CANspeed == 4){Can.setBaudRate(1000000);}
  }
}

//************************************************************************************************************
// LECTURE (dans loop) : une trame par tour, pour ne pas bloquer si le réseau est saturé
void ReadCAN(){
  if (configPage1.CANspeed == 0) {return;}

  // voyant CAN : rouge pendant CAN_ERROR_HOLD ms après une trame refusée
  if (lastCANError != 0 && millis() - lastCANError < CAN_ERROR_HOLD) {currentStatus.CANError = 1;}
  else                                                               {currentStatus.CANError = 0;}

  if (Can.read(CAN_RX_msg)){
    Sniff();
    trafficCount++;
    lastCANRx = millis();

    switch (configPage1.CANMode){
      case CANMODE_LATCH:     CAN_Latch();     break;
      case CANMODE_BROADCAST: CAN_Broadcast(); break;
      // case CANMODE_PERSO_1: CAN_Perso1();   break;
    }

    // trames de sortie en Demande/Réponse : réponse aux demandes
    if (configPage1.CANOutMode == CANOUT_DEMANDE){
      if (CAN_RX_msg.id == CAN_ID_ASK_STATE) {SendState();}
      if (CAN_RX_msg.id == CAN_ID_ASK_CURR)  {SendCurrents();}
    }
  }

  // trafic : trames reçues par seconde
  if (millis() - trafficTimer >= 1000){
    currentStatus.CANTraffic = trafficCount;
    trafficCount = 0;
    trafficTimer = millis();
  }

  // sécurité CAN : plus rien reçu depuis CANTimeout s -> entrées et groupes CAN à OFF
  if (configPage1.CANTimeout != 0 && millis() - lastCANRx >= (uint32_t)configPage1.CANTimeout * 1000UL){
    currentStatus.CANInputs1 = 0;
    currentStatus.CANInputs2 = 0;
    currentStatus.CANGroups = 0;
  }
}

// TABLEAU DE BORD EN CONTINU (dans loop)
void SendCAN(){
  if (configPage1.CANspeed == 0 || configPage1.CANOutMode != CANOUT_BROADCAST || configPage1.CANTxRate == 0) {return;}
  if (millis() - lastDashSend >= 1000UL / configPage1.CANTxRate){
    lastDashSend = millis();
    SendState();
    SendCurrents();
  }
}
