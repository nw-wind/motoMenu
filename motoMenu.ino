#include "SmartButton.h"
#include "SmartDelay.h"

// include the library code:
#include <LiquidCrystal.h>

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

template<class T> inline Print &operator <<(Print &obj, T arg) {
  obj.print(arg);
  return obj;
}
#define endl "\n"

enum Mode {modeGauges = 0, modeOnTheWay, modeSport, totalModes};

enum Button {Mode = 0, Up, Down, OK};

const char *dModes[] = {"Normal", "Sport", "Speed", "Menu"};
byte dMode = 0;
const char *sModes[] = {"Stay", "Go"};
byte sMode = 0;
const char *odo[] = {"0000", "123", "456"};
byte odoI = 0;

SmartDelay checkSpeed(3000000UL);
SmartDelay checkDisp(2000000UL);

class BtGauges {
  private:
    enum input {Idle = 0, modeClick, okClick, okHold, okLongHold, upClick, downClick, speedOn, speedOff, totalInputs};
    enum state {dispNorm = 0, dispSpeed, dispSport, dispMenu, totalStates};
    typedef void (BtGauges::*FSM)(enum state st, enum input in);
    enum state btState = dispNorm;
    enum input btInput = Idle;
    FSM action[totalStates][totalInputs] = {
      {&BtGauges::toIdle, &BtGauges::toDispToggle, NULL, &BtGauges::toDispMenu, &BtGauges::toOdoDrop, &BtGauges::toOdoChange, &BtGauges::toOdoChange, &BtGauges::toDispGo, &BtGauges::toDispStay},
      {&BtGauges::toIdle, NULL, NULL, NULL, NULL, NULL, NULL, &BtGauges::toDispGo, &BtGauges::toDispStay},
      {NULL, &BtGauges::toDispToggle, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
      {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL}
    };

    void doAction(enum state st, enum input in) {
      if (action[st][in] == NULL) {
        //Serial.println("NULL action");
        return;
      }
      //Serial << "Do action " << st << " " << in << endl;
      (this->*(action[st][in]))(st, in);
    }

    void toIdle(enum state st, enum input in) {
      //Serial << "toIdle " << st << " " << in << endl;
    }
    void toDispToggle(enum state st, enum input in) {
      //Serial << "toDispToggle " << st << " " << in << endl;
      if (dMode == 0 || dMode == 1) {
        dMode ^= 1;
      } else {
        dMode = 0;
      }
      if (st == dispNorm) btState = dispSport;
      else if (st == dispSport) btState = dispNorm;
      checkDisp.Reset();
    }
    void toDispMenu(enum state st, enum input in) {
      //Serial << "toDispMenu " << st << " " << in << endl;
      dMode = 3;
      btState = dispMenu;
      checkDisp.Reset();
    }
    void toOdoDrop(enum state st, enum input in) {
      //Serial << "toOdoDrop " << st << " " << in << endl;
      checkDisp.Reset();
    }
    void toOdoChange(enum state st, enum input in) {
      //Serial << "toOdoChange " << st << " " << in << endl;
      if (in == upClick) {
        //Serial << "UP" << endl;
        odoI++; odoI %= 3;
      } else {
        if (in == downClick) {
          //Serial << "DOWN" << endl;
          odoI += 3; odoI--; odoI %= 3;
        }
      }
      checkDisp.Reset();
    }
    void toDispGo(enum state st, enum input in) {
      //Serial << "toDispGo " << st << " " << in << endl;
      //if (dMode == 2) Serial << "Already in go mode" << endl;
      if (dMode != 1) dMode = 2;
      sMode = 1;
      btState = dispSpeed;
      checkDisp.Reset();
    }
    void toDispStay(enum state st, enum input in) {
      //Serial << "toDispStay " << st << " " << in << endl;
      //if (dMode == 0) Serial << "Already in stay mode" << endl;
      if (dMode != 1) dMode = 0;
      sMode = 0;
      btState = dispNorm;
      checkDisp.Reset();
    }
    //void toIdle(enum state st, enum input in){Serial << "Idle " << st << " " << in << endl;}

  public:
    void click(enum Button bt) {
      Serial << "Button " << bt << " clicked" << endl;
      switch (bt) {
        case Mode:
          doAction(btState, modeClick);
          break;
        case Up:
          doAction(btState, upClick);
          break;
        case Down:
          doAction(btState, downClick);
          break;
        case OK:
          doAction(btState, okClick);
          break;
      }
    }
    void hold(enum Button bt) {
      //Serial << "Button " << bt << " clicked" << endl;
    }
    void longhold(enum Button bt) {
      //Serial << "Button " << bt << " clicked" << endl;
    }

    void speedStart() {
      doAction(btState, speedOn);
    }
    void speedStop() {
      doAction(btState, speedOff);
    }

};

BtGauges bg;

void testfunc() {}

class BtGeneral : public SmartButton {
  private:
    enum Button button;
  public:
    BtGeneral(int pin) : SmartButton (pin) {}
    BtGeneral(int pin, enum Button mode) : SmartButton (pin) {
      button = mode;
      //Serial << "Pointer to class size = " << sizeof(&BtGeneral::onClick) << endl;
    }
    void onClick() {
      bg.click(button);
    }
    void onHold() {
      bg.hold(button);
    }
    void onLongHold() {
      bg.longhold(button);
    }
};

BtGeneral btMode(9, Mode);
BtGeneral btUp(8, Up);
BtGeneral btDown(7, Down);
BtGeneral btOK(6, OK);

void setup() {
  Serial.begin(115200);
  Serial << "Ready" << endl;
  pinMode(10, INPUT_PULLUP);
  Serial << "Sizes:"
         << "BtGeneral=" << sizeof(btMode) << endl
         << "SmartDelay=" << sizeof(checkSpeed) << endl
         << "BtGauges=" << sizeof(bg) << endl
         << endl;
  Serial << "pointer to function size = " << sizeof(&testfunc) << endl;
  lcd.begin(20, 2);
}

#include <stdarg.h>
#define PRINTF_BUF 21
void printAt(int c, int r, const char *s, ...) {
  char buf[PRINTF_BUF];
  va_list ap;
  va_start(ap, s);
  //if (displayOK) {
  lcd.setCursor(c, r);
  vsnprintf(buf, sizeof(buf), s, ap);
  lcd.print(buf);
  //}
  va_end(ap);
}

unsigned long velo;

void loop() {
  btMode.run();
  btUp.run();
  btDown.run();
  btOK.run();
  if (checkSpeed.Now()) {
    if (digitalRead(10)) {
      //Serial << "Pin10 HIGH" << endl;
      bg.speedStop();
    }
    else {
      //Serial << "Pin10 LOW" << endl;
      bg.speedStart();
    }
  }
  if (checkDisp.Now()) {
    //Serial << "Display: " << dModes[dMode] << " Speed: " << sModes[sMode] << " Odo: " << odo[odoI] << endl;
    printAt(0, 0, "%6s", dModes[dMode]);
    printAt(0, 1, "%6s", sModes[sMode]);
    printAt(10, 0, "%6s", odo[odoI]);
  }
}
