#include <LedControl.h>
#include <LiquidCrystal.h>
#include <time.h>

#define JOY1_X A0
#define JOY2_X A1
#define JOY3_X A2
#define JOY1_BUTTON 13
#define JOY2_BUTTON 8
#define JOY3_BUTTON A3
#define V0_PIN 9
#define REPLAY_BUTTON A4

const int DIN_PIN = 12; //data in
const int CLK_PIN = 11; //clock
const int CS_PIN = 10; //load/cs

LedControl display = LedControl(DIN_PIN, CLK_PIN, CS_PIN);
LiquidCrystal lcd(3, 4, 5, 6, 7, 2);

const uint64_t cards[] = {
  0x06083c4242423c00, //APPLE
  0x0000364944447800, //STRAWBERRY
  0x070a1222e7a5e700, //CHERRY
  0x00040606067e3c00, //BANANA
  0x3c7ee7dbdbe77e3c, //KIWI
  0x003c664242663c00, //ORANGE
  0x3c1818242424243c, //PINEAPPLE
  0x0018242442423c00, //PEAR
  0x001824245a5a3c00, //AVOCADO
  0x081828080808083c, //1
  0x3c04043c2020203c  //2
  //0x3c04042c1404043c//3
};
const int fruitLen = sizeof(cards)/8;

const uint64_t fail = {
  0x8142241818244281 //X
};
const uint64_t success = {
  0x0000240081423c00 //:)
};

int joyX[3] = {JOY1_X, JOY2_X, JOY3_X};
int dataX[3];    //xValue, firstJoy
int joyButton[3] = {JOY1_BUTTON, JOY2_BUTTON, JOY3_BUTTON};
int dataButton[3];    //buttonState
int currentPlayer;
int nrPlayers = 1;
int activePlayers;
int scorePlayers[3];
int count;    //the number of discarded cards on the board
bool introduction = false;    //false -> still in intro | true -> done with intro 
int show;
bool duelActive = false;
int duelRounds;
bool joyMoved = false;
int prev = -1;    //checking for double
int pprev = -1;    //checking for sandwich
bool empty = true;    //if there is any card on the board
bool delayRunning = false;
unsigned long delayStartTime = 0;
bool finished = false;

void readData() {
  for (int i = 0; i < nrPlayers; i++) {
    dataX[i] = analogRead(joyX[i]);    //int firstJoy = analogRead(JOY1_X);
  }
  for (int i = 0; i < nrPlayers; i++) {
    dataButton[i] = digitalRead(joyButton[i]);    //int buttonState = digitalRead(JOY1_BUTTON);
  }
}

void gameIntroduction() {
  lcd.clear();
  lcd.setCursor(4, 0);
  lcd.print("Welcome!");
  delay(2000);    //wait for players to read the message. don't do any other tasks while waiting
  lcd.clear();
  lcd.print("How many players?");
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
}

void wrong(int index, int nr) {
  currentPlayer = (index + 1) % nrPlayers;
  displayImage(fail);
  lcd.clear();
  lcd.setCursor(4, 0);
  lcd.print("GOTCHA!!");
  delay(400);    //wait for the message to appear long enough on the screen. therefore you do not play while waiting
  lcd.clear();
  scorePlayers[index] -= nr;
  char printline[20];
  char conv[2];
  if (nrPlayers != 1) {
    strcpy(printline, "Player number ");
    itoa(index + 1, conv, 10);
    strcat(printline, conv);
    lcd.print(printline);
    lcd.setCursor(0, 1);
    strcpy(printline, "loses ");
  }
  else {
    lcd.setCursor(3, 0);
    strcpy(printline, "-");
  }
  itoa(nr + 1, conv, 10);    //because the index starts from 0 and the player's number from one
  strcat(printline, conv);
  if (nr == 0) {
    strcat(printline, " point!");
  }
  else {
    strcat(printline, " points!");
  }
  lcd.print(printline);
  delay(1500);    ////wait for the message to appear long enough on the screen. therefore you do not play while waiting
  lcd.clear();
  displayImage(cards[nr]);    //to make sure the X disappears
}

void correct(int index) { 
  currentPlayer = index;
  displayImage(success);
  lcd.clear();
  if (duelActive == false) {
    lcd.setCursor(4, 0);
    lcd.print("CORRECT!");
    delay(2400);    ////wait for the message to appear long enough on the screen. therefore you do not play while waiting
    lcd.clear();
  }
  scorePlayers[index] += count;
  char printline[20];
  char conv[2];
  if (nrPlayers != 1) {
    strcpy(printline, "Player number ");
    itoa(index + 1, conv, 10);
    strcat(printline, conv);
    lcd.print(printline);
    lcd.setCursor(0, 1);
    strcpy(printline, "gains ");
  }
  else {
    lcd.setCursor(3, 0);
    strcpy(printline, "+");
  }
  itoa(count, conv, 10);
  strcat(printline, conv);
  if (count == 0) {
    strcat(printline, " point!");
  }
  else {
    strcat(printline, " points!");
  }
  lcd.print(printline);
  delay(1500);    ////wait for the message to appear long enough on the screen. therefore you do not play while waiting
  lcd.clear();
  count = 0;
  display.clearDisplay(0);
}

void createPlayers() {
  for (int i = 0; i < nrPlayers; i++){
    scorePlayers[i] = 30;
  }
  if (nrPlayers == 1) {
    scorePlayers[0] = 60;
    soloMode();
  }
  activePlayers = nrPlayers;
}

void playersTurn() {
  //only Ox | RANDOM-ish order
  if ((dataX[currentPlayer] > 1000 || dataX[currentPlayer] < 100) && joyMoved == false) {
    display.clearDisplay(0);
    delay(100);    //make the image blink  //=> this must disappear
    scorePlayers[currentPlayer]--;    //discards one card
    if (nrPlayers != 1) {
      if (finished == false) {
        displayScore();
        verify();    //check for winner
      }
    }
    else {
      lcd.clear();
      lcd.setCursor(1, 0);
      lcd.print("Current Score:");
      lcd.setCursor(7, 1);
      lcd.print(scorePlayers[0]);
    }
    if (prev == -1) {
      prev = show;
    }
    else {
      pprev = prev;
      prev = show;
    }
    if (duelActive == true) {
      duelRounds--;
    }
    else {
      currentPlayer++;    //wait for your turn to come again
      currentPlayer %= nrPlayers;
      empty = false;    //a card has been discarted not during a duel
    }
    show = random(0, fruitLen); 
    if (show >= 9 && show < fruitLen && duelActive == true) {
      duelRounds = show - 8;
      currentPlayer++;    //wait for your turn to come again
      currentPlayer %= nrPlayers;
    }
    count++;
    joyMoved = true;
  }
  if (dataX[currentPlayer] < 600 && dataX[currentPlayer] > 400) {
    joyMoved = false;
  }
}

void buttonPressed() {
  for (int i = 0; i < nrPlayers; i++) {
    if (dataButton[i] == 0) {
      if (delayRunning == false) {
        if (prev == show || pprev == show) {    //double or sandwich
          if (duelActive == true) {    //while dueling sandwiches and doubles do not count
            wrong(i, show);
          }
          else {
            correct(i);
            empty = true;
            prev = pprev = -1;
          }
        }
        else {
          wrong(i, show);
          prev = pprev = -1;
        }
      }
      //prevent the player who pressed immediately after the "winner" from losing points
      if (delayRunning == false) {
        delayRunning = true;
        delayStartTime = millis();
      }
      if ((delayRunning == true) && (millis() - delayStartTime >= 1000)) {
        delayRunning = false;
      }
    }
  }
}

void duel(int attacker) {
  if (currentPlayer > 0) {
    attacker = currentPlayer - 1;
  }
  else {
    attacker = nrPlayers - 1;
  }
  if (duelActive == false) {
    duelRounds = show - 8;
    duelActive = true;
  }
  if ( duelRounds != 0) {
    playersTurn();
  }
  else {
    delay(200);    //the player needs to see his card beofre the computer moves on to announcing the winner of the duel 
    correct(attacker);
    empty = true;
    duelActive = false;
  }
}

void soloMode() {
  playersTurn();
  buttonPressed();
  if (empty == false)
      displayImage(cards[show]);
}

void replay() {
  display.clearDisplay(0);
  lcd.clear();
  currentPlayer = 0;
  nrPlayers = 1;
  count = 0;
  introduction = false; 
  duelActive = false;
  duelRounds;
  joyMoved = false;
  prev = -1; 
  pprev = -1;
  empty = true;
  delayRunning = false;
  finished = false;
  gameIntroduction();
}

void finish(int index) {
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("The winner is");
  lcd.setCursor(0, 1);
  lcd.print("Player number ");
  char number[2];
  itoa(index + 1, number, 10);
  lcd.print(number);
  display.clearDisplay(0);
  empty = true;
  delay(2400);    //wait for the winner to gloat
  //replay:
  lcd.clear();
  lcd.print("Payback?");
  lcd.setCursor(0, 1);
  lcd.print("Press the button!");
  finished = true;
}  

void verify() { 
  for (int i = 0; i < nrPlayers; i++) {
    if (scorePlayers[i] <= 0) {
      scorePlayers[i] = -1;
      activePlayers--;
    }
  }
  if (activePlayers == 1) {
    int i;
    for (i = 0; i < nrPlayers && scorePlayers[i] == -1; i++);
    finish(i);
  }
}

void displayScore() {
  lcd.clear();
  lcd.setCursor(0, 0);
  char printline[20] = "Player number ";
  char conv[2];
  itoa(currentPlayer + 1, conv, 10);
  strcat(printline, conv);
  lcd.print(printline);
  lcd.setCursor(6, 1);
  lcd.print(scorePlayers[currentPlayer]);
}

void setup() {
  for (int i = 0; i < 3; i++) {    //max number of players for setup
    pinMode(joyButton[i], INPUT_PULLUP);    
  }
  for (int i = 0; i < 3; i++) {
    pinMode(joyX[i], INPUT);
  }
  pinMode(REPLAY_BUTTON, INPUT_PULLUP);
  display.clearDisplay(0);
  display.shutdown(0, false);
  display.setIntensity(0, 10);
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(1, 0);
  pinMode(V0_PIN, OUTPUT); 
  analogWrite(V0_PIN, 90);
  srand(time(NULL));    //seed
  gameIntroduction();

  Serial.begin(9600);
}

void loop() {
  readData();    //read: joyX + buttonState + millis
  if (analogRead(REPLAY_BUTTON) > 1018) {    //if we digitalRead, the button is too sensitive |lack of analog pins
      lcd.clear();
      replay();
  }
  if (introduction == false) {    //introduction mode: first joystick has control over the number of players
    if (delayRunning == false) {
      if (dataX[0] < 100 && joyMoved == false) {
        nrPlayers++;
        if (nrPlayers > 3) {
          nrPlayers = 3;
        }
        lcd.setCursor(7, 1);
        lcd.print(nrPlayers);
        joyMoved = true;
      }
      if (dataX[0] > 600 && joyMoved == false){
        nrPlayers--;
        if (nrPlayers < 1) {
          nrPlayers = 1;
        }
        lcd.setCursor(7, 1);
        lcd.print(nrPlayers);
        joyMoved = true;
      }
      if (dataX[0] < 600 && dataX[0] > 400){
        joyMoved = false;
      }
    }
    if (dataButton[0] == 0) {
      lcd.clear();
      lcd.setCursor(2, 0);
      lcd.print("Let's play!!");
      if (delayRunning == false) {
        delayRunning = true;
        delayStartTime = millis();
     }
    }
    if ((delayRunning == true) && (millis() - delayStartTime >= 1000)) {    //1000 = delayTime
      delayRunning = false;
      introduction = true;
      lcd.clear();
      createPlayers();
     }
  }
  else {    //game mode:
    playersTurn();
    buttonPressed();    //check whether or not a button was pressed
  
    if (empty == false) {    //if the stack is empty wait for a player to move the joystick, else keep showing the current card
      displayImage(cards[show]);
      if (nrPlayers > 1) {
        if ( (show >= 9 && show < fruitLen) || duelActive == true) {    //duel
          duel(currentPlayer);
        }
      }
    }
  }

  //see the scores in the serial monitor
  /*Serial.println("Score:");
  Serial.println(scorePlayers[0]);
  Serial.println(scorePlayers[1]);
  Serial.println(scorePlayers[2]);*/
}
