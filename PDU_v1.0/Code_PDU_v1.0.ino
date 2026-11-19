/***********************************************************************************************************
 * BZO PDU v1.0 - firmware du processeur principal (distributeur de puissance 16 voies)
 *
 * Documentation complète : GUIDE_IA_BZO_PDU.md (dans ce dossier) - à lire avant toute modification.
 * Réglages TunerStudio : BZO_PDU_v1.0.ini (à garder synchronisé avec globals.h).
 *
 * Outils : Arduino IDE 1.8.19, core STM32 2.4.0, bibliothèque STM32_CAN (pazi88).
 *
 * ORDRE DES #include OBLIGATOIRE : chaque fichier utilise ce qui est déclaré avant lui.
 *   BZO_TS.h             communication TunerStudio (commune à plusieurs cartes, ne pas modifier)
 *   globals.h            macros BIT_..., réglages (configPage1/2), temps réel (currentStatus), log TS
 *   Outputs_management.h broches des sorties, modes, fonction mère ManageOutputs()
 *   TS_Commands.h        boutons TunerStudio (mode test, ON/OFF manuels, erreurs, fusibles)
 *   DigitalReadings.h    lecture des 16 entrées
 *   AnalogReadings.h     mesures du processeur de mesure (courants, températures, tension)
 *   Security.h           seuils absolus, fusibles, LEDs rouges, LED status, ligne MCU_OK
 *   CAN_Commands.h       bac à sable CAN (modes, ID, réception, émission, sniffer)
 *
 * TS_SIGNATURE doit être identique à la ligne "signature" du .ini.
 * TS_EEPROM_VERSION : à incrémenter (décision du propriétaire du projet) quand une structure de page change.
 **********************************************************************************************************/

#define TS_SIGNATURE      "BZO GPIO PDU v1.0"   // = "signature" du .ini
#define TS_PROTOCOLE_CRC                     // gros MCU
#define TS_EEPROM_VERSION 1                    // +1 à chaque changement de config1 / config2
#include "BZO_TS.h"        
#include "globals.h"  
#include "Outputs_management.h"
#include "TS_Commands.h"       
#include "DigitalReadings.h"  
#include "AnalogReadings.h"
#include "Security.h"
#include "CAN_Commands.h"

HardwareSerial SerialTS(PA10, PA9);   // liaison PC (TunerStudio) : RX PA10, TX PA9 ("Serial" seul est sur PA3/PA2, la liaison ADC)

void setup() {
  InitSecurity();      // en premier : le superviseur d'alim ne laisse que 2 s pour monter MCU_OK
  TS.addPage(1, configPage1);
  TS.addPage(2, configPage2);
  TS.setOutputs(getTSLogEntry, TS_LOG_TAILLE, currentStatus);
  InitCommands();
  TS.begin(SerialTS, 115200);   // relit les réglages en EEPROM : les Init qui s'en servent viennent après
  InitDigitals();
  InitCAN();
  InitOutputs();
}

void loop(){
  TS.update();
  ReadDigitals();
  ReadAnalogs();
  CheckSecurity();
  ReadCAN();
  ManageOutputs();
  SendCAN();
}
