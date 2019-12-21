/**
 * Wiegand-26 cardreader emulator (scetch for Arduino TE-MINI328)
 *
 * @author: Alexander Korolev (avkw@bk.ru)
 */

#include "pins_arduino.h"
/*
 * an extension to the interrupt support for arduino.
 * add pin change interrupts to the external interrupts, giving a way
 * for users to have interrupts drive off of any pin.
 * Refer to avr-gcc header files, arduino source and atmega datasheet.
 */
 
#define OUT_D0_PIN  17           // Выход D0 - A3
#define OUT_D1_PIN  16           // Выход D1 - A2

String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete
unsigned char facility = 0;      // Код организации для формирования кода карты
boolean facilityInput = false;   // Введён код организации
unsigned long cardCode = 0;      // Код карты (26 бит)

void setup()
{
  Serial.begin(9600);
  Serial.println("TE-MINI CardEmulator v1.0");
  Serial.print("Initialization... ");
  
  // Настройка выходов D0 и D1
  pinMode(OUT_D0_PIN, OUTPUT);
  digitalWrite(OUT_D0_PIN, HIGH);
  pinMode(OUT_D1_PIN, OUTPUT);
  digitalWrite(OUT_D1_PIN, HIGH);

  // Зарезервировать 40 байт для приёма данных
  inputString.reserve(40);
  
  Serial.println("OK");
  Serial.println("Wiegand-26 output: A3 -> Data0, A2 -> Data1.");
  Serial.println("Usage: \"<card_number>\\n\" or \"<facility>,<number>\\n\".");
}

void sendCardCode()
{
  cardCode <<= 1;
  cardCode |= 1;

  // Вычислить бит чётности (0)  
  for ( int i = 1; i < 13; ++i )
  {
    if ( cardCode & (1 << i) )
      cardCode ^= 1;
  }
  
  // Вычислить бит нечётности (25)
  for ( int i = 13; i < 25; ++i )
  {
    if ( cardCode & (1L << i) )
      cardCode ^= (1L << 25);
  }
  
  // Выдача кода карты (26 бит)
  noInterrupts();
  for ( int i = 25; i >= 0; i-- )
  {
    if ( cardCode & (1L << i) )
      digitalWrite(OUT_D1_PIN, LOW);
    else
      digitalWrite(OUT_D0_PIN, LOW);
    delayMicroseconds(200);
    digitalWrite(OUT_D0_PIN, HIGH);
    digitalWrite(OUT_D1_PIN, HIGH);
    delayMicroseconds(1800);
  }
  interrupts();
}

void loop()
{
  if ( stringComplete )
  {
    cardCode = inputString.toInt();
    // clear the string:
    inputString = "";
    stringComplete = false;

    if ( facilityInput )
    {
      facilityInput = false;
      if ( cardCode & 0xFFFF0000 )
      {
        Serial.println("Wrong number format");
        return;
      }
      cardCode |= ((unsigned long)facility) << 16;
    }
    else
    {
      if ( cardCode & 0xFF000000 )
      {
        Serial.println("Wrong number format");
        return;
      }
    }

    sendCardCode();
    Serial.print((cardCode >> 1) & 0xFFFFFF);
    Serial.print('(');
    Serial.print((cardCode >> 17) & 0xFF);
    Serial.print(',');
    Serial.print((cardCode >> 1) & 0xFFFF);
    Serial.println(")-OK");
  }
}

/*
  SerialEvent occurs whenever a new data comes in the
 hardware serial RX.  This routine is run between each
 time loop() runs, so using delay inside loop can delay
 response.  Multiple bytes of data may be available.
 */
void serialEvent()
{
  while ( Serial.available() )
  {
    // get the new byte:
    char inChar = (char)Serial.read();
    if ( isDigit(inChar) )
    {
      inputString += inChar;
    }
    else if ( inChar == '\n' && inputString.length() > 0 )
    {
      stringComplete = true;
    }
    else if ( inChar == ',' && inputString.length() > 0 )
    {
      facility = inputString.toInt();
      facilityInput = true;
      inputString = "";
    }
    else
    {
      facilityInput = false; 
      inputString = "";
    }
  }
}

