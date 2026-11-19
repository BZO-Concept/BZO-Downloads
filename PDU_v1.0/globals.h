/***********************************************************************************************************
 * globals.h - tout ce qui est partagé par le firmware
 *
 *   - macros BIT_SET / BIT_CLEAR / BIT_CHECK (à utiliser pour toute manipulation de bit)
 *   - struct config1  = page 1 du .ini (configPage1) : entrées, CAN, fusibles
 *   - struct config2  = page 2 du .ini (configPage2) : réglages des 16 sorties et des 8 groupes
 *   - struct statuses = valeurs temps réel (currentStatus), structure libre
 *   - getTSLogEntry()  = liste des octets envoyés à TunerStudio ([OutputChannels] du .ini)
 *
 * RÈGLES :
 *   - les pages restent "packed" : TunerStudio lit et écrit leurs octets directement
 *   - un nouveau réglage se met à la FIN de config1 ou config2, et à l'offset correspondant dans le .ini
 *   - une nouvelle valeur temps réel : champ dans statuses + case dans getTSLogEntry()
 *     + TS_LOG_TAILLE + ligne dans [OutputChannels] du .ini (même offset)
 *   - le numéro en commentaire des champs de statuses = offset dans le log TS
 **********************************************************************************************************/

#define BIT_SET(a,b) ((a) |= (1U<<(b)))
#define BIT_CLEAR(a,b) ((a) &= ~(1U<<(b)))          //Macros pour les fonctions de lecture/ecriture bit
#define BIT_CHECK(var,pos) !!((var) & (1U<<(pos)))

//************************************************************************************************************
// configPage1 = page 1 du .ini : réglages généraux (25 octets) - offsets : voir GUIDE_IA_BZO_PDU.md §4.2
struct __attribute__((packed)) config1
{
  uint8_t  DigitalLevel1;    // choix HIGH/LOW ch1-8
  uint8_t  DigitalLevel2;    // choix HIGH/LOW ch9-16
  uint8_t  CANspeed;         // choix vitesse CAN (0 = Désactivé, 1 = 125k, 2 = 250k, 3 = 500k, 4 = 1M)
  uint8_t  CANMode;          // trames d'entrée (0 = Demande/Réponse, 1 = Broadcast, 2 et + = modes ajoutés dans CAN_Commands.h)
  uint8_t  CANTimeout;       // sécurité CAN : s sans trame reçue avant de repasser les entrées CAN à OFF (0 = désactivée)
  uint8_t  CANOutMode;       // trames de sortie : 0 = Demande/Réponse, 1 = Broadcast
  uint8_t  CANTxRate;        // fréquence des trames de sortie en Broadcast (Hz)
  uint8_t  Fuse_ch1;           // fusible voie 1 : courant max (A, 0 = désactivé)
  uint8_t  Fuse_ch2;           // fusible voie 2 : courant max (A, 0 = désactivé)
  uint8_t  Fuse_ch3;           // fusible voie 3 : courant max (A, 0 = désactivé)
  uint8_t  Fuse_ch4;           // fusible voie 4 : courant max (A, 0 = désactivé)
  uint8_t  Fuse_ch5;           // fusible voie 5 : courant max (A, 0 = désactivé)
  uint8_t  Fuse_ch6;           // fusible voie 6 : courant max (A, 0 = désactivé)
  uint8_t  Fuse_ch7;           // fusible voie 7 : courant max (A, 0 = désactivé)
  uint8_t  Fuse_ch8;           // fusible voie 8 : courant max (A, 0 = désactivé)
  uint8_t  Fuse_ch9;           // fusible voie 9 : courant max (A, 0 = désactivé)
  uint8_t  Fuse_ch10;          // fusible voie 10 : courant max (A, 0 = désactivé)
  uint8_t  Fuse_ch11;          // fusible voie 11 : courant max (A, 0 = désactivé)
  uint8_t  Fuse_ch12;          // fusible voie 12 : courant max (A, 0 = désactivé)
  uint8_t  Fuse_ch13;          // fusible voie 13 : courant max (A, 0 = désactivé)
  uint8_t  Fuse_ch14;          // fusible voie 14 : courant max (A, 0 = désactivé)
  uint8_t  Fuse_ch15;          // fusible voie 15 : courant max (A, 0 = désactivé)
  uint8_t  Fuse_ch16;          // fusible voie 16 : courant max (A, 0 = désactivé)
  uint8_t  FuseDelay;         // fusibles : temps toléré au-dessus du seuil avant coupure, toutes voies (x0.1 s)
  uint8_t  FuseRearm;         // fusibles : réarmement automatique après coupure, toutes voies (x0.1 s, 0 = pas de réarmement auto)
};

// configPage2 = page 2 du .ini : réglages des sorties (104 octets)
struct __attribute__((packed)) config2
{
  uint8_t  OutMode[16];      // mode de chaque voie (MODE_... dans Outputs_management.h)
  uint8_t  OutSource[16];    // bits 0-5 : 0 = aucune, 1-16 = entrée physique 1-16, 17-32 = entrée CAN virtuelle 1-16 ; bit 7 : sens inversé
  uint8_t  OutTimeout[16];   // coupure au bout de... (x0.5 s, 0 = sans limite, ignorée en ON et OFF)
  uint8_t  OutPWMPeriod[16]; // période PWM (x0.2 s, 0 = allumage fixe)
  uint8_t  OutPWMDuty[16];   // rapport cyclique PWM (%)
  uint8_t  GroupInput[8];    // entrée qui active le groupe (0 = aucune, 1-16)
  uint8_t  GroupOutA[8];     // 1re voie commandée par le groupe (0 = aucune, 1-16)
  uint8_t  GroupOutB[8];     // 2e voie commandée par le groupe (0 = aucune, 1-16)
};

struct CANFrame                 // une trame vue par le sniffer CAN
{
  uint16_t id;
  uint8_t  len;
  uint8_t  data[8];
};

//************************************************************************************************************
// currentStatus : valeurs temps réel. Le numéro en commentaire = offset dans le log TS (getTSLogEntry plus bas)
struct statuses                 // structure libre : ce qui part vers TS est dans getTSLogEntry()
{
  uint8_t  secl;                // secondes depuis le démarrage (rempli par la librairie)
  uint8_t  statutTS;            // statut com/mémoire (rempli par la librairie)
  uint8_t  erreursTS;           // erreurs de com (rempli par la librairie)
  uint8_t  TestMode;            // mode test : 0 = normal, 1 = manuel (Start Test / End Test)
  uint16_t boucles;             // tours de loop par seconde (rempli par la librairie)
  uint8_t DigitalStatus1;       // 6 (ch1-8)
  uint8_t DigitalStatus2;       // 7 (ch9-16)
  uint8_t IntensityADC_ch1;     // intensite voie 1 (brut 0-255)
  uint8_t IntensityADC_ch2;     // intensite voie 2 (brut 0-255)
  uint8_t IntensityADC_ch3;     // intensite voie 3 (brut 0-255)
  uint8_t IntensityADC_ch4;     // intensite voie 4 (brut 0-255)
  uint8_t IntensityADC_ch5;     // intensite voie 5 (brut 0-255)
  uint8_t IntensityADC_ch6;     // intensite voie 6 (brut 0-255)
  uint8_t IntensityADC_ch7;     // intensite voie 7 (brut 0-255)
  uint8_t IntensityADC_ch8;     // intensite voie 8 (brut 0-255)
  uint8_t IntensityADC_ch9;     // intensite voie 9 (brut 0-255)
  uint8_t IntensityADC_ch10;    // intensite voie 10 (brut 0-255)
  uint8_t IntensityADC_ch11;    // intensite voie 11 (brut 0-255)
  uint8_t IntensityADC_ch12;    // intensite voie 12 (brut 0-255)
  uint8_t IntensityADC_ch13;    // intensite voie 13 (brut 0-255)
  uint8_t IntensityADC_ch14;    // intensite voie 14 (brut 0-255)
  uint8_t IntensityADC_ch15;    // intensite voie 15 (brut 0-255)
  uint8_t IntensityADC_ch16;    // intensite voie 16 (brut 0-255)
  uint8_t BoardTempADC1;        // temperature carte capteur IC9 (brut)
  uint8_t BoardTempADC2;        // temperature carte capteur IC18 (brut)
  uint8_t BatteryADC;           // tension +12P (brut, pont 82k/10k)

  uint8_t  Current_ch1;         // 8 courant voie 1 (x0.1 A)
  uint8_t  Current_ch2;         // 9 courant voie 2 (x0.1 A)
  uint8_t  Current_ch3;         // 10 courant voie 3 (x0.1 A)
  uint8_t  Current_ch4;         // 11 courant voie 4 (x0.1 A)
  uint8_t  Current_ch5;         // 12 courant voie 5 (x0.1 A)
  uint8_t  Current_ch6;         // 13 courant voie 6 (x0.1 A)
  uint8_t  Current_ch7;         // 14 courant voie 7 (x0.1 A)
  uint8_t  Current_ch8;         // 15 courant voie 8 (x0.1 A)
  uint8_t  Current_ch9;         // 16 courant voie 9 (x0.1 A)
  uint8_t  Current_ch10;        // 17 courant voie 10 (x0.1 A)
  uint8_t  Current_ch11;        // 18 courant voie 11 (x0.1 A)
  uint8_t  Current_ch12;        // 19 courant voie 12 (x0.1 A)
  uint16_t Current_ch13;        // 20 courant voie 13 (x0.1 A)
  uint16_t Current_ch14;        // 22 courant voie 14 (x0.1 A)
  uint16_t Current_ch15;        // 24 courant voie 15 (x0.1 A)
  uint16_t Current_ch16;        // 26 courant voie 16 (x0.1 A)
  uint8_t  BoardTemp1;          // 28 temperature carte IC9 (°C, 0 si negatif)
  uint8_t  BoardTemp2;          // 29 temperature carte IC18 (°C, 0 si negatif)
  uint8_t  Battery10;           // 30 tension +12P (x0.1 V)
  uint8_t  SafetyStatus;        // 31 securite : 0 normal, 1 proche d'un seuil, 2 seuil depasse
  uint8_t  LastError;           // 32 derniere erreur memorisee (0 aucune, 1-16 courant voie N, 17-18 temp IC9/IC18, 19 +12P)
  uint16_t LastErrorValue;      // 33 valeur mesuree au moment de l'erreur (meme unite que la mesure)
  uint8_t  FuseStatus1;         // 35 fusibles ch1-8  (1 bit par voie, 1 = fusible coupé)
  uint8_t  FuseStatus2;         // 36 fusibles ch9-16 (1 bit par voie, 1 = fusible coupé)
  uint8_t  OutputStatus1;       // 37 état réel des voies 1-8  (1 bit par voie, 1 = allumée)
  uint8_t  OutputStatus2;       // 38 état réel des voies 9-16
  uint8_t  CANInputs1;          // 39 entrées CAN virtuelles 1-8  (1 bit par entrée)
  uint8_t  CANInputs2;          // 40 entrées CAN virtuelles 9-16
  uint8_t  CANGroups;           // 41 groupes activés par CAN (1 bit par groupe)
  uint16_t CANTraffic;          // 42 trames CAN reçues par seconde
  CANFrame CANSniff[10];        // 44 sniffer : 10 dernières trames reçues (0 = la plus récente), 11 octets chacune
  uint8_t  SniffPause;          // 154 sniffer en pause (1) ou actif (0)
  uint8_t  CANError;            // 155 1 = trame reçue sur un ID BZO avec une syntaxe incorrecte (tenu CAN_ERROR_HOLD ms)
};

struct config1  configPage1;
struct config2  configPage2;
struct statuses currentStatus;

//************************************************************************************************************
// Log TS : liste de ce qu'on envoie a TS, octet par octet ([OutputChannels] du .ini)
// Offsets 0 a 5 : bloc standard BZO_TS du .ini (secl, statut, erreurs, mode test, boucles/s).
// Pour ajouter une valeur : un "case" ici + la ligne dans [OutputChannels] du .ini
// + mettre TS_LOG_TAILLE et ochBlockSize a la nouvelle taille.

#define TS_LOG_TAILLE 156       // = ochBlockSize du .ini

uint8_t getTSLogEntry(uint16_t offset)
{
  uint8_t valeur = 0;

  switch (offset)
  {
    case 0:  valeur = currentStatus.secl;                    break; // secondes
    case 1:  valeur = currentStatus.statutTS;                break; // statut
    case 2:  valeur = currentStatus.erreursTS;               break; // erreurs de com
    case 3:  valeur = currentStatus.TestMode;                break; // mode test
    case 4:  valeur = lowByte(currentStatus.boucles);        break; // boucles/s (U16)
    case 5:  valeur = highByte(currentStatus.boucles);       break;
    case 6:   valeur = currentStatus.DigitalStatus1;         break;
    case 7:   valeur = currentStatus.DigitalStatus2;         break;
    case 8:   valeur = currentStatus.Current_ch1;            break;
    case 9:   valeur = currentStatus.Current_ch2;            break;
    case 10:  valeur = currentStatus.Current_ch3;            break;
    case 11:  valeur = currentStatus.Current_ch4;            break;
    case 12:  valeur = currentStatus.Current_ch5;            break;
    case 13:  valeur = currentStatus.Current_ch6;            break;
    case 14:  valeur = currentStatus.Current_ch7;            break;
    case 15:  valeur = currentStatus.Current_ch8;            break;
    case 16:  valeur = currentStatus.Current_ch9;            break;
    case 17:  valeur = currentStatus.Current_ch10;           break;
    case 18:  valeur = currentStatus.Current_ch11;           break;
    case 19:  valeur = currentStatus.Current_ch12;           break;
    case 20:  valeur = lowByte(currentStatus.Current_ch13);  break;
    case 21:  valeur = highByte(currentStatus.Current_ch13); break;
    case 22:  valeur = lowByte(currentStatus.Current_ch14);  break;
    case 23:  valeur = highByte(currentStatus.Current_ch14); break;
    case 24:  valeur = lowByte(currentStatus.Current_ch15);  break;
    case 25:  valeur = highByte(currentStatus.Current_ch15); break;
    case 26:  valeur = lowByte(currentStatus.Current_ch16);  break;
    case 27:  valeur = highByte(currentStatus.Current_ch16); break;
    case 28:  valeur = currentStatus.BoardTemp1;             break;
    case 29:  valeur = currentStatus.BoardTemp2;             break;
    case 30:  valeur = currentStatus.Battery10;              break;
    case 31:  valeur = currentStatus.SafetyStatus;           break;
    case 32:  valeur = currentStatus.LastError;              break;
    case 33:  valeur = lowByte(currentStatus.LastErrorValue); break;
    case 34:  valeur = highByte(currentStatus.LastErrorValue); break;
    case 35:  valeur = currentStatus.FuseStatus1;              break;
    case 36:  valeur = currentStatus.FuseStatus2;              break;
    case 37:  valeur = currentStatus.OutputStatus1;            break;
    case 38:  valeur = currentStatus.OutputStatus2;            break;
    case 39:  valeur = currentStatus.CANInputs1;               break;
    case 40:  valeur = currentStatus.CANInputs2;               break;
    case 41:  valeur = currentStatus.CANGroups;                break;
    case 42:  valeur = lowByte(currentStatus.CANTraffic);      break;
    case 43:  valeur = highByte(currentStatus.CANTraffic);     break;
    case 154: valeur = currentStatus.SniffPause;               break;
    case 155: valeur = currentStatus.CANError;                 break;
  }

  // sniffer CAN : 10 trames de 11 octets à partir de l'offset 44 (ID sur 2 octets, longueur, 8 données)
  if (offset >= 44 && offset < 154){
    uint8_t trame = (offset - 44) / 11;      // quelle trame (0 à 9)
    uint8_t place = (offset - 44) % 11;      // quel octet dans la trame (0 à 10)
    if      (place == 0) {valeur = lowByte(currentStatus.CANSniff[trame].id);}
    else if (place == 1) {valeur = highByte(currentStatus.CANSniff[trame].id);}
    else if (place == 2) {valeur = currentStatus.CANSniff[trame].len;}
    else                 {valeur = currentStatus.CANSniff[trame].data[place - 3];}
  }

  return valeur;
}
