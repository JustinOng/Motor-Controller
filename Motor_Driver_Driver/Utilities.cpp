#include "Utilities.h"

#define MAX_INT_LENGTH 5

static uint8_t Utilities::parseInt(uint8_t * str, uint8_t len, uint8_t start_index, int16_t * out) {
  uint8_t digits[MAX_INT_LENGTH];
  uint8_t digit_length = 0;
  int8_t sign = 1;
  uint8_t end_index = 0;

  for(uint8_t i = start_index; i < len; i++) {
    if (digit_length == 0 && str[i] == '-') {
      sign = -1;
    }
    else if (digit_length < MAX_INT_LENGTH) {
      if (str[i] >= '0' && str[i] <= '9') {
        digits[digit_length++] = str[i] - '0';
      }
      else if (digit_length > 0) {
        // found non number after reading in one number already
        if (!(str[i] >= '0' && str[i] <= '9')) {
          end_index = i;
          break;
        }
      }
    }
    
    if (digit_length > MAX_INT_LENGTH) {
      // exit loop if already found MAX_INT_LENGTH numerical digits
      end_index = i;
      break;
    }

    end_index = min(i+1, len);
  }

  for(int8_t i = digit_length; i >= 0; i--) {
    *out += digits[digit_length] * pow(10, digit_length - i);
  }
}

