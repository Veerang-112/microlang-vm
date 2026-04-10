#include <EEPROM.h>

const uint8_t OP_PINMODE = 0x10;
const uint8_t OP_WRITE   = 0x11;
const uint8_t OP_DELAY   = 0x12;

const uint8_t HEADER1 = 0x7E;
const uint8_t HEADER2 = 0x7E;

const int EEPROM_SIGNATURE_ADDR = 0;
const int EEPROM_SIZE_ADDR      = 1;   // 1 and 2
const int EEPROM_PROGRAM_ADDR   = 16;

const uint8_t EEPROM_SIGNATURE = 0x42;
const uint16_t MAX_PROGRAM_SIZE = 256;

uint16_t programSize = 0;
uint16_t pc = 0;
bool programLoaded = false;

int readByteBlocking(unsigned long timeoutMs = 3000) {
  unsigned long start = millis();
  while (millis() - start < timeoutMs) {
    if (Serial.available()) {
      return Serial.read();
    }
  }
  return -1;
}

void saveProgramToEEPROM(const uint8_t* data, uint16_t size) {
  EEPROM.update(EEPROM_SIGNATURE_ADDR, EEPROM_SIGNATURE);
  EEPROM.update(EEPROM_SIZE_ADDR, size & 0xFF);
  EEPROM.update(EEPROM_SIZE_ADDR + 1, (size >> 8) & 0xFF);

  for (uint16_t i = 0; i < size; i++) {
    EEPROM.update(EEPROM_PROGRAM_ADDR + i, data[i]);
  }

  programSize = size;
  pc = 0;
  programLoaded = true;
}

void loadProgramFromEEPROM() {
  if (EEPROM.read(EEPROM_SIGNATURE_ADDR) != EEPROM_SIGNATURE) {
    programLoaded = false;
    programSize = 0;
    return;
  }

  programSize = EEPROM.read(EEPROM_SIZE_ADDR) |
                (EEPROM.read(EEPROM_SIZE_ADDR + 1) << 8);

  if (programSize == 0 || programSize > MAX_PROGRAM_SIZE) {
    programLoaded = false;
    programSize = 0;
    return;
  }

  pc = 0;
  programLoaded = true;
}

uint8_t nextByte() {
  if (pc >= programSize) {
    pc = 0;
  }
  return EEPROM.read(EEPROM_PROGRAM_ADDR + pc++);
}

void checkForUpload() {
  if (Serial.available() < 4) return;

  if (Serial.peek() != HEADER1) {
    Serial.read();
    return;
  }

  int h1 = readByteBlocking();
  int h2 = readByteBlocking();

  if (h1 != HEADER1 || h2 != HEADER2) return;

  int sizeLo = readByteBlocking();
  int sizeHi = readByteBlocking();

  if (sizeLo < 0 || sizeHi < 0) return;

  uint16_t incomingSize = (uint16_t)sizeLo | ((uint16_t)sizeHi << 8);

  if (incomingSize == 0 || incomingSize > MAX_PROGRAM_SIZE) {
    Serial.println("UPLOAD_FAIL_SIZE");
    return;
  }

  uint8_t buffer[MAX_PROGRAM_SIZE];

  for (uint16_t i = 0; i < incomingSize; i++) {
    int b = readByteBlocking();
    if (b < 0) {
      Serial.println("UPLOAD_FAIL_TIMEOUT");
      return;
    }
    buffer[i] = (uint8_t)b;
  }

  saveProgramToEEPROM(buffer, incomingSize);
  Serial.println("UPLOAD_OK");
}

void executeNextInstruction() {
  if (!programLoaded || programSize == 0) return;

  uint8_t op = nextByte();

  switch (op) {
    case OP_PINMODE: {
      uint8_t pin = nextByte();
      uint8_t mode = nextByte();
      pinMode(pin, mode ? OUTPUT : INPUT);
      break;
    }

    case OP_WRITE: {
      uint8_t pin = nextByte();
      uint8_t value = nextByte();
      digitalWrite(pin, value ? HIGH : LOW);
      break;
    }

    case OP_DELAY: {
      uint16_t ms = nextByte();
      ms |= ((uint16_t)nextByte() << 8);
      delay(ms);
      break;
    }

    default: {
      pc = 0;
      break;
    }
  }
}

void setup() {
  Serial.begin(115200);
  loadProgramFromEEPROM();
}

void loop() {
  checkForUpload();
  executeNextInstruction();
}