/***********************************************************************************************************
 * TS_Commands.h - boutons TunerStudio (commandes directes "E\xGG\xNN" du .ini)
 *
 *   - mode test : StartTest() coupe tout et donne la main aux boutons ON/OFF,
 *     EndTest() coupe tout et rend la main à la fonction mère
 *   - OutON_N() / OutOFF_N() : pilotage manuel de la voie N (sans effet hors mode test,
 *     ON refusé si le fusible de la voie est coupé)
 *   - ClearError() : efface la dernière erreur mémorisée (Security.h)
 *
 * POUR AJOUTER UN BOUTON : le prototype en haut, TS.addCommand(0xGGNN, Fonction) dans InitCommands(),
 * la fonction sous "déclarer ensuite chaque fonction commandes ici", et la ligne "E\xGG\xNN" dans
 * [ControllerCommands] du .ini. Groupe 0xFF réservé à BZO_TS.h. Pas de limite de nombre.
 **********************************************************************************************************/

  
uint8_t ManualStatus1 = 0;      // état manuel ch1-8  (1 bit par voie)
uint8_t ManualStatus2 = 0;      // état manuel ch9-16 (1 bit par voie)

void ClearError();                     // prototypes des fonctions déclarées plus bas
void ResetFuses();                     // dans Security.h
void StartTest();  void EndTest();
void SniffStop();  void SniffGo();
void OutON_1();    void OutOFF_1();
void OutON_2();    void OutOFF_2();
void OutON_3();    void OutOFF_3();
void OutON_4();    void OutOFF_4();
void OutON_5();    void OutOFF_5();
void OutON_6();    void OutOFF_6();
void OutON_7();    void OutOFF_7();
void OutON_8();    void OutOFF_8();
void OutON_9();    void OutOFF_9();
void OutON_10();   void OutOFF_10();
void OutON_11();   void OutOFF_11();
void OutON_12();   void OutOFF_12();
void OutON_13();   void OutOFF_13();
void OutON_14();   void OutOFF_14();
void OutON_15();   void OutOFF_15();
void OutON_16();   void OutOFF_16();
  
void InitCommands(){
  
    TS.addCommand(0x0110, ClearError);   // efface la dernière erreur mémorisée (Security.h)
    TS.addCommand(0x0111, ResetFuses);   // réarme tous les fusibles tout de suite (Security.h)
    TS.addCommand(0x0112, SniffStop);    // sniffer CAN en pause (affichage figé)
    TS.addCommand(0x0113, SniffGo);      // sniffer CAN relancé

    TS.addCommand(0x0401, StartTest);    // mode manuel : tout coupé, les voies se pilotent une par une
    TS.addCommand(0x0402, EndTest);      // fin du mode manuel : tout coupé, retour au fonctionnement normal

    TS.addCommand(0x0201, OutON_1);    TS.addCommand(0x0301, OutOFF_1);
    TS.addCommand(0x0202, OutON_2);    TS.addCommand(0x0302, OutOFF_2);
    TS.addCommand(0x0203, OutON_3);    TS.addCommand(0x0303, OutOFF_3);
    TS.addCommand(0x0204, OutON_4);    TS.addCommand(0x0304, OutOFF_4);
    TS.addCommand(0x0205, OutON_5);    TS.addCommand(0x0305, OutOFF_5);
    TS.addCommand(0x0206, OutON_6);    TS.addCommand(0x0306, OutOFF_6);
    TS.addCommand(0x0207, OutON_7);    TS.addCommand(0x0307, OutOFF_7);
    TS.addCommand(0x0208, OutON_8);    TS.addCommand(0x0308, OutOFF_8);
    TS.addCommand(0x0209, OutON_9);    TS.addCommand(0x0309, OutOFF_9);
    TS.addCommand(0x020A, OutON_10);   TS.addCommand(0x030A, OutOFF_10);
    TS.addCommand(0x020B, OutON_11);   TS.addCommand(0x030B, OutOFF_11);
    TS.addCommand(0x020C, OutON_12);   TS.addCommand(0x030C, OutOFF_12);
    TS.addCommand(0x020D, OutON_13);   TS.addCommand(0x030D, OutOFF_13);
    TS.addCommand(0x020E, OutON_14);   TS.addCommand(0x030E, OutOFF_14);
    TS.addCommand(0x020F, OutON_15);   TS.addCommand(0x030F, OutOFF_15);
    TS.addCommand(0x0210, OutON_16);   TS.addCommand(0x0310, OutOFF_16);
}

  //déclarer ensuite chaque fonction commandes ici

void SaveError(uint8_t code, uint16_t value);   // dans Security.h

void ClearError(){ SaveError(0, 0); }


// coupe les 16 voies et remet les états manuels à 0
void AllOutputsOFF(){
  digitalWrite(OUT_CH1, LOW);
  digitalWrite(OUT_CH2, LOW);
  digitalWrite(OUT_CH3, LOW);
  digitalWrite(OUT_CH4, LOW);
  digitalWrite(OUT_CH5, LOW);
  digitalWrite(OUT_CH6, LOW);
  digitalWrite(OUT_CH7, LOW);
  digitalWrite(OUT_CH8, LOW);
  digitalWrite(OUT_CH9, LOW);
  digitalWrite(OUT_CH10, LOW);
  digitalWrite(OUT_CH11, LOW);
  digitalWrite(OUT_CH12, LOW);
  digitalWrite(OUT_CH13, LOW);
  digitalWrite(OUT_CH14, LOW);
  digitalWrite(OUT_CH15, LOW);
  digitalWrite(OUT_CH16, LOW);
  ManualStatus1 = 0;
  ManualStatus2 = 0;
}

void StartTest(){ AllOutputsOFF(); currentStatus.TestMode = 1; }
void EndTest()  { AllOutputsOFF(); currentStatus.TestMode = 0; }

// sniffer CAN : pause / reprise (le trafic continue d'être compté)
void SniffStop() { currentStatus.SniffPause = 1; }
void SniffGo()   { currentStatus.SniffPause = 0; }

// commandes manuelles : sans effet hors mode test, ON refusé si le fusible de la voie est coupé
void OutON_1()   { if (currentStatus.TestMode == 1 && BIT_CHECK(currentStatus.FuseStatus1, 0) == false) {digitalWrite(OUT_CH1, HIGH);  BIT_SET(ManualStatus1, 0);} }
void OutOFF_1()  { if (currentStatus.TestMode == 1) {digitalWrite(OUT_CH1, LOW);   BIT_CLEAR(ManualStatus1, 0);} }
void OutON_2()   { if (currentStatus.TestMode == 1 && BIT_CHECK(currentStatus.FuseStatus1, 1) == false) {digitalWrite(OUT_CH2, HIGH);  BIT_SET(ManualStatus1, 1);} }
void OutOFF_2()  { if (currentStatus.TestMode == 1) {digitalWrite(OUT_CH2, LOW);   BIT_CLEAR(ManualStatus1, 1);} }
void OutON_3()   { if (currentStatus.TestMode == 1 && BIT_CHECK(currentStatus.FuseStatus1, 2) == false) {digitalWrite(OUT_CH3, HIGH);  BIT_SET(ManualStatus1, 2);} }
void OutOFF_3()  { if (currentStatus.TestMode == 1) {digitalWrite(OUT_CH3, LOW);   BIT_CLEAR(ManualStatus1, 2);} }
void OutON_4()   { if (currentStatus.TestMode == 1 && BIT_CHECK(currentStatus.FuseStatus1, 3) == false) {digitalWrite(OUT_CH4, HIGH);  BIT_SET(ManualStatus1, 3);} }
void OutOFF_4()  { if (currentStatus.TestMode == 1) {digitalWrite(OUT_CH4, LOW);   BIT_CLEAR(ManualStatus1, 3);} }
void OutON_5()   { if (currentStatus.TestMode == 1 && BIT_CHECK(currentStatus.FuseStatus1, 4) == false) {digitalWrite(OUT_CH5, HIGH);  BIT_SET(ManualStatus1, 4);} }
void OutOFF_5()  { if (currentStatus.TestMode == 1) {digitalWrite(OUT_CH5, LOW);   BIT_CLEAR(ManualStatus1, 4);} }
void OutON_6()   { if (currentStatus.TestMode == 1 && BIT_CHECK(currentStatus.FuseStatus1, 5) == false) {digitalWrite(OUT_CH6, HIGH);  BIT_SET(ManualStatus1, 5);} }
void OutOFF_6()  { if (currentStatus.TestMode == 1) {digitalWrite(OUT_CH6, LOW);   BIT_CLEAR(ManualStatus1, 5);} }
void OutON_7()   { if (currentStatus.TestMode == 1 && BIT_CHECK(currentStatus.FuseStatus1, 6) == false) {digitalWrite(OUT_CH7, HIGH);  BIT_SET(ManualStatus1, 6);} }
void OutOFF_7()  { if (currentStatus.TestMode == 1) {digitalWrite(OUT_CH7, LOW);   BIT_CLEAR(ManualStatus1, 6);} }
void OutON_8()   { if (currentStatus.TestMode == 1 && BIT_CHECK(currentStatus.FuseStatus1, 7) == false) {digitalWrite(OUT_CH8, HIGH);  BIT_SET(ManualStatus1, 7);} }
void OutOFF_8()  { if (currentStatus.TestMode == 1) {digitalWrite(OUT_CH8, LOW);   BIT_CLEAR(ManualStatus1, 7);} }
void OutON_9()   { if (currentStatus.TestMode == 1 && BIT_CHECK(currentStatus.FuseStatus2, 0) == false) {digitalWrite(OUT_CH9, HIGH);  BIT_SET(ManualStatus2, 0);} }
void OutOFF_9()  { if (currentStatus.TestMode == 1) {digitalWrite(OUT_CH9, LOW);   BIT_CLEAR(ManualStatus2, 0);} }
void OutON_10()  { if (currentStatus.TestMode == 1 && BIT_CHECK(currentStatus.FuseStatus2, 1) == false) {digitalWrite(OUT_CH10, HIGH);  BIT_SET(ManualStatus2, 1);} }
void OutOFF_10() { if (currentStatus.TestMode == 1) {digitalWrite(OUT_CH10, LOW);  BIT_CLEAR(ManualStatus2, 1);} }
void OutON_11()  { if (currentStatus.TestMode == 1 && BIT_CHECK(currentStatus.FuseStatus2, 2) == false) {digitalWrite(OUT_CH11, HIGH);  BIT_SET(ManualStatus2, 2);} }
void OutOFF_11() { if (currentStatus.TestMode == 1) {digitalWrite(OUT_CH11, LOW);  BIT_CLEAR(ManualStatus2, 2);} }
void OutON_12()  { if (currentStatus.TestMode == 1 && BIT_CHECK(currentStatus.FuseStatus2, 3) == false) {digitalWrite(OUT_CH12, HIGH);  BIT_SET(ManualStatus2, 3);} }
void OutOFF_12() { if (currentStatus.TestMode == 1) {digitalWrite(OUT_CH12, LOW);  BIT_CLEAR(ManualStatus2, 3);} }
void OutON_13()  { if (currentStatus.TestMode == 1 && BIT_CHECK(currentStatus.FuseStatus2, 4) == false) {digitalWrite(OUT_CH13, HIGH);  BIT_SET(ManualStatus2, 4);} }
void OutOFF_13() { if (currentStatus.TestMode == 1) {digitalWrite(OUT_CH13, LOW);  BIT_CLEAR(ManualStatus2, 4);} }
void OutON_14()  { if (currentStatus.TestMode == 1 && BIT_CHECK(currentStatus.FuseStatus2, 5) == false) {digitalWrite(OUT_CH14, HIGH);  BIT_SET(ManualStatus2, 5);} }
void OutOFF_14() { if (currentStatus.TestMode == 1) {digitalWrite(OUT_CH14, LOW);  BIT_CLEAR(ManualStatus2, 5);} }
void OutON_15()  { if (currentStatus.TestMode == 1 && BIT_CHECK(currentStatus.FuseStatus2, 6) == false) {digitalWrite(OUT_CH15, HIGH);  BIT_SET(ManualStatus2, 6);} }
void OutOFF_15() { if (currentStatus.TestMode == 1) {digitalWrite(OUT_CH15, LOW);  BIT_CLEAR(ManualStatus2, 6);} }
void OutON_16()  { if (currentStatus.TestMode == 1 && BIT_CHECK(currentStatus.FuseStatus2, 7) == false) {digitalWrite(OUT_CH16, HIGH);  BIT_SET(ManualStatus2, 7);} }
void OutOFF_16() { if (currentStatus.TestMode == 1) {digitalWrite(OUT_CH16, LOW);  BIT_CLEAR(ManualStatus2, 7);} }
