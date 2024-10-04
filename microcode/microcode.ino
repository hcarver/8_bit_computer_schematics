/**
 * This sketch programs the microcode EEPROMs for the 8-bit breadboard computer
 * It includes support for a flags register with carry and zero flags
 * See this video for more: https://youtu.be/Zg1NdPKoosU
 */

// Arduino
// D2 = shift register serial in (on lower right shift register, which then overflows into the upper left shift register)
// D13 = shift register write enable (high to latch)
// D12 = DQ3
// D11 = DQ4
// D10 = DQ5
// D9 = DQ6
// D8 = DQ7
// D7 = shift register clock (positive-edge triggered)
// D6 = A10
// D5 = !G
// D4 = !W
// D3 = A9

const int SHIFT_DATA_PIN = 2;
const int SHIFT_CLK_PIN = 7;
const int SHIFT_LATCH_PIN = 13;
const int EEPROM_NOT_G_PIN = 5;
const int EEPROM_NOT_W_PIN = 4;

const int EEPROM_D3 = 12;
const int EEPROM_D4 = 11;
const int EEPROM_D5 = 10;
const int EEPROM_D6 = 9;
const int EEPROM_D7 = 8;
const int EEPROM_A10 = 6;
const int EEPROM_A9 = 3;


// 0-indexed bit_number, where 0 is least significant
bool getBitFromInteger(int input, int bit_number) {
  return (bool) ((input >> bit_number) % 2);
}

byte getByteFromBits(bool b7, bool b6, bool b5, bool b4, bool b3, bool b2, bool b1, bool b0) {
  return b7 << 7 +
    b6 << 6 +
    b5 << 5 +
    b4 << 4 +
    b3 << 3 +
    b2 << 2 +
    b1 << 1 +
    b0;
}

void outputBytesToShiftRegister(byte b1, byte b0) {
  shiftOut(SHIFT_DATA_PIN, SHIFT_CLK_PIN, MSBFIRST, b1);
  shiftOut(SHIFT_DATA_PIN, SHIFT_CLK_PIN, MSBFIRST, b0);

  digitalWrite(SHIFT_LATCH_PIN, LOW);
  digitalWrite(SHIFT_LATCH_PIN, HIGH);
  digitalWrite(SHIFT_LATCH_PIN, LOW);
}

// Shift register wirings:
// Highest order bit = first inputted
// QH - A3
// QG - A2
// QF - A1
// QE - A0
// QD - DQ0
// QC - DQ1
// QB - DQ2
// QA - nothing
//
// QH - nothing
// QG - nothing
// QF - A8
// QE - A7
// QD - A6
// QC - A5
// QB - A4
// QA - nothing
// Lowest order bit = last inputted
void writeByteAtAddress(int address, byte value) {
  const byte higher_shift_byte = getByteFromBits(
    getBitFromInteger(address, 3),
    getBitFromInteger(address, 2),
    getBitFromInteger(address, 1),
    getBitFromInteger(address, 0),
    getBitFromInteger(value, 0),
    getBitFromInteger(value, 1),
    getBitFromInteger(value, 2),
    0);
    
  const byte lower_shift_byte = getByteFromBits(
    0,
    0,
    getBitFromInteger(address, 8),
    getBitFromInteger(address, 7),
    getBitFromInteger(address, 6),
    getBitFromInteger(address, 5),
    getBitFromInteger(address, 4),
    0);

  outputBytesToShiftRegister(higher_shift_byte, lower_shift_byte);

  // Arduino
  // D2 = shift register serial in (on lower right shift register, which then overflows into the upper left shift register)
  // D13 = shift register write enable (high to latch)
  // D12 = DQ3
  // D11 = DQ4
  // D10 = DQ5
  // D9 = DQ6
  // D8 = DQ7
  // D7 = shift register clock (positive-edge triggered)
  // D6 = A10
  // D5 = !G
  // D4 = !W
  // D3 = A9
  digitalWrite(EEPROM_D3, getBitFromInteger(value, 3));
  digitalWrite(EEPROM_D4, getBitFromInteger(value, 4));
  digitalWrite(EEPROM_D5, getBitFromInteger(value, 5));
  digitalWrite(EEPROM_D6, getBitFromInteger(value, 6));
  digitalWrite(EEPROM_D7, getBitFromInteger(value, 7));
  digitalWrite(EEPROM_A10, getBitFromInteger(address, 10));
  digitalWrite(EEPROM_A9, getBitFromInteger(address, 9));

  digitalWrite(EEPROM_NOT_W_PIN, LOW);
  delayMicroseconds(100);
  digitalWrite(EEPROM_NOT_W_PIN, HIGH);
  delay(10);
  digitalWrite(EEPROM_NOT_W_PIN, LOW);
}



#define HLT 0b1000000000000000  // Halt clock
#define MI  0b0100000000000000  // Memory address register in
#define RI  0b0010000000000000  // RAM data in
#define RO  0b0001000000000000  // RAM data out
#define IO  0b0000100000000000  // Instruction register out
#define II  0b0000010000000000  // Instruction register in
#define AI  0b0000001000000000  // A register in
#define AO  0b0000000100000000  // A register out
#define EO  0b0000000010000000  // ALU out
#define SU  0b0000000001000000  // ALU subtract
#define BI  0b0000000000100000  // B register in
#define OI  0b0000000000010000  // Output register in
#define CE  0b0000000000001000  // Program counter enable
#define CO  0b0000000000000100  // Program counter out
#define J   0b0000000000000010  // Jump (program counter in)
#define FI  0b0000000000000001  // Flags in

#define FLAGS_Z0C0 0
#define FLAGS_Z0C1 1
#define FLAGS_Z1C0 2
#define FLAGS_Z1C1 3

#define JC  0b0111
#define JZ  0b1000

uint16_t UCODE_TEMPLATE[16][8] = {
  { MI|CO,  RO|II|CE,  0,      0,      0,           0, 0, 0 },   // 0000 - NOP
  { MI|CO,  RO|II|CE,  IO|MI,  RO|AI,  0,           0, 0, 0 },   // 0001 - LDA
  { MI|CO,  RO|II|CE,  IO|MI,  RO|BI,  EO|AI|FI,    0, 0, 0 },   // 0010 - ADD
  { MI|CO,  RO|II|CE,  IO|MI,  RO|BI,  EO|AI|SU|FI, 0, 0, 0 },   // 0011 - SUB
  { MI|CO,  RO|II|CE,  IO|MI,  AO|RI,  0,           0, 0, 0 },   // 0100 - STA
  { MI|CO,  RO|II|CE,  IO|AI,  0,      0,           0, 0, 0 },   // 0101 - LDI
  { MI|CO,  RO|II|CE,  IO|J,   0,      0,           0, 0, 0 },   // 0110 - JMP
  { MI|CO,  RO|II|CE,  0,      0,      0,           0, 0, 0 },   // 0111 - JC
  { MI|CO,  RO|II|CE,  0,      0,      0,           0, 0, 0 },   // 1000 - JZ
  { MI|CO,  RO|II|CE,  0,      0,      0,           0, 0, 0 },   // 1001
  { MI|CO,  RO|II|CE,  0,      0,      0,           0, 0, 0 },   // 1010
  { MI|CO,  RO|II|CE,  0,      0,      0,           0, 0, 0 },   // 1011
  { MI|CO,  RO|II|CE,  0,      0,      0,           0, 0, 0 },   // 1100
  { MI|CO,  RO|II|CE,  0,      0,      0,           0, 0, 0 },   // 1101
  { MI|CO,  RO|II|CE,  AO|OI,  0,      0,           0, 0, 0 },   // 1110 - OUT
  { MI|CO,  RO|II|CE,  HLT,    0,      0,           0, 0, 0 },   // 1111 - HLT
};

uint16_t ucode[4][16][8];

void initUCode() {
  // ZF = 0, CF = 0
  memcpy(ucode[FLAGS_Z0C0], UCODE_TEMPLATE, sizeof(UCODE_TEMPLATE));

  // ZF = 0, CF = 1
  memcpy(ucode[FLAGS_Z0C1], UCODE_TEMPLATE, sizeof(UCODE_TEMPLATE));
  ucode[FLAGS_Z0C1][JC][2] = IO|J;

  // ZF = 1, CF = 0
  memcpy(ucode[FLAGS_Z1C0], UCODE_TEMPLATE, sizeof(UCODE_TEMPLATE));
  ucode[FLAGS_Z1C0][JZ][2] = IO|J;

  // ZF = 1, CF = 1
  memcpy(ucode[FLAGS_Z1C1], UCODE_TEMPLATE, sizeof(UCODE_TEMPLATE));
  ucode[FLAGS_Z1C1][JC][2] = IO|J;
  ucode[FLAGS_Z1C1][JZ][2] = IO|J;
}

void blink() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);  
}

void setup() {
  Serial.begin(57600);

  Serial.println("Beginning");
  blink();
   
  Serial.println("Initialising UCode array");
  initUCode();
  blink();

  Serial.println("Setting pin modes");
  pinMode(SHIFT_DATA_PIN, OUTPUT);
  pinMode(SHIFT_CLK_PIN, OUTPUT);
  pinMode(SHIFT_LATCH_PIN , OUTPUT);
  pinMode(EEPROM_NOT_G_PIN, OUTPUT);
  pinMode(EEPROM_NOT_W_PIN, OUTPUT);
  
  pinMode(EEPROM_D3 , OUTPUT);
  pinMode(EEPROM_D4 , OUTPUT);
  pinMode(EEPROM_D5 , OUTPUT);
  pinMode(EEPROM_D6, OUTPUT);
  pinMode(EEPROM_D7, OUTPUT);
  pinMode(EEPROM_A10, OUTPUT);
  pinMode(EEPROM_A9, OUTPUT);
  blink();

  Serial.println("Setting key pins");
  digitalWrite(EEPROM_NOT_G_PIN, HIGH);
  digitalWrite(EEPROM_NOT_W_PIN, HIGH);
  blink();
  

  // Program data bytes
  Serial.print("Programming EEPROM");

//  // Program the 8 high-order bits of microcode into the first 128 bytes of EEPROM
//  for (int address = 0; address < 1024; address += 1) {
//    int flags       = (address & 0b1100000000) >> 8;
//    int byte_sel    = (address & 0b0010000000) >> 7;
//    int instruction = (address & 0b0001111000) >> 3;
//    int step        = (address & 0b0000000111);
//
//    writeByteAtAddress(address, 0xff);
//
//    if (byte_sel) {
//      //writeEEPROM(address, ucode[flags][instruction][step]);
//    } else {
//      //writeEEPROM(address, ucode[flags][instruction][step] >> 8);
//    }
//
//    if (address % 64 == 0) {
//      Serial.print(".");
//    }
//  }
  for(int address = 0; address < 2048; address++) {
    writeByteAtAddress(address, address % 256);

    if (address % 64 == 0) {
      Serial.print(".");
    }
  }

  Serial.println(" done");
}


void loop() {
  // put your main code here, to run repeatedly:
}
