#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif 
#ifndef wbi
#define wbi(sfr, bit, val) if (val) { sbi(sfr, bit); } else { cbi(sfr, bit); }
#endif

#define TLC_LAT 3
#define TLC_CLK 4
#define TLC_SIN 5

uint8_t duty_cycles[8] = {0};

volatile uint32_t cur_millis = 0;

void write_duty_cycles(void) {
  cbi(PORTC, TLC_CLK);
  cbi(PORTC, TLC_LAT);

  cbi(PORTC, TLC_SIN);  
  // fill OUT15-8 with 0s since the upper 8 channels are not used
  for(uint8_t i = 0; i <  8*12; i++) {
    sbi(PORTC, TLC_CLK);
    cbi(PORTC, TLC_CLK);
  }
  
  for(int8_t ch = 7; ch >= 0; ch--) { 
    cbi(PORTC, TLC_SIN);
    
    for(uint8_t i = 0; i < 4; i++) {
      cbi(PORTC, TLC_CLK);
      sbi(PORTC, TLC_CLK);
    }
    
    for(int8_t i = 7; i >= 0; i--) {
      cbi(PORTC, TLC_CLK);

      if (duty_cycles[ch] & (1<<i)) {
        sbi(PORTC, TLC_SIN);
      }
      else {
        cbi(PORTC, TLC_SIN);
      }

      sbi(PORTC, TLC_CLK);
    }
  }

  sbi(PORTC, TLC_LAT);
  cbi(PORTC, TLC_LAT);
}

void setup() {
  // set C0:2, D3, D5:7 and B0 as outputs for DIR
  DDRC |= 0x7;
  DDRD |= (1<<3);
  DDRB |= (1<<0);
  
  // set C3:6 as output
  DDRC |= 0x38;

  // set PB5 as output for STATUS
  DDRB |= (1<<5);
  
  // https://www.reddit.com/r/arduino/comments/3898g6/generating_14_mhz_clock_on_arduino/
  
  // Configure timer1 as the 20KHz clock to drive BLANK of the TLC5940 on pin OC1A
  
  // OC1A as output
  //PORTB |= (1<<1);
  DDRB |= (1<<1);
  
  // [8:7] clear OC1A on match, set at BOTTOM
  // [1:0] Fast PWM with TOP = ICR1
  TCCR1A = 0b10000010;

  // [4:3] Fast PWM with TOP = ICR1
  // [2:0] prescalar = 8
  TCCR1B = 0b00011010;

  // reset timer when TCNT1 = ICR1 = 99
  // OC1A is also reset to 0/1 in non inverting/inverting mode respectively
  // results in 16M/(8*100)Hz = 20KHz
  ICR1 = 99;

  // Configure timer1 to also fire interrupts when it overflows ie at 20KHz
  // Used to provide a millis variable at 50us resolution

  // [0] enable timer1 overflow interrupt
  TIMSK1 = 0b00000001;

  // clear all flags
  TIFR1 = 0b00000000;

  // clear OC1A when TCNT1 = OCR1A = 0
  // high for 500ns, is > 300ns than in the datasheet
  OCR1A = 0;

  // Configure timer2 as the 2MHz clock to drive GSCLK of the TLC5940 on pin OC2A

  //OC2A as output
  DDRB |= (1<<3);
  
  // [8:7] toggle OC2A on match with OCR2A
  // [1:0] WGM Mode 7, Fast PWM with TOP = OCR2A
  TCCR2A = 0b01000011;

  // [3] WGM Mode 7, Fast PWM with TOP = OCR2A
  // [2:0] prescalar = 1
  TCCR2B = 0b00001001;

  // toggle OC2A at 4Mhz, giving f=2MHz
  OCR2A = 3;

  Serial.begin(115200);
  Serial.setTimeout(5000);
}

void loop() {
  static uint32_t last_change = 0;
  static uint8_t motor = 0xFF,
    dir = 0xFF,
    power = 0xFF;

  int tmp;

  if (Serial.available()) {
  
    if (Serial.peek() == 'M') {
      if (Serial.available() >= 2) {      
        tmp = Serial.parseInt();
  
        if (tmp >= 1 && tmp <= 8) {
          motor = tmp;
        }
      }
    }
    else if (Serial.peek() == 'D') {
      if (Serial.available() >= 2) {    
        tmp = Serial.parseInt();
  
        if (tmp == 0 || tmp == 1) {
          dir = tmp;
        }
      }
    }
    else if (Serial.peek() == 'P') {
      if (Serial.available() >= 2) {
        tmp = Serial.parseInt();
  
        if (tmp >= 0 && tmp <= 100) {
          Serial.print("Found power: ");
          Serial.println(tmp);
          power = tmp;
        }
      }
    }
    else if (Serial.peek() == '\n' || Serial.peek() == '\r') {
      // remove the \n or \r from serial buffer
      Serial.read();

      // remove the second \n or \r if present
      if (Serial.peek() == '\n' || Serial.peek() == '\r') Serial.read();
      
      if (motor >= 1 && motor <= 8) {
        // if dir was defined for this line then set it
        if (dir == 0 || dir == 1) {
          switch(motor) {
            case 1:
            case 2:
            case 3:
              wbi(PORTC, motor-1, dir);
              break;
            case 4:
              wbi(PORTD, 3, dir);
              break;
            case 5:
              wbi(PORTB, 0, dir);
              break;
            case 6:
            case 7:
            case 8:
              wbi(PORTD, motor - 1, dir);
              break;
          }
        }

        // if power was defined then set it
        if (power < 255) {
          duty_cycles[motor-1] = power;
          write_duty_cycles();
        }

        Serial.print('M');
        Serial.print(motor);
        Serial.print('D');
        Serial.print(dir);
        Serial.print('P');
        Serial.print(power);
        motor = dir = power = 255;
        Serial.println('S');
      }
      else {
        Serial.print('M');
        Serial.print(motor);
        Serial.print('D');
        Serial.print(dir);
        Serial.print('P');
        Serial.print(power);
        Serial.println('E');
      }
    }
    else {
      // byte is invalid, was not caught by any of the cases above so dump it
      Serial.read();
    }
  }

  write_duty_cycles();
  
  if ((cur_millis - last_change) > 250) {
    PORTB ^= (1<<5);
    
    last_change = cur_millis;
  }
}

ISR (TIMER1_OVF_vect) {
  // ISR fires at 20KHz so 20 ticks for 1ms
  static uint8_t ticks = 0;

  ticks++;

  if (ticks >= 20) {
    ticks = 0;
    
    cur_millis++;
  }
}

