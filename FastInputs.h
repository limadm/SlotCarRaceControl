/*
 * Fast Debounced Inputs for Arduino
 * MIT license © 2017 Daniel M. Lima
 */

template<byte... pins>
class FastInputs {
  long t;
  byte pin[21];
  byte debuf[sizeof...(pins)];
  byte sampl[sizeof...(pins)];
  bool state[sizeof...(pins)];
public:
  FastInputs():
    pin(),
    debuf(),
    sampl(),
    state()
  {
    const byte input[] = { pins... };
    for (byte i=0; i<sizeof(pin); i++) pin[i] = 255;
    for (byte i=0; i<sizeof(input); i++) pin[input[i]] = i;
  }
  ~FastInputs() {}
  // update input buffers
  void operator()(void) {
    for (byte p=0; p<sizeof(pin); p++) {
      byte i = pin[p];
      if (i != 255) {
        debuf[i] = (debuf[i]/2) + (digitalRead(p) ? 0 : 128);
        state[i] =  debuf[i]>253 ? 1 : debuf[i]<2 ? 0 : state[i];
        sampl[i] =  sampl[i]<<1 | state[i];
      }
    }
  }
  // returns true if button is down
  // (i.e. when input is pulled DOWN)
  bool operator[](byte p) const {
    return state[pin[p]];
  }
  // returns true if button has changed with given edge
  // defaults to falling edge (button has just been pressed)
  bool operator()(byte p, bool rising=false) const {
    return sampl[pin[p]] == (rising ? 0xF0 : 0x0F);
  }
};
