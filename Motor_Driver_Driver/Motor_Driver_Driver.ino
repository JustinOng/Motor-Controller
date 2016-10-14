#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
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

  duty_cycles[0] = 50;
  duty_cycles[1] = 25;
  duty_cycles[2] = 75;
  duty_cycles[3] = 0;
  duty_cycles[4] = 100;
}

void loop() {
  static uint32_t last_change = 0;

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

