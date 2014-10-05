int fsrVal = 0;
int noCup = 1023; 
int maxCup = 300;
int emptyCup = 1000;

int myPumps[] = {9, 10, 11, 12, 13};
//int bottles[5];
int bottles[5] = {2000, 1000, 500, 250, 125};
int maxVal = 0;

void setup() {
  Serial.begin(9600);           // set up Serial library at 9600 bps  
}

void loop() {
  // Force Sensitive Resistor
  fsrVal = analogRead(A7); 
  if (fsrVal < emptyCup and fsrVal > maxCup) { // indicate cup is present
    Serial.print("Begin Mixing!\n");
  }
  else if (fsrVal < maxCup) {
    Serial.print("Ready to Serve!\n");
  }
  else {
    Serial.print("idle\n"); 
  }
  
  // Pumps    
  // Find Max Val
  for (int i = 0 ; i < 5 ; i++) {
    if (bottles[i] > maxVal) {
      maxVal = bottles[i];
    }
  }
  
  // Begin Mixing!
  for (int j = maxVal ; j > 0 ; j--) {
    if (bottles[0] == j) {
      analogWrite(myPumps[0], 255);
    }
    if (bottles[1] == j) {
      analogWrite(myPumps[1], 255);
    }
    if (bottles[2] == j) {
      analogWrite(myPumps[2], 255);
    }
    if (bottles[3] == j) {
      analogWrite(myPumps[3], 255);
    } 
    if (bottles[4] == j) {
      analogWrite(myPumps[4], 255);
    }
  }

//  analogWrite(myPumps[0], 255);
//  analogWrite(myPumps[1], 255);
//  analogWrite(myPumps[2], 255);
//  analogWrite(myPumps[3], 255);
//  analogWrite(myPumps[4], 255);
}

