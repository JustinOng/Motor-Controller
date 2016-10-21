#include "Arduino.h"

class Utilities {
  public:
    // takes in a char buffer , len of char buffer, index to start searching at and int for output
    static uint8_t parseInt(uint8_t * str, uint8_t len, uint8_t start_index, int16_t * out);
};
