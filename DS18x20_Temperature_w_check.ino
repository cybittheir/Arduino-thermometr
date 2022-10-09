#include <OneWire.h>

// OneWire DS18S20, DS18B20, DS1822 Temperature
//
// http://www.pjrc.com/teensy/td_libs_OneWire.html
//
// The DallasTemperature library can do all this work for you!
// http://milesburton.com/Dallas_Temperature_Control_Library

#define POWER_MODE  1; // режим питания, 0 - внешнее, 1 - паразитное

OneWire  ds(18);  // on pin 18 (a 4.7K resistor is necessary)

//Константы пинов сегментов

const int TOP = 2; //A
const int TOP_RIGHT = 3; //B
const int BOTTOM_RIGHT = 4; //C
const int BOTTOM = 5; //D
const int BOTTOM_LEFT = 6; //E
const int TOP_LEFT = 7; //F
const int MIDDLE = 8; //G

const int SIGN = 16; //PLUS
const int CELS = 17; //CELS

const int firstSeg = 2;
const int segments = 16;

int lastSeg = firstSeg + segments;


//включение светодиодов
const int LED_ON = 1;
const int LED_OFF = 0;

//частота замеров
const int freq=5000; //msec

//длительность шага теста
const int ch_delay=150;

//Сдвиг пинов для второго регистра
const int offset = 7;

//для сравнения показаний
int compare = 99;

//Массив пинов для каждой цифры, где первый элемент - длина массива
int nums[][10] = {
  {6,TOP_LEFT,TOP,TOP_RIGHT,BOTTOM_LEFT,BOTTOM,BOTTOM_RIGHT},
  {2,TOP_RIGHT,BOTTOM_RIGHT},
  {5,TOP,TOP_RIGHT,MIDDLE,BOTTOM_LEFT,BOTTOM},
  {5,TOP,TOP_RIGHT,MIDDLE,BOTTOM_RIGHT,BOTTOM},
  {4,TOP_LEFT,TOP_RIGHT,MIDDLE,BOTTOM_RIGHT},
  {5,TOP_LEFT,TOP,MIDDLE,BOTTOM_RIGHT,BOTTOM},
  {6,TOP_LEFT,TOP,MIDDLE,BOTTOM_LEFT,BOTTOM_RIGHT,BOTTOM},
  {3,TOP,TOP_RIGHT,BOTTOM_RIGHT},
  {7,TOP_LEFT,TOP,TOP_RIGHT,MIDDLE,BOTTOM_LEFT,BOTTOM,BOTTOM_RIGHT},
  {6,TOP_LEFT,TOP,TOP_RIGHT,MIDDLE,BOTTOM,BOTTOM_RIGHT}
  };

void setup(void) {
  for(int i=firstSeg;i<lastSeg;i++){
    pinMode(i,OUTPUT);
  }
  Serial.begin(9600);

//циклы тестов при включении
  for (int b=0;b<2;b++){

      //Цикл отключения по всем сегментам
  
  for(int i=firstSeg;i<lastSeg;i++){
    //Выключаем пины
      digitalWrite(i,LED_OFF);
      Serial.print("=");
  }
  
  delay(ch_delay);
  Serial.println("");

  int ci0 = lastSeg;

  //Цикл отключения по всем сегментам
  if (b==0) {
  //Цикл последовательного включения по всем сегментам
     for(int i=firstSeg;i<lastSeg;i++){
  //Включаем пины
       digitalWrite(i,LED_ON);
       Serial.print(i);
       Serial.print("; ");
       delay(ch_delay);
     }

  } else {
  
     int half = (lastSeg - 2 - firstSeg)/2;
     int last2 = half + firstSeg;
  
     for(int i=firstSeg;i<last2;i++){
  //Включаем пины
       digitalWrite(i,LED_ON);
       int i2= i + half;
       digitalWrite(i2,LED_ON);
       Serial.print(i);
       Serial.print("+");
       Serial.print(i2);
       Serial.print("; ");
       delay(ch_delay);
       ci0 = i;
    }

  }

  for(int ci=ci0;ci<lastSeg;ci++){
    digitalWrite(ci,LED_ON);
  }

  delay(ch_delay);

  }
  
  for(int i=firstSeg;i<lastSeg;i++){
  //Выключаем пины
    digitalWrite(i,LED_OFF);
  }

  delay(ch_delay);
  delay(ch_delay);

}

//Включение цифры (index), где при offs == 0 первый регистр, и при offs == offset второй регистр
void printDigit(int index, int offs){
  for(int j=0;j<nums[index][0];j++){
      digitalWrite(nums[index][j+1]+offs,LED_ON);
      }
  }

void printSign (int s){

//показываем '+' при положительных температурах

  if (s>0){
    digitalWrite(SIGN,LED_ON);
  } else {
    digitalWrite(SIGN,LED_OFF);      
  }
}


void loop(void) {
  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  float celsius, fahrenheit;
  int plus = 1;
  int d1 = 0;
  
  if ( !ds.search(addr)) {
 //   Serial.println("No more addresses.");
 //   Serial.println();
    ds.reset_search();
    delay(250);
    return;
  }
  
  Serial.print("ROM =");
  for( i = 0; i < 8; i++) {
    Serial.write(' ');
    Serial.print(addr[i], HEX);
  }

  if (OneWire::crc8(addr, 7) != addr[7]) {
      Serial.println("CRC is not valid!");
      return;
  }
  Serial.println();
 
  // the first ROM byte indicates which chip
  switch (addr[0]) {
    case 0x10:
      Serial.println("  Chip = DS18S20");  // or old DS1820
      type_s = 1;
      break;
    case 0x28:
      Serial.println("  Chip = DS18B20");
      type_s = 0;
      break;
    case 0x22:
      Serial.println("  Chip = DS1822");
      type_s = 0;
      break;
    default:
      Serial.println("Device is not a DS18x20 family device.");
      return;
  } 

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end
  
  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.
  
  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad

  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
  }

  int16_t raw = (data[1] << 8) | data[0];

  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }
  
  celsius = raw / 16.0;

  int disp=round(abs(celsius));
  
  if (celsius<0){
    plus = -1;
  }

  if (plus*disp==compare) {

    Serial.println("показания датчика:");
    Serial.print(celsius);
    Serial.println("C");
    Serial.println("показания индикатора не изменились");
    delay(freq/2);
    
  } else {
  
 //Выключаем пины
   for(int p=firstSeg;p<lastSeg;p++){
      digitalWrite(p,LED_OFF);
   }
 
   //включаем знак градуса
   digitalWrite(CELS,LED_ON);
   
   //Показываем знак
   printSign(plus);

 //Первая цифра
  
  if (disp>=10){
     d1 = disp/10;
 //Включаем первую цифру
     printDigit(d1,0);
  }

 //Вторая цифра
     int d2 = disp%10;

 //Включаем вторую цифру
     printDigit(d2,offset);

//вывод в консоль
    Serial.print("  Temperature = ");
    Serial.print(celsius);
    Serial.println(" C");
    Serial.print(plus);
    Serial.print("* ");
    Serial.println(disp);
    Serial.println("---");
    Serial.print("d1:");
    if (abs(d1)>0){
      Serial.print(":");
      Serial.print(d1);
    }
    else {
      Serial.print("(abs)0");
    }
    Serial.print("; d2:");
    Serial.println(d2);
    Serial.println("===");

    delay(freq);
    compare=disp*plus;
  
  }
}
