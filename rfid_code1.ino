#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Keypad.h>

#define SS_PIN 5
#define RST_PIN 4

MFRC522 mfrc522(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Keypad setup
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {32, 33, 25, 26};
byte colPins[COLS] = {27, 14, 12, 13};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

struct TagInfo {
  byte uid[4];
  String tabletName;
  float price;
};

TagInfo knownTags[] = {
  {{0x75, 0x5C, 0x77, 0xAE}, "Paracetamol", 5.50},
  {{0xF3, 0xCD, 0x64, 0x31}, "Antacid Syrup", 69.00},
  {{0xE3, 0x7E, 0x0F, 0x28}, "Cetirizine", 1.85},
  {{0xC9, 0xE7, 0x76, 0xC2}, "Aspirin", 3.00},
  {{0x69, 0x90, 0x76, 0xC2}, "Dextromethorphan", 83.00},
  {{0x79, 0x3E, 0x76, 0xC2}, "Acebrophylline 100mg", 18.00},
  {{0xD7, 0xF9, 0x96, 0x5F}, "Acetazolamide 250 mg", 20.00},
  {{0x99, 0x04, 0x01, 0xBA}, "Acyclovir 200 mg", 24.00},
  {{0xC9, 0xE7, 0x76, 0xC2}, "Aspirin", 3.00},
  {{0x75, 0x5C, 0x77, 0xAE}, "Paracetamol", 5.50},
  {{0x99, 0x04, 0x01, 0xBA}, "Acyclovir 200 mg", 24.00},
  {{0xE3, 0x7E, 0x0F, 0x28}, "Cetirizine", 1.85},
  {{0xF3, 0xCD, 0x64, 0x31}, "Antacid", 69.00},
  {{0x12, 0x34, 0x56, 0x78}, "Tablet C", 7.75}
};
const int numTags = sizeof(knownTags) / sizeof(knownTags[0]);

struct CartItem {
  String name;
  float price;
  int quantity;
};

CartItem cart[50];
int cartSize = 0;

void setup() {
  Serial.begin(115200);
  SPI.begin();
  mfrc522.PCD_Init();
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("RFID Ready...");
  delay(1500);
  lcd.clear();
}

void loop() {
  char key = keypad.getKey();

  if (key == '*' || key == '#') {
    showTotalBill();
    waitForContinue();
    return;
  }

  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) return;

  int tagIndex = getTagIndex(mfrc522.uid.uidByte, mfrc522.uid.size);
  if (tagIndex != -1) {
    displayTabletInfo(tagIndex);
    int quantity = getQuantityFromKeypad();
    addToCart(knownTags[tagIndex].tabletName, knownTags[tagIndex].price, quantity);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Added to cart");
    delay(1200);
    lcd.clear();
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Unknown Tag");
    delay(1500);
    lcd.clear();
  }

  mfrc522.PICC_HaltA();
}

int getTagIndex(byte* scannedUID, byte size) {
  for (int i = 0; i < numTags; i++) {
    bool match = true;
    for (byte j = 0; j < size; j++) {
      if (scannedUID[j] != knownTags[i].uid[j]) {
        match = false;
        break;
      }
    }
    if (match) return i;
  }
  return -1;
}

void displayTabletInfo(int index) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(knownTags[index].tabletName);
  delay(1000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Price: Rs.");
  lcd.print(knownTags[index].price);
  delay(1000);
  lcd.clear();
}

int getQuantityFromKeypad() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter Qty:");
  String qtyStr = "";
  char key;
  while (true) {
    key = keypad.getKey();
    if (key) {
      if (key >= '0' && key <= '9') {
        qtyStr += key;
        lcd.setCursor(0, 1);
        lcd.print(qtyStr);
      } else if (key == '#') {
        if (qtyStr.length() > 0) break;
      }
    }
  }
  return qtyStr.toInt();
}

void addToCart(String name, float price, int quantity) {
  for (int i = 0; i < cartSize; i++) {
    if (cart[i].name == name) {
      cart[i].quantity += quantity;
      return;
    }
  }
  if (cartSize < 50) {
    cart[cartSize].name = name;
    cart[cartSize].price = price;
    cart[cartSize].quantity = quantity;
    cartSize++;
  }
}

void showTotalBill() {
  float total = 0;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Calculating...");
  delay(1000);

  for (int i = 0; i < cartSize; i++) {
    float itemTotal = cart[i].price * cart[i].quantity;
    total += itemTotal;
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Total Rs:");
  lcd.print(total, 2);
  delay(3000);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Thank You!");
  delay(2000);

  resetCart();  
}

void waitForContinue() {
  lcd.setCursor(0, 1);
  while (true) {
    char key = keypad.getKey();
    if (key == '*' || key == '#') {
      lcd.clear();
      break;
    }
  }
}

void resetCart() {
  cartSize = 0;
}