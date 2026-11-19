/***********************************************************************************************************
 * AnalogReadings.h - mesures analogiques fournies par le processeur de mesure
 *
 * Toutes les TIMEOUT_ADC ms, on envoie ADC_ID ('0') au processeur de mesure qui répond 19 octets (ADC 8 bits, réf 5 V).
 * Les valeurs brutes vont dans currentStatus (IntensityADC_chN, BoardTempADC1/2, BatteryADC),
 * puis sont converties en unités réelles (Current_chN en 0,1 A, BoardTemp en °C, Battery10 en 0,1 V).
 * À chaque trame reçue : newADC = true (lu par CheckSecurity() dans Security.h).
 *
 * Les coefficients SENSE_K_1_12 / SENSE_K_13_16 sont des valeurs typiques : à recaler au banc si besoin.
 * SENSE_RS_13_16 = résistances de mesure R11-R14 réellement montées (3,9k).
 **********************************************************************************************************/

#define TIMEOUT_ADC 50          // ms
#define ADC_ID      '0'         // appel envoyé au processeur de mesure

// Conversions (ADC 8 bits, référence VDD 5 V : 1 pas = 5000/255 = 19,6 mV)
#define SENSE_K_1_12    2700UL    // ratio de mesure K voies 1-12 (typique)
#define SENSE_RS_1_12   1200UL    // résistance de mesure voies 1-12 RN6/RN7/RN10 (ohms)
#define SENSE_K_13_16   52100UL   // ratio de mesure K voies 13-16 (typique)
#define SENSE_RS_13_16  3900UL    // résistance de mesure voies 13-16 R11-R14 (ohms)

HardwareSerial SerialADC(PA3, PA2);

bool initADC = false;
int EraseADC = 0;
bool wait_for_ADC = false;
uint32_t begin_timeoutADC = 0;
bool newADC = false;            // nouvelle trame recue (lue par CheckSecurity)

void getADC(){

//envoie de l'appel
  if (wait_for_ADC == false && TIMEOUT_ADC < (millis() - begin_timeoutADC)){
    SerialADC.write(ADC_ID);
    wait_for_ADC = true;
    begin_timeoutADC = millis();
  }

//cas ou on a des octets présents alors qu'on a pas fait d'appel
  if (SerialADC.available() > 0 && wait_for_ADC == false){
    while (SerialADC.available() > 0){
      EraseADC = SerialADC.read();
    }
  }

//cas ou on reçois un mauvais message après l'appel, mauvais appel, ou délai dépassé
  if (SerialADC.available() != 19 && wait_for_ADC == true && TIMEOUT_ADC < (millis() - begin_timeoutADC)){
    while (SerialADC.available() > 0){
      EraseADC = SerialADC.read();
    }
    wait_for_ADC = false;
  }

//cas OK, on reçois le message correspondant a l'appel
  if (SerialADC.available() == 19 && wait_for_ADC == true){

         currentStatus.IntensityADC_ch6 = SerialADC.read();//ADC_A1
         currentStatus.IntensityADC_ch7 = SerialADC.read();//ADC_A2
         currentStatus.IntensityADC_ch8 = SerialADC.read();//ADC_A3 
         currentStatus.IntensityADC_ch9 = SerialADC.read();//ADC_A4
         currentStatus.IntensityADC_ch10 = SerialADC.read();//ADC_A5
         currentStatus.IntensityADC_ch11 = SerialADC.read();//ADC_A6
         currentStatus.IntensityADC_ch12 = SerialADC.read();//ADC_A7
         currentStatus.BoardTempADC1 = SerialADC.read();//ADC_A16
         currentStatus.BoardTempADC2 = SerialADC.read();//ADC_A17 
         currentStatus.BatteryADC = SerialADC.read();//ADC_A22
         currentStatus.IntensityADC_ch16 = SerialADC.read();//ADC_A23
         currentStatus.IntensityADC_ch15 = SerialADC.read();//ADC_A24
         currentStatus.IntensityADC_ch14 = SerialADC.read();//ADC_A25
         currentStatus.IntensityADC_ch13 = SerialADC.read();//ADC_A26 
         currentStatus.IntensityADC_ch1 = SerialADC.read();//ADC_A27
         currentStatus.IntensityADC_ch2 = SerialADC.read();//ADC_A28
         currentStatus.IntensityADC_ch3 = SerialADC.read();//ADC_A29
         currentStatus.IntensityADC_ch4 = SerialADC.read();//ADC_A30
         currentStatus.IntensityADC_ch5 = SerialADC.read();//ADC_A31

         //conversions
         // voies 1-12 : I = V x K / Rsense -> x0.1 A
         currentStatus.Current_ch1 = (currentStatus.IntensityADC_ch1 * 50UL * SENSE_K_1_12) / (255UL * SENSE_RS_1_12);
         currentStatus.Current_ch2 = (currentStatus.IntensityADC_ch2 * 50UL * SENSE_K_1_12) / (255UL * SENSE_RS_1_12);
         currentStatus.Current_ch3 = (currentStatus.IntensityADC_ch3 * 50UL * SENSE_K_1_12) / (255UL * SENSE_RS_1_12);
         currentStatus.Current_ch4 = (currentStatus.IntensityADC_ch4 * 50UL * SENSE_K_1_12) / (255UL * SENSE_RS_1_12);
         currentStatus.Current_ch5 = (currentStatus.IntensityADC_ch5 * 50UL * SENSE_K_1_12) / (255UL * SENSE_RS_1_12);
         currentStatus.Current_ch6 = (currentStatus.IntensityADC_ch6 * 50UL * SENSE_K_1_12) / (255UL * SENSE_RS_1_12);
         currentStatus.Current_ch7 = (currentStatus.IntensityADC_ch7 * 50UL * SENSE_K_1_12) / (255UL * SENSE_RS_1_12);
         currentStatus.Current_ch8 = (currentStatus.IntensityADC_ch8 * 50UL * SENSE_K_1_12) / (255UL * SENSE_RS_1_12);
         currentStatus.Current_ch9 = (currentStatus.IntensityADC_ch9 * 50UL * SENSE_K_1_12) / (255UL * SENSE_RS_1_12);
         currentStatus.Current_ch10 = (currentStatus.IntensityADC_ch10 * 50UL * SENSE_K_1_12) / (255UL * SENSE_RS_1_12);
         currentStatus.Current_ch11 = (currentStatus.IntensityADC_ch11 * 50UL * SENSE_K_1_12) / (255UL * SENSE_RS_1_12);
         currentStatus.Current_ch12 = (currentStatus.IntensityADC_ch12 * 50UL * SENSE_K_1_12) / (255UL * SENSE_RS_1_12);

         // voies 13-16 : I = V x K / Rsense -> x0.1 A (255 pas = 67 A)
         currentStatus.Current_ch13 = (currentStatus.IntensityADC_ch13 * 50UL * SENSE_K_13_16) / (255UL * SENSE_RS_13_16);
         currentStatus.Current_ch14 = (currentStatus.IntensityADC_ch14 * 50UL * SENSE_K_13_16) / (255UL * SENSE_RS_13_16);
         currentStatus.Current_ch15 = (currentStatus.IntensityADC_ch15 * 50UL * SENSE_K_13_16) / (255UL * SENSE_RS_13_16);
         currentStatus.Current_ch16 = (currentStatus.IntensityADC_ch16 * 50UL * SENSE_K_13_16) / (255UL * SENSE_RS_13_16);

         // capteur de température : T = (1569 mV - V) / 8,2 mV/°C -> °C (0 si negatif)
         int32_t temp1 = (4000950L - (int32_t)currentStatus.BoardTempADC1 * 50000L) / 20910L;
         if (temp1 < 0) {temp1 = 0;}
         if (temp1 > 255) {temp1 = 255;}
         currentStatus.BoardTemp1 = temp1;
         int32_t temp2 = (4000950L - (int32_t)currentStatus.BoardTempADC2 * 50000L) / 20910L;
         if (temp2 < 0) {temp2 = 0;}
         if (temp2 > 255) {temp2 = 255;}
         currentStatus.BoardTemp2 = temp2;

         // +12P : V = Vadc x (82k + 10k) / 10k -> x0.1 V (bloqué à 25,5 V)
         uint16_t batt = (currentStatus.BatteryADC * 460UL) / 255UL;
         if (batt > 255) {batt = 255;}
         currentStatus.Battery10 = batt;

         newADC = true;

    while (SerialADC.available() > 0){
      EraseADC = SerialADC.read();
    }
  begin_timeoutADC = millis();
  wait_for_ADC = false;
  }
}

void ReadAnalogs(){   //fonctionnement normal
  if (initADC == false) {
    SerialADC.begin(115200);
    initADC = true;
    }

  if (initADC == true){getADC();}
}
