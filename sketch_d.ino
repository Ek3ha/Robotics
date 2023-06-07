//WHEN POWER SERVICE IS DISCONNECTED RE-UPLOAD THE FILE, WAIT FOR IT TO SAY: "DONE" AND TURN IT ON THE SECOND AFTER


//Defining PINS
#define EN 8
#define X_DIR 5   // X -axis stepper motor direction control
#define X_STP 2   // x -axis stepper control
#define Y_DIR 6   // y -axis stepper motor direction control
#define Y_STP 3   // y -axis stepper control
#define limitX 9  //Limit pins: X->9 , Y->10 , Z->11

#define EM 12 //Electromagnet PIN

//Calibrate Size
int width = 3620;   //X
int height = 3370;  //Y
// int target_x = width;
// int target_y = height;

float factor = 0.8; //How much of the total canvas is playing board. Keep it below 0,89 because of discarding spot
int board_sizeX = width*factor;
int board_sizeY = height*factor;

//Serial Communication
int received = 0;  //Received code Python Serial (pos1:XY -> pos2: XY)
int last_received = 0;
bool electro = false;

//Positions of starting and goal coordinate
int posAX;
int posAY;
int posBX;
int posBY;

int step = 1;  //Sequencing movement

//checker Coordinate lists. Board size 8x8
int xPos[9];  //0 is empty, 1 to 9 are checker pieces.
int yPos[9];

int current_x = 0;
int current_y = 0;

int target_x = width;
int target_y = height;
bool reached = false;

bool x_wiggle = false;
bool y_wiggle = false;



void setup() {
  Serial.begin(9600);  //KEEP FREE FOR PYTHON
  pinMode(X_DIR, OUTPUT);
  pinMode(Y_DIR, OUTPUT);
  pinMode(X_STP, OUTPUT);
  pinMode(Y_STP, OUTPUT);
  pinMode(limitX, INPUT_PULLUP);
  pinMode(EN, OUTPUT);
  pinMode(EM, OUTPUT);

  for (int i = 1; i < 10; i++) {  //Create Checker centers
    xPos[i] = (board_sizeX / 16) * ((i * 2) - 1);
    yPos[i] = (board_sizeY / 16) * ((i * 2) - 1);
  }

  delay(6000);  //Time to turn on the power source in miliseconds

  for (int i = 0; i < (max(width, height) * 1.2); i++) {  //Reset XY to 0 with 1.2 as a factor to overcompensate just in case calibration is a bit off
    digitalWrite(X_STP, HIGH);
    digitalWrite(Y_STP, HIGH);
    delayMicroseconds(500);
    digitalWrite(X_STP, LOW);
    digitalWrite(Y_STP, LOW);
    delayMicroseconds(500);
    digitalWrite(X_DIR, LOW);  //turn direction towards x=0
    digitalWrite(Y_DIR, LOW);  //turn direction towards y=0
  }

  digitalWrite(EM, LOW); //Turn off electromagnet by default
  electro = false;
}
void loop() {
  //CALCULATE POSITION
  //Receive python code
  //First number is X coordinate of starting position
  //Second number is Y coordinate of starting position
  //Third number is X coordinate starting position
  //Final number is Y coordinate starting position
  //If either the 3rd or 4th number is a 0 that means the piece needs to be taken of the board. The first two numbers are still the starting position.
  while (Serial.available()){
    String a = Serial.readString();
    received = a.toInt();
  }
  // received = 1188;  //Valid move example. REPLACE THIS WITH SERIAL RECEIVER.

  if (step == 6 && (received != last_received)) {  //If new number comes in and the sequence is done.
    step = 1;                                      //Back to step one.
  }

  last_received = received;  //Makes back-up of last received number

  //convert into positions
  posAX = received / 1000;
  posAY = (received % 1000) / 100;
  posBX = (received % 100) / 10;
  posBY = received % 10;

  //SEQUENCE MOVING THE MOTOR

  if ((posBX == 0) || (posBY == 0)) { //If piece needs to be discarded. User picks it up from this point from the board
    if (step == 4) {  //Go up
      target_y = height;
      check();
      if (reached == true) {
        electro = false; //Turn off Electromagnet
        delay(500);
        step += 2;
      }
    }
    else if (step == 3) {  //Go to the left
      target_x = width;
      check();
      if (reached == true) {
        delay(500);
        step += 1;
      }
    }
    else if (step == 2) {  //Go up.
      target_y = yPos[posAY] + (board_sizeY / 16);
      check();
      if (reached == true) {
        delay(500);
        step += 1;
      }
    }
    else if (step == 1) {  //Go to start position and pickup item.
      target_x = xPos[posAX];
      target_y = yPos[posAY];
      check();
      if (reached == true) {
        electro = true; //Turn on Electromagnet
        delay(2000);
        step += 1;
      }
    }

  }

  else {
    //MOVING STEPS NORMAL
    if (step == 5) {  //Final position
      target_x = xPos[posBX];
      check();
      if (reached == true) {
        electro = false; //Turn off Electromagnet
        delay(2000);
        step += 1;
      }
    } else if (step == 4) {  //Go up
      target_y = yPos[posBY];
      check();
      if (reached == true) {
        delay(500);
        step += 1;
      }
    } else if (step == 3) {  //Go to the left
      target_x = xPos[posBX] + (board_sizeX / 16);
      check();
      if (reached == true) {
        delay(500);
        step += 1;
      }
    } else if (step == 2) {  //Go up.
      target_y = yPos[posAY] + (board_sizeY / 16);
      check();
      if (reached == true) {
        delay(500);
        step += 1;
      }
    } else if (step == 1) {  //Go to start position and pickup item.
      target_x = xPos[posAX];
      target_y = yPos[posAY];
      check();
      if (reached == true) {
        electro = true; //Turn on Electromagnet
        delay(2000);
        step += 1;
      }
    }
  }

  //MOVE MOTOR TO SET GOAL

  if ((current_x == target_x) && (current_y == target_y)) {  //Make motor stop when goal is reached
    digitalWrite(X_STP, LOW);
    digitalWrite(Y_STP, LOW);
  } else {
    //Run Motors
    digitalWrite(X_STP, HIGH);
    digitalWrite(Y_STP, HIGH);
    delayMicroseconds(500);
    digitalWrite(X_STP, LOW);
    digitalWrite(Y_STP, LOW);
    delayMicroseconds(500);

    if (current_x != target_x) {  //X Position
      if (current_x < target_x) {
        digitalWrite(X_DIR, HIGH);
        current_x += 1;
      } else if (current_x > target_x) {
        digitalWrite(X_DIR, LOW);
        current_x -= 1;
      }
    } else {
      if (x_wiggle == true) {
        digitalWrite(X_DIR, HIGH);
        x_wiggle = false;
      } else {
        digitalWrite(X_DIR, LOW);
        x_wiggle = true;
      }
    }

    if (current_y != target_y) {  //Y Position
      if (current_y < target_y) {
        digitalWrite(Y_DIR, HIGH);
        current_y += 1;
      } else if (current_y > target_y) {
        digitalWrite(Y_DIR, LOW);
        current_y -= 1;
      }
    } else {
      if (y_wiggle == true) {  //Whiggle makes other motor not slow
        digitalWrite(Y_DIR, HIGH);
        y_wiggle = false;
      } else {
        digitalWrite(Y_DIR, LOW);
        y_wiggle = true;
      }
    }
  }

  if (electro==true){
    digitalWrite(EM, HIGH); //Turn on Electromagnet
  }
  else{
    digitalWrite(EM, LOW); //Turn off Electromagnet
  }
}

void check() {  //CHECK IF TARGET HAS BEEN REACHED ALREADY
  if ((current_x == target_x) && (current_y == target_y)) {
    reached = true;
  } else {
    reached = false;
  }
}
