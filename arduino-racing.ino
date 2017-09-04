/* Arduino Racing
 * © 2017 Daniel Lima, Mayan Mathen */

#include <FastInputs.h>
#include <ShiftRegister.h>
#include <SerialLCD.h>

// timing
static long _dt;
#define after(t) static long _t ## t = 0; _t ## t += _dt; if (_t ## t > (t) && (_t ## t = 1)) 

// pin assignment
enum MyPins {
// power supply voltage
  Vps = A2,
// track photodiodes
  ir0 = A0,
  ir1 = A1,
// buttons
  swLIGHTS = 4,
  swTRACKS = 5,
  swPREV = 6,
  swNEXT = 7,
// outputs
  pwrLIGHTS = 8,
  pwrTRACKS = 9,
  led0 = 10,
  led1 = 11,
  led2 = 12,
  ledIR = 13,
  SPEAKER = A3,
  dDATA = A4,
  dSHCP = A5,
  dSTCP = 2,
};

static void beep(int t) {
  tone(SPEAKER, 2048, t);
}

struct Player {
  long last, best;
  byte laps;
  bool won;
  void reset(byte laps) {
    this->laps = laps; last = 0; best = 2e9; won = 0;
  }
  void lap(long t) {
    long elapsed = t-last;
    last = t;
    if (elapsed < best) best = elapsed;
    laps--;
    if (!laps) won = 1;
  }
} P[2];

struct {
  void (*state)(); // state function
  float volts;
  long start, pause, best;
  byte laps;
  bool tracks, lights, lowIR;

  void countdown(byte lv) {
    int t = lv ? 2000 : 500;
    digitalWrite(led2, lv==0);
    digitalWrite(led1, 1<lv && lv<=3);
    digitalWrite(led0, lv==3);
    // SparkFun COM-07950 buzzers are tuned for 2.048 kHz
    // Plays 0.5s in levels 0/1/2 (wait); 2s in level 3 (race start)
    beep(t);
  }
  void reset() {
    digitalWrite(pwrTRACKS, tracks=0);
    state = atSetup;
    P[0].reset(laps);
    P[1].reset(laps);
    countdown(5);
  }
} sys;

ShiftRegister shReg(dDATA,dSHCP,dSTCP);
SerialLCD lcd(shReg, 1, 3, 4, 5, 6, 7);
FastInputs<Vps,ir0,ir1,swLIGHTS,swTRACKS,swPREV,swNEXT> input;

void setup() {
  pinMode(Vps, INPUT);
  pinMode(ir0, INPUT_PULLUP);
  pinMode(ir1, INPUT_PULLUP);
  pinMode(swLIGHTS, INPUT_PULLUP);
  pinMode(swTRACKS, INPUT_PULLUP);
  pinMode(swPREV, INPUT_PULLUP);
  pinMode(swNEXT, INPUT_PULLUP);
  pinMode(pwrLIGHTS, OUTPUT);
  pinMode(pwrTRACKS, OUTPUT);
  pinMode(led0, OUTPUT);
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(ledIR, OUTPUT);
  pinMode(SPEAKER, OUTPUT);
  speak();
  input();
  sys.lowIR = !input[ir0] && !input[ir1];
  sys.reset();
  sys.laps = 5;
}

void loop() {
  static long t0 = 0;
  _dt = millis() - t0;
  input();
  power();
  voltage();
  sys.state();
  //display();
  t0 += _dt;
}

void power() {
  // basic output and power control
  if (input(swLIGHTS)) sys.lights = !sys.lights;
  if (input(swTRACKS)) sys.tracks = !sys.tracks;
  digitalWrite(pwrLIGHTS, sys.lights);
  digitalWrite(pwrTRACKS, sys.tracks);
  digitalWrite(ledIR, input[ir0] || input[ir1]);
}

void voltage() {
  // 0-10k : 100k voltage divider, 50V DC max.
  after(100) sys.volts = analogRead(Vps) / 1023.0 * 75;
}

static void speak() {
	tone(SPEAKER, 1864, 200);
	tone(SPEAKER, 2217, 200);
	tone(SPEAKER, 1975, 200);
	tone(SPEAKER, 2093, 200);
	tone(SPEAKER, 2217, 200);
}

static void display() {
  //lcd.refresh();
}

static void atSetup() {
  if (input(swPREV)) {
    sys.laps = max(1, sys.laps - 1);
    beep(100);
  }
  else if (input(swNEXT) && !sys.tracks) {
    sys.laps = min(99, sys.laps + 1);
    beep(100);
  }
  else if (input(swNEXT) && sys.tracks) {
    P[0].reset(sys.laps);
    P[1].reset(sys.laps);
    // race start, begin semaphore countdown
    for (int i = 3; i > 0; i--) {
      sys.countdown(i);
      long t = millis();
      while (millis() - t < 1000) { // wait 1s while checking for false start
        input();
        if (input[ir0] || input[ir1] || !sys.tracks) {
          // false start or power down detected, beep 3x and restart setup
          for (int j = 0; j < 3; j++) {
            beep(500); delay(300);
          }
          sys.reset();
          return;
        }
      }
    }
    sys.state = atRace;
    sys.start = millis();
    sys.countdown(0);
  }
  lcd.print(sys.laps);
}

static void atRace() {
  long now = millis();
  for (int i = 0; i < 2; i++) {
    if (input(i ? ir1 : ir0)) {
      // something passed the finish line!
      if (P[i].last) P[i].lap(now);
      else P[i].last = sys.start; // ignore the starting line
    }
  }
  lcd.print(P[0].laps*10 + P[1].laps);
  if (P[0].won || P[1].won) {
    // announce winner
    if (P[0].won && P[1].won) {
      lcd.print(88);
      sys.best = min(P[0].best, P[1].best);
    } else {
      // - WINNER is
      byte winner = P[1].won;
      sys.best = P[winner].best;
      lcd.print(winner+1); // 1 or 2
    }
    sys.state = atFinish;
  }
  else if (input(swPREV) || !sys.tracks) {
    // pause race
    sys.pause = now;
    sys.state = atPause;
    digitalWrite(pwrTRACKS, sys.tracks=0);
  }
}

static void atPause() {
  long now = millis();
  if (input(swPREV)) {
    // reset
    sys.reset();
  } else if (input(swNEXT)) {
    if (!sys.tracks) {
      // tracks not powered
      beep(100); delay(400); beep(100); delay(400); beep(100);
    } else {
      // continue
      sys.state = atRace;
      // account for the paused time
      long elapsed = now - sys.pause;
      for (int i=0; i<2; i++) P[i].last += elapsed;
      beep(1000); // beep for 1s
    }
  }
  lcd.print(sys.volts);
}

static void atFinish() {
  if (input(swPREV) || input(swNEXT) || !sys.tracks) {
    sys.reset();
  }
  
  // Blinking lights of joy and happiness!
  after(500) {
    sys.lights = !sys.lights;
    digitalWrite(pwrLIGHTS, sys.lights);
    beep(500);
  }

  after(3000) {
    lcd.print(sys.best / 1000.0f);
  }
  
  // auto RESET in 60s
  after(60000) {
    sys.reset();
  }
}
