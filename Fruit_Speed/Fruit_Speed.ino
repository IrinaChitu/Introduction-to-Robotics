#include <LedControl.h>
#include <LiquidCrystal.h>

#define JOY1_X A0
#define JOY2_X A1
#define JOY3_X A2
#define JOY1_BUTTON 8
#define JOY2_BUTTON 13
#define JOY3_BUTTON A3 //lack of pins. folosim analogii in mod digital
#define V0_PIN 9

const int DIN_PIN = 12; //data in
const int CLK_PIN = 11; //clock
const int CS_PIN = 10; //load/cs

LedControl display = LedControl(DIN_PIN, CLK_PIN, CS_PIN);
LiquidCrystal lcd(2, 3, 4, 5, 6, 7); //pinii la care e conectat LCD-ul

const uint64_t fruits[] = {
  0x003c4242423c1060, //APPLE
  0x001e2222926c0000, //STRAWBERRY
  0x00e7e7e7444850e0, //BANANA
  0x003c7e6060602000, //CHERRY
  0x3c7ee7dbdbe77e3c, //KIWI
  0x003c664242663c00, //ORANGE
  0x3c2424242418183c, //PINEAPPLE
  0x003c424224241800, //PEAR
  0x003c5a5a24241800  //AVOCADO
};
const int fruitLen = sizeof(fruits)/8;

const uint64_t fail = {
  0x8142241818244281 //X
};

const uint64_t success = {
  0x003c428100240000 //:)
};

int joyX[3] = {JOY1_X, JOY2_X, JOY3_X};
int dataX[3]; //xValue, firstJoy
int joyButton[3] = {JOY1_BUTTON, JOY2_BUTTON, JOY3_BUTTON};
int dataButton[3]; //buttonState
int currentPlayer;
int nrPlayers = 1;
int activePlayers;
int scorePlayers[3];
int count; //nr de "carti" de la ultima apasre corecta
bool introduction = false; //false -> asteapta nr jucatori | true -> am terminat cu intro 
int show;
bool joyMoved = false;
int prev = -1; //dubla
int pprev = -1; //sandvis
bool empty = true; //daca s a dat carte jos sau nu
unsigned long currentMillis;
unsigned long previousMillis = 0; //last time "sth happened" | unsigned for time variables for time holders cause the value will quickly become too large for an int to store
const unsigned long debounceDelay = 100; //milliseconds

void myDelay(int ms) {
  previousMillis = millis();
  while (millis() - previousMillis < ms);
}

void readData() {
  for (int i = 0; i < nrPlayers; i++) {
    dataX[i] = analogRead(joyX[i]);  //int firstJoy = analogRead(JOY1_X);
  }
  for (int i = 0; i < nrPlayers; i++) {
    dataButton[i] = digitalRead(joyButton[i]);  //int buttonState = digitalRead(JOY1_BUTTON);
  }
  currentMillis = millis();
}

void gameIntroduction() {
  lcd.clear();
  lcd.setCursor(4, 0);
  lcd.print("Welcome!");
  myDelay(debounceDelay * 20); //delay(2000);
  lcd.clear();
  lcd.print("How many players?");
  myDelay(debounceDelay); //delay(100);
  lcd.setCursor(7, 1);
  lcd.print(nrPlayers);
}

void displayImage(uint64_t image) {
  for (int i = 0; i < 8; i++) {
    byte row = (image >> i * 8) & 0xFF;
    for (int j = 0; j < 8; j++) {
      display.setLed(0, i, j, bitRead(row, j));
    }
  }
  myDelay(debounceDelay / 10); //delay(10);
}

void wrong(int index, int nr) {
  currentPlayer = (index + 1) % nrPlayers;
  displayImage(fail);
  lcd.clear();
  lcd.setCursor(4, 0);
  lcd.print("GOTCHA!!");
  myDelay(debounceDelay * 4); //delay(400);
  lcd.clear();
  scorePlayers[index] -= nr; //se scade punctajul jucatorului care a apasat gresit
  char printline[20] = "Player number ";
  char conv[2];
  itoa(index + 1, conv, 10);
  strcat(printline, conv);
  lcd.print(printline);
  lcd.setCursor(0, 1);
  strcpy(printline, "loses ");
  itoa(nr + 1, conv, 10); //pt ca index de la 0
  strcat(printline, conv);
  if (nr == 0)
    strcat(printline, " point!");
  else
    strcat(printline, " points!");
  lcd.print(printline);
  myDelay(debounceDelay * 15); //delay(1500);
  lcd.clear();
  displayImage(fruits[nr]); //pt ca se mai intampla sa nu dispara x-ul
}

void correct(int index) { 
  currentPlayer = index;
  displayImage(success);
  lcd.clear();
  lcd.setCursor(4, 0);
  lcd.print("CORRECT!");
  myDelay(debounceDelay * 24); //delay(2400);
  lcd.clear();
  scorePlayers[index] += count;
  char printline[20] = "Player number ";
  char conv[2];
  itoa(index + 1, conv, 10);
  strcat(printline, conv);
  lcd.print(printline);
  lcd.setCursor(0, 1);
  strcpy(printline, "gains ");
  itoa(count, conv, 10);
  strcat(printline, conv);
  if (count == 0)
    strcat(printline, " point!");
  else
    strcat(printline, " points!");
  lcd.print(printline);
  myDelay(debounceDelay * 15); //delay(1500);
  lcd.clear();
  count = 0;
  display.clearDisplay(0);
}

void createPlayers() { //sa i se asocieze aici si controlerul
  //scorePlayers = new int[nrPlayers]; //<- alocarea dinamica crapa pe arduino
  for (int i = 0; i < nrPlayers; i++){
    scorePlayers[i] = 20;
  }
  activePlayers = nrPlayers;
}

void playersTurn() {
  //imi afiseaza cand misc pe axa Ox: ordine RANDOM-ish
  if ((dataX[currentPlayer] > 1000 || dataX[currentPlayer] < 100) && joyMoved == false) {
    display.clearDisplay(0);
    myDelay(debounceDelay); //delay(100);
    if (prev == -1)
      prev = show;
    else {
      pprev = prev;
      prev = show;
    }
    currentPlayer++; //pt ca un jucator sa poata da o singura data carte (sa dea pe rand)
    currentPlayer %= nrPlayers;
    show = random(0, fruitLen); //max value is exclusive
    count++;
    joyMoved = true;
    empty = false;
  }
  if (dataX[currentPlayer] < 600 && dataX[currentPlayer] > 400){
    joyMoved = false;
  }
}

void buttonPressed() {
  for (int i = 0; i < nrPlayers; i++) {
    if (dataButton[i] == 0) {
      if (prev == show || pprev == show) { // dubla sau sandvis
        correct(i);
        empty = true;
        prev = pprev = -1;
      }
      else {
        wrong(i, show);
        prev = pprev = -1;
      }
    }
  }
}

void soloMode() { //schimba afisarea cu "you gain x points"
  playersTurn();
  buttonPressed();
  if (empty == false)
      displayImage(fruits[show]);
}

void finish(int index) {
  lcd.setCursor(1, 0);
  lcd.print("The winner is");
  lcd.setCursor(1, 1);
  lcd.print("Player number ");
  char number[2];
  itoa(index + 1, number, 10);
  lcd.print(number);
  display.clearDisplay(0); //stinge matricea
  myDelay(debounceDelay * 24); //delay(10);
  //replay
  //eventual intreaba daca vrea sa joace iar, iar daca nu spune bye bye
  gameIntroduction();
  introduction = false;
}

void verify() { //daca e un sg jucator din start trebuie un alt tip de verificare | mai trebuie lucrat la verificare
  for (int i = 0; i < nrPlayers; i++)
    if (scorePlayers[i] == 0) {
      scorePlayers[i] = -1;
      activePlayers--;
    }
  if (activePlayers == 1) {
    int i;
    for (i = 0; i < nrPlayers && scorePlayers[i] == -1; i++);
    finish(i);
  }
}

void setup() {
  for (int i = 0; i < 3; i++) { //nr max of players pt initializare
    pinMode(joyButton[i], INPUT_PULLUP); // de la pullup e default high
  }
  for (int i = 0; i < 3; i++) {
    pinMode(joyX[i], INPUT); // de la pullup e default high
  }
  display.clearDisplay(0);
  display.shutdown(0, false);
  display.setIntensity(0, 10); //cand il dadeam mai mic imi crapa pe unele fructe

  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(1, 0);  // 2 = a cata casuta din linie sa fie aprinsa
                     // 1 = pe a cata linie ne pozitionam; 0 = prima linie, 1 = a doua linie
  pinMode(V0_PIN, OUTPUT); 
  analogWrite(V0_PIN, 90);

  gameIntroduction();

  Serial.begin(9600);
}

void loop() {
  readData(); //joyX + buttonState + millis
  
  if (introduction == false) {  //introduction <- primul joystick are control (poate modifici ulterior ca primul joystick receptionat sa modifice
    if (dataX[0] > 600 && joyMoved == false) {
      nrPlayers++;
      if (nrPlayers > 3)
        nrPlayers = 3;
      lcd.setCursor(7, 1);
      lcd.print(nrPlayers);
      joyMoved = true;
    }
    if (dataX[0] < 100 && joyMoved == false){
      nrPlayers--;
      if (nrPlayers < 1)
        nrPlayers = 1;
      lcd.setCursor(7, 1);
      lcd.print(nrPlayers);
      joyMoved = true;
    }
    if (dataX[0] < 600 && dataX[0] > 400){
      joyMoved = false;
    }
    if (dataButton[0] == 0) {
      lcd.clear();
      lcd.setCursor(2, 0);
      lcd.print("Let's play!!");
      myDelay(debounceDelay * 10); //delay(1000);
      introduction = true;
      lcd.clear();
      createPlayers();
    }
  }
  else {  //in joc
    playersTurn(); //se da carte jos
    buttonPressed(); //se verifica daca se apasa butonul sau nu
    
    if (empty == false) { //daca nu e gol stackul de carti se afiseaza cartea, altfel se asteapta sa se dea din cursor
      displayImage(fruits[show]);
    }

    if (nrPlayers != 1)
      verify(); //se verifica daca avem un castigator
    else {
      lcd.setCursor(1, 0);
      lcd.print("Current Score:");
      lcd.setCursor(7, 1);
      lcd.print(scorePlayers[0]);
    }
  }
  Serial.println("Score:");
  Serial.println(scorePlayers[0]);
  Serial.println(scorePlayers[1]);
  Serial.println(scorePlayers[2]);
}
