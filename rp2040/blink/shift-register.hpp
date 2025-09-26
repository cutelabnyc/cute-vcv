#include <stdint.h>
#include "pico/stdlib.h"

template <typename T> struct Pin {
  T val;
  uint8_t address;
  uint8_t type; // should be ternary - INPUT OUTPUT PULLUP

  void PinInit() { 
    gpio_init(this->address); 
    gpio_set_dir(this->address, this->type);
  };
  void PinRead() { this->val = gpio_get(this->address); };
  void PinWrite(T valToWrite) { gpio_put(this->address, valToWrite); };
};

class ShiftRegister {
private:
  Pin<uint8_t> data;
  Pin<uint8_t> latch;
  Pin<uint8_t> clock;

public:
  ShiftRegister(uint8_t dataPinAddress, uint8_t latchPinAddress,
                uint8_t clockPinAddress) {
    data = {0, dataPinAddress, GPIO_OUT};
    latch = {0, latchPinAddress, GPIO_OUT};
    clock = {0, clockPinAddress, GPIO_OUT};

    /* data.PinInit(); */
    /* data.PinInit(); */
    /* data.PinInit(); */
    gpio_init(data.address);
    gpio_set_dir(data.address, data.type);
    gpio_init(latch.address);
    gpio_set_dir(latch.address, latch.type);
    gpio_init(clock.address);
    gpio_set_dir(clock.address, clock.type);
  };

  void process(uint8_t *bitsToWrite, uint8_t numBitsToWrite, bool reverse) {
    gpio_put(latch.address, 0);

    for (int i = 0; i < numBitsToWrite; i++) {
      gpio_put(clock.address, 0);
      gpio_put(data.address, bitsToWrite[reverse ? 7 - i : i]);
      gpio_put(clock.address, 1);
    }

    gpio_put(latch.address, 1);
    /* latch.PinWrite(LOW); */

    /* for (int i = 0; i < numBitsToWrite; i++) { */
    /* clock.PinWrite(LOW); */
    /* data.PinWrite(bitsToWrite[reverse ? 7 - i : i]); */
    /* clock.PinWrite(HIGH); */
    /* } */

    /* latch.PinWrite(HIGH); */
  }
};

