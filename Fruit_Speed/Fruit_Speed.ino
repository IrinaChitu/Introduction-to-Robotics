#include <LedControl.h>
#include <LiquidCrystal.h>

#define JOY1_X A0
#define JOY2_X A1
#define JOY3_X A2
#define JOY1_BUTTON 8
#define JOY2_BUTTON 13
#define JOY3_BUTTON A3
#define V0_PIN 9

const int DIN_PIN = 12; //data in
const int CLK_PIN = 11; //clock
const int CS_PIN = 10; //load/cs

LedControl display = LedControl(DIN_PIN, CLK_PIN, CS_PIN);
LiquidCrystal lcd(2, 3, 4, 5, 6, 7);

const uint64_t cards[] = {
  0x003c4242423c1060, //APPLE
  0x001e2222926c0000, //STRAWBERRY
  0x00e7e7e7444850e0, //BANANA
  0x003c7e6060602000, //CHERRY
  0x3c7ee7dbdbe77e3c, //KIWI
  0x003c664242663c00, //ORANGE
  0x3c2424242418183c, //PINEAPPLE
  0x003c424224241800, //PEAR
  0x003c5a5a24241800, //AVOCADO
  0x3c10101010141810, //1 
  0x3c0404043c20203c, //2
  0x3c2020283420203c, //3
  0x2020203e24283020  //4
};
const int fruitLen = sizeof(cards)/8;

const uint64_t fail = {
  0x8142241818244281 //X
};

const uint64_t success = {
  0x003c428100240000 //:)
};

int joyX[3] = {JOY1_X, JOY2_X, JOY3_X};
int dataX[3];   //xValue, firstJoy
int joyButton[3] = {JOY1_BUTTON, JOY2_BUTTON, JOY3_BUTTON};
int dataButton[3];    //buttonState
int currentPlayer;
int nrPlayers = 1;
int activePlayers;
int scorePlayers[3];
int count;    //the number of discarded cards on the board
bool introduction = false;    //false -> still in intro | true -> done with intro 
int show;
bool joyMoved = false;
int prev = -1;    //checking for double
int pprev = -1;   //checking for sandwich
bool empty = true;    //if there is any card on the board
unsigned long currentMillis;
unsigned long previousMillis = 0;   //last time "sth happened" | unsigned for time variables for time holders 'cause the value will quickly become too large for an int to store
const unsigned long debounceDelay = 100;    //milliseconds

void myDelay(int ms) {
  previousMillis = millis();
  while (millis() - previousMillis < ms);
}

void readData() {
  for (int i = 0; i < nrPlayers; i++) {
    dataX[i] = analogRead(joyX[i]);   //int firstJoy = analogRead(JOY1_X);
  }
  for (int i = 0; i < nrPlayers; i++) {
    dataButton[i] = digitalRead(joyButton[i]);    //int buttonState = digitalRead(JOY1_BUTTON);
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
  scorePlayers[index] -= nr;
  char printline[20] = "Player number ";
  char conv[2];
  itoa(index + 1, conv, 10);
  strcat(printline, conv);
  lcd.print(printline);
  lcd.setCursor(0, 1);
  strcpy(printline, "loses ");
  itoa(nr + 1, conv, 10);   //because the index starts from 0 and the player's number from one
  strcat(printline, conv);
  if (nr == 0)
    strcat(printline, " point!");
  else
    strcat(printline, " points!");
  lcd.print(printline);
  myDelay(debounceDelay * 15); //delay(1500);
  lcd.clear();
  displayImage(cards[nr]);    //to make sure the X disappears
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

void createPlayers() {
  for (int i = 0; i < nrPlayers; i++){
    scorePlayers[i] = 20;
  }
  activePlayers = nrPlayers;
}

void playersTurn() {
  //only Ox | RANDOM-ish order
  if ((dataX[currentPlayer] > 1000 || dataX[currentPlayer] < 100) && joyMoved == false) {
    display.clearDisplay(0);
    myDelay(debounceDelay); //delay(100);
    if (prev == -1)
      prev = show;
    else {
      pprev = prev;
      prev = show;
    }
    scorePlayers[currentPlayer]--;    //discards one card
    currentPlayer++;    //wait for your turn to come again
    currentPlayer %= nrPlayers;
    show = random(0, fruitLen); 
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
      if (prev == show || pprev == show) {    //double or sandwich
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

void soloMode() {
  playersTurn();
  buttonPressed();
  if (empty == false)
      displayImage(cards[show]);
}

void finish(int index) {
  lcd.setCursor(1, 0);
  lcd.print("The winner is");
  lcd.setCursor(1, 1);
  lcd.print("Player number ");
  char number[2];
  itoa(index + 1, number, 10);
  lcd.print(number);
  display.clearDisplay(0);
  myDelay(debounceDelay * 24); //delay(10);
  //add replay
  gameIntroduction();
  introduction = false;
}

void verify() { 
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
  for (int i = 0; i < 3; i++) {   //max number of players for setup
    pinMode(joyButton[i], INPUT_PULLUP);    
  }
  for (int i = 0; i < 3; i++) {
    pinMode(joyX[i], INPUT);
  }
  display.clearDisplay(0);
  display.shutdown(0, false);
  display.setIntensity(0, 10);

  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(1, 0);
  pinMode(V0_PIN, OUTPUT); 
  analogWrite(V0_PIN, 90);

  gameIntroduction();

  Serial.begin(9600);
}

void loop() {
  readData(); //read: joyX + buttonState + millis
  
  if (introduction == false) {    //introduction mode: first joystick has control over the number of players
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
  else {    //game mode:
    playersTurn();
    buttonPressed();    //check whether or not a button was pressed ! => attachInterrupt !
    
    if (empty == false) {   //if the stack is empty wait for a player to move the joystick, else keep showing the current card
      displayImage(cards[show]);
    }

    if (nrPlayers != 1)
      verify();   //check for winner
    else {
      lcd.setCursor(1, 0);
      lcd.print("Current Score:");
      lcd.setCursor(7, 1);
      lcd.print(scorePlayers[0]);
    }
  }
  //for now the only place where you can see the scores is in the serial monitor
  Serial.println("Score:");
  Serial.println(scorePlayers[0]);
  Serial.println(scorePlayers[1]);
  Serial.println(scorePlayers[2]);
}
