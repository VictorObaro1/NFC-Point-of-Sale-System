#include <Wire.h>
#include <PN532_I2C.h>
#include <PN532.h>
#include <ArduinoJson.h>
#include <Thread.h>
#include <NfcAdapter.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>


PN532_I2C pn532_i2c(Wire);
NfcAdapter nfc = NfcAdapter(pn532_i2c);
LiquidCrystal_I2C lcd(0x27, 20, 4);
StaticJsonDocument<200> doc;

const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns

#define buzzer A1
#define deviceId "9913804724"
#define pin "1234"

bool flag = true;
char key = ' ';
uint8_t screen;
uint8_t CursorPosition = 0;
bool display_clear_flag;
String Inputed_Amount = "";
uint8_t i, j;

char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte rowPins[ROWS] = {6, 7, 8, 9}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {2, 3, 4, 5}; //connect to the column pinouts of the keypad

//initialize an instance of class NewKeypad
Keypad customKeypad = Keypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

void setup(void) {
  Serial.begin(115200);
  //Serial.println("NDEF Reader");
  pinMode(buzzer, OUTPUT);
  nfc.begin();
  lcd.init();      // initialize the lcd
  lcd.backlight(); // Turns On BackLight
  lcd.clear();
  lcd.setCursor(6, 1);
  lcd.print("NFC POS ");
  lcd.setCursor(6, 2);
  lcd.print("TERMINAL");
  display_clear_flag = true;
}


class KeyPad {
  public:
    bool soundFlag;
    void keySound() {
      flag = false;
      if (key == '*')
      {
        soundFlag = ! soundFlag;
      }
    }
    void DisplayKeyPress() {
      lcd.print(key);
    }
};

class MENU {
  public:
    void displayMenu() {
      lcd.setCursor(4, 0);
      lcd.print("MAKE PAYMENT");
      lcd.setCursor(3, 1);
      lcd.print("RECEIVE PAYMENT");
      lcd.setCursor(4, 2);
      lcd.print("CHECK BALANCE");
    }

    void make_payment() {
      doc["transactionType"] = "make";
      doc["deviceId"] = deviceId;
      if (keyFunction()) {
        screen = 5;
      }
    }

    void receive_payment() {
      doc["transactionType"] = "receive";
      doc["deviceId"] = deviceId;
      if (keyFunction()) {
        screen = 5;
      }
    }

    void check_balance() {
      if (display_clear_flag == false) {
        lcd.clear();
      }
      display_clear_flag = true;
      lcd.setCursor(3, 0);
      lcd.print("DEVICE BALANCE");
      lcd.setCursor(4, 1);
      lcd.print("CARD BALANCE");
    }


    bool tagReader()
    {
      if (nfc.tagPresent())
      {
        String UID = "";
        NfcTag tag = nfc.read();
        UID = tag.getUidString();
        UID.replace(" ", "");

        if (tag.hasNdefMessage())
        {
          NdefMessage message = tag.getNdefMessage();
          uint8_t recordCount = message.getRecordCount();
          for (uint8_t i = 0; i < recordCount; i++)
          {
            //Serial.print("\nNDEF Record "); Serial.println(i + 1);
            NdefRecord record = message.getRecord(i);

            uint8_t payloadLength = record.getPayloadLength();
            byte payload[payloadLength];
            record.getPayload(payload);

            if (i == 0) {
              String cardNumber = ""; // Processes the message as a string vs as a HEX value
              for (uint8_t c = 3; c < payloadLength; c++) {
                cardNumber += (char)payload[c];
              }
              doc["cardNumber"] = cardNumber;
            }

            /*if (i == 1) {
              String walletBalance = ""; // Processes the message as a string vs as a HEX value
              for (uint8_t c = 3; c < payloadLength; c++) {
                walletBalance += (char)payload[c];
              }
              doc["walletBalance"] = walletBalance;
              }*/
          }
          serializeJson(doc, Serial);
        }
        return true;
      }
      return false;
    }

    void Read_Card() {
      if (display_clear_flag == true) {
        lcd.clear();
      }
      display_clear_flag = false;
      //String Confirmed_Amount = doc ["amount"];
      String Confirmed_Amount = Inputed_Amount;
      Inputed_Amount = "";
      //doc ["amount"] = Confirmed_Amount;
      lcd.setCursor(0, 1);
      lcd.print("AMT ENTERED #");
      lcd.setCursor(13, 1);
      lcd.print(Confirmed_Amount);
      lcd.setCursor(0, 2);
      lcd.print("PLACE YOUR CARD");
      if (tagReader()) {
        screen = 6;
      }
    }


    bool keyFunction() {
      if (display_clear_flag == false) {
        i = 14;
        j = 0;
        lcd.clear();
        if (key == 'D') {
          i--;
          key = ' ';
        }
      }
      display_clear_flag = true;
      // clear display and enter is pressed

      lcd.setCursor(0, 0);
      lcd.print("INPUT AMOUNT: ");
      if (key) {
        if (i > 14 && key == '#') {
          j--;
          Inputed_Amount.remove(j);
          i--;
          key = ' ';
          lcd.setCursor(i, 0);
          lcd.print(" ");
        }
        //clear last single numerical input on the dispaly

        else if (key != '#') {
          if (key == 'D') {
            if (i > 14) {
              key = ' ';
              lcd.setCursor(i, 0);
              lcd.print(" ");
              lcd.setCursor(0, 1);
              Inputed_Amount.remove(0, 1);
              doc ["amount"] = Inputed_Amount;

              //Inputed_Amount = "";
              return true;
            }
          }

          else {
            lcd.setCursor(i, 0);
            lcd.print(key);
            i++;
            Inputed_Amount += key;
            j++;
          }
          // print out numerical input on the display and increment cursor position
        }
        KeyPad sound;
        sound.keySound();
      }
      return false;
    }

    void Send_Transaction() {
      if (display_clear_flag == false) {
        lcd.clear();
        lcd.setCursor(3, 1);
        lcd.print("PROCESSING...");
      }
      display_clear_flag = true;
      DeserializationError err = deserializeJson(doc, Serial);

      if (!err) {
        String statuscode = doc["statuscode"].as<String>();
        String body = doc["body"].as<String>();
        String message = doc["message"].as<String>();
        doc.clear();
        lcd.setCursor(0, 1);
        lcd.print("Status Code: ");
        lcd.setCursor(13, 1);
        lcd.print(statuscode);
        lcd.setCursor(0, 0);
        lcd.print(message);
        lcd.setCursor(0, 3);
        lcd.print("Press Back");
        screen = 7;
        if (statuscode == "200") {
          digitalWrite(buzzer, HIGH);
          delay(1000);
          digitalWrite(buzzer, LOW);
        }
      }
    }

    void Return_Home() {

    }

    void scroll() {
      switch (key) {
        case 'A': //Navigate up command
          CursorPosition--;
          break;
        case 'B': //Navigate down command
          CursorPosition++;
          break;
      }

      if (CursorPosition > 2) {
        CursorPosition = 0;
      }
      switch (CursorPosition) {
        case 0 :
          lcd.setCursor(2, 0);
          lcd.print("<");
          lcd.setCursor(17, 0);
          lcd.print(">");

          lcd.setCursor(1, 1);
          lcd.print(" ");
          lcd.setCursor(19, 1);
          lcd.print(" ");

          lcd.setCursor(2, 2);
          lcd.print(" ");
          lcd.setCursor(18, 2);
          lcd.print(" ");
          break;

        case 1 :
          lcd.setCursor(1, 1);
          lcd.print("<");
          lcd.setCursor(19, 1);
          lcd.print(">");

          lcd.setCursor(2, 0);
          lcd.print(" ");
          lcd.setCursor(17, 0);
          lcd.print(" ");

          lcd.setCursor(2, 2);
          lcd.print(" ");
          lcd.setCursor(18, 2);
          lcd.print(" ");
          break;

        case 2 :
          lcd.setCursor(2, 2);
          lcd.print("<");
          lcd.setCursor(18, 2);
          lcd.print(">");

          lcd.setCursor(1, 1);
          lcd.print(" ");
          lcd.setCursor(19, 1);
          lcd.print(" ");

          lcd.setCursor(2, 0);
          lcd.print(" ");
          lcd.setCursor(17, 0);
          lcd.print(" ");
          break;
      }
    }
};


void loop(void) {
  key = customKeypad.getKey();
  digitalWrite(buzzer, LOW);

  KeyPad KEY;
  MENU menu;
  KEY.keySound();


  if (key == 'C') {
    screen = 1;
    if (display_clear_flag) {
      lcd.clear();
    }
    display_clear_flag = false;
    menu.displayMenu();
  }
  else if ( CursorPosition == 0 && key == 'D')
    screen = 2;
  else if ( CursorPosition == 1 && key == 'D')
    screen = 3;
  else if ( CursorPosition == 2 && key == 'D')
    screen = 4;


  switch (screen) {
    case 1:
      menu.scroll();
      break;
    case 2:
      menu.make_payment();
      break;
    case 3:
      menu.receive_payment();
      break;
    case 4:
      menu.check_balance();
      break;
    case 5:
      menu.Read_Card();
      break;
    case 6:
      menu.Send_Transaction();
      break;
    case 7:
      menu.Return_Home();
      break;
  }
}

