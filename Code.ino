//biblioteke potrebne za rad
#include<dht.h>
#include<IRremote.h>
#include <Wire.h> 
#include<DS1302.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <SD.h>
#include <EEPROM.h>

LiquidCrystal_I2C lcd(0x27,16,2);
#define DHT11_PIN 22
decode_results results;
dht DHT;
DS1302 rtc(2,3,4);
const int chipSelect = 53;

//incijalizacija pinova sa senzorima
int RECV_PIN=7;
IRrecv irrecv(RECV_PIN);
int SoilSensor=A11;
int LightSensor=A9;
int Pump=A15;
int Button1=14;
int Button2=8;
int Button3=17;                  
int led=12;
int pin_pot=A7;

//incijalizacija pomocnih varijabli
int SoilHumidity=0;
int chk=0;
int SoilValue=0;
int LightSensorValue=0;
int LightValue=0;
int button1state=0;
int button2state=0;
int button3state=0;
int state=0;
int logging=0;
int lcd_print=0;
int AD_value=0;
int interval=0;
int address=0;
long int period=0;
unsigned long currentMillis;
unsigned long previousMillis=0;

//kreiranje znaka za stupanjCelzijus
byte stupanjCelzijus[] = {
  0x07,0x05,0x07,0x00,0x00,0x00,0x00,0x00
};
//kreirenje znaka za postotak
byte Postotak[] = {
  0x18,0x19,0x02,0x04,0x08,0x10,0x03,0x03
};
//kreiranje znaka za ugasenu bazu podataka
byte bazaUgasena[] = {
  0x00,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x00
};
//kreiranje znaka za upaljenu bazu podataka
byte bazaUpaljena[] = {
  0x10,0x18,0x1C,0x1E,0x1E,0x1C,0x18,0x10
};

void setup() {
  Serial.begin(9600);
  
  irrecv.enableIRIn();
  
  lcd.init();  
  lcd.backlight();
  lcd.setCursor(1,0);
  
  pinMode(Button1,INPUT_PULLUP);
  pinMode(Button2,INPUT_PULLUP);
  pinMode(Button3,INPUT_PULLUP);
  pinMode(pin_pot,INPUT);
  pinMode(led,OUTPUT);
  pinMode(SoilSensor,INPUT);
  pinMode(LightSensor,INPUT);
  pinMode(Pump,OUTPUT);

  rtc.halt(false);
  rtc.writeProtect(false);
  //postavljanje vremena na sat, nakon početnog postavljanja je zakoentirano
  //rtc.setDOW(WEDNESDAY);
  //rtc.setTime(20,32,0);
  //rtc.setDate(12,02,2020);

  //kreiranje specijalnih znakova
  lcd.createChar(0,stupanjCelzijus);
  lcd.createChar(1,Postotak);
  lcd.createChar(2,bazaUgasena);
  lcd.createChar(3,bazaUpaljena);
  
  //inicijaliziranje SD kartice
  while (!Serial) 
  { 
  }
  if (!SD.begin(chipSelect)) 
  {
    while (1);
  }
  Serial.println("Kartica je inicijalizirana i spremna za rad");

  if(EEPROM.read(address)!=1)
  {
  //kreiranje zaglavlja za dokument
  File dataFile = SD.open("datalog.txt", FILE_WRITE);     //otvaranje datoteke za upis podataka
  if (dataFile)
  {
    dataFile.print("Datum i vrijeme: ");
    dataFile.print("\t\t\t");
    dataFile.print("Vlaga tla: ");
    dataFile.print("\t");
    dataFile.print("Temperatura zraka: ");
    dataFile.print("\t");
    dataFile.print("Vlaga zraka: ");
    dataFile.print("\t\t");
    dataFile.print("Svjetlost: ");
    dataFile.print("\t");
    dataFile.print("\n");
    dataFile.close();
    EEPROM.write(address,1);
  }
  else
  {
    Serial.println("Pogreška kod otvaranja datoteke datalog.txt");    //ispis poruke o pogrešci kod otvaranja datoteke za rad
  }
  }
  
}

void loop() {
  currentMillis=millis();
  
  SoilValue=analogRead(SoilSensor);                          //dohvat podataka sa senzora vlage tla
  LightSensorValue=analogRead(LightSensor);                  //dohvat podataka sa senzora svjetline
  chk=DHT.read11(DHT11_PIN);                                 //dohvat podataka sa DHT 11 senzora
  AD_value=analogRead(pin_pot);                              //očitanje stanja potenciometra
  
  button1state=digitalRead(Button1);                         //očitanje stanja tipkala
  button2state=digitalRead(Button2);                         //očitanje stanja tipkala
  button3state=digitalRead(Button3);                         //očitanje stanja tipkala

  LightValue=map(LightSensorValue,800,5,0,100);              //kalibriranje podataka za svijetlost   
  SoilHumidity=map(SoilValue,1100,400,1,100);                //kalibriranje podataka za vlagu tla
  interval=map(AD_value,0,1023,0,100);                       //postavljanje intervala u kojem će se podatci spremati u bazu podataka
  
  if(interval<=10) period=1000;
  if(interval>10&&interval<=20)  period=2000;
  if(interval>20&&interval<=30)  period=3000;
  if(interval>30&&interval<=40)  period=5000;
  if(interval>40&&interval<=50)  period=60000;
  if(interval>50&&interval<=60)  period=120000;
  if(interval>60&&interval<=70)  period=180000;
  if(interval>70&&interval<=80)  period=300000;
  if(interval>80&&interval<=90)  period=600000;
  if(interval>90&&interval<=100) period=1200000;
  

  
  if (button1state==0)                                       //uključivanje/isključivanje pumpe za navodnjavanje
  {
    state=!state;
  }
  if(button2state==0)                                        //promjena ispisa na lcd zaslonu
  {
    lcd_print=!lcd_print;
  }
  if(button3state==0)                                       //uključivanje/isključivanje pohranjivanja podataka u bazu
  {
    logging=!logging;
  }

  //kod za upotrebu daljinskog upravljača
  if (irrecv.decode(&results))
  {
    Serial.println(results.value,DEC);
    if(results.value==3238126971)                           //ukoliko je pritisnuta 0 isključuje pumpu za navodnjavanje 
    {
      state=HIGH;
    }
    if(results.value==2534850111)                          //ukoliko je pritisnuta 1 uključuje pumpu za navodnjavanje
    {
      state=LOW;
    }

  }
  irrecv.resume();                                       //priprema senzor za ponovni prihvat podataka

  //paljenje/gašenje pumpe za navodnjavanje
  if(state==LOW)
  {
    analogWrite(Pump,LOW);
    digitalWrite(led,HIGH);
  }
  else
  {
    analogWrite(Pump,HIGH);
    digitalWrite(led,LOW);
  }

  //ispis jednog profila na LCD zaslonu
  if(lcd_print==0)
  {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Vlaga tla: ");
  lcd.print(SoilHumidity);
  lcd.write(1);
  lcd.print(" ");
  lcd.setCursor(0,1);
  lcd.print("Light: ");
  lcd.print(LightValue);
  lcd.print("lux");
  }

  //ispis drugog profila na LCD zaslonu
  if(lcd_print==1)
  {
    lcd.clear();
    lcd.print("Temp: ");
    lcd.print(DHT.temperature);
    lcd.write(0);
    lcd.print("   ");
    if(logging==0)       lcd.write(2);                //ispis znaka da je baza ugašena
    else                 lcd.write(3);                //ispis znaka da je baza upaljena
    lcd.setCursor(0,1);
    lcd.print("Hum: ");
    lcd.print(DHT.humidity);
    lcd.write(1);
    int vrijeme=Usporedba(period);                 //ipis intervala pohrane
    lcd.print(" ");
    lcd.print(vrijeme);
    lcd.print("s");
  }
  if(logging==HIGH)
  {
    if(currentMillis-previousMillis>=period)
    {
  File dataFile = SD.open("datalog.txt", FILE_WRITE);    //otvaranje datoteke za upis podataka
  if (dataFile)                                         //upis podataka u datoteku
  {
    dataFile.print(rtc.getDOWStr());
    dataFile.print(" ");
    dataFile.print(rtc.getDateStr());
    dataFile.print(" - ");
    dataFile.print(rtc.getTimeStr());
    dataFile.print("\t\t");
    dataFile.print(SoilHumidity);
    dataFile.print("\t\t");
    dataFile.print(DHT.temperature);
    dataFile.print("\t\t\t");
    dataFile.print(DHT.humidity);
    dataFile.print("\t\t\t");
    dataFile.print(LightValue);
    dataFile.print("\n");
    dataFile.close();
  }
  else
  {
    Serial.println("Pogreška kod otvaranja datoteke datalog.txt");    //ispis poruke o pogrešci prilikom otvaranja datoteke
  }
  previousMillis=currentMillis;
  }
  }

  //ispis svih parametara na Serial monitoru za analizu
  Serial.print("Vrijednost vlage tla: ");
  Serial.println(SoilHumidity);
  Serial.print("Vrijednost svijetlosti: ");
  Serial.println(LightValue);
  Serial.print("Temperatura: ");
  Serial.println(DHT.temperature);
  Serial.print("Vlaga: ");
  Serial.println(DHT.humidity);
  Serial.print("Stanje pumpe: ");
  Serial.println(state);
  Serial.print("Stanje ispisa: ");
  Serial.println(lcd_print);
  Serial.print(rtc.getDOWStr());
  Serial.print("\t");
  Serial.print(rtc.getDateStr());
  Serial.print("\t");
  Serial.println(rtc.getTimeStr());
  Serial.print("Interval: ");
  Serial.println(interval);
  Serial.print("Stanje baze podataka: ");
  Serial.println(logging);
  delay(1000);  //pauziranje sustava od 1s zbog optimalnog rada
}

int Usporedba(int temp){             //funkcija koja pretvara vrijeme intervala iz ms u s 
  int vrijeme;
  switch(temp)
  {
    case(1000):
      return vrijeme=1;
      break;
    case(2000):
      return vrijeme=2;
      break;
    case(3000):
      return vrijeme=3;
      break;
    case(5000):
      return vrijeme=5;
      break;
    case(60000):
      return vrijeme=10;
      break;
    case(120000):
      return vrijeme=30;
      break;
    case(180000):
      return vrijeme=60;
      break;
    case(300000):
      return vrijeme=120;
      break;
    case(600000):
      return vrijeme=300;
      break;
    case(1200000):
      return vrijeme=600;
      break;
    default: 
      return vrijeme=0;
      break;
  }
}
