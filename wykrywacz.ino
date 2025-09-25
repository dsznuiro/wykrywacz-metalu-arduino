#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Ustawienia ekranu OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Definicje pinów
#define pulsePin A0       // Wyjście do "pobudzania" cewki
#define sensePin A1       // Wejście do pomiaru odpowiedzi cewki
#define potPin A2         // Potencjometr do ustawienia progu czułości
#define ledGreen 6        // Zielona dioda
#define ledYellow 7       // Żółta dioda
#define ledRed 8          // Czerwona dioda
#define buzzerPin 12      // Buzzer

// Zmienne do kontroli buzzera
unsigned long lastBeepTime = 0;
bool buzzerState = false;
const unsigned long yellowBeepInterval = 300; // Interwał dla stanu żółtego (ms)

// Zmienne do optymalizacji odświeżania OLED
long lastReading = 0;
int lastThreshold = 0;

void setup() {
  Serial.begin(9600);

  // Ustawienie trybów pinów
  pinMode(pulsePin, OUTPUT);
  pinMode(sensePin, INPUT);
  pinMode(potPin, INPUT);
  pinMode(ledGreen, OUTPUT);
  pinMode(ledYellow, OUTPUT);
  pinMode(ledRed, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(pulsePin, LOW);

  // Inicjalizacja OLED
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Wykrywacz metalu");
  display.display();
  delay(1000);
}

void loop() {
  // Pomiar średniej wartości z cewki
  long sum = 0;
  for (int i = 0; i < 100; i++) {
    digitalWrite(pulsePin, HIGH);
    delayMicroseconds(10);
    digitalWrite(pulsePin, LOW);
    delayMicroseconds(10);
    sum += analogRead(sensePin);
  }
  long average = sum / 100;

  // Odczyt potencjometru i przeskalowanie wartości na próg
  int potValue = analogRead(potPin);
  int threshold = map(potValue, 0, 1023, 700, 1000);
  int yellowThreshold = threshold - 10;
  int redThreshold = threshold - 20;

  // Sterowanie diodami LED w zależności od pomiaru
  digitalWrite(ledGreen, average < threshold ? HIGH : LOW);
  digitalWrite(ledYellow, average < yellowThreshold ? HIGH : LOW);
  digitalWrite(ledRed, average < redThreshold ? HIGH : LOW);

  // Sterowanie buzzerem
  if (average < redThreshold) {
    // Ciągły dźwięk przy wykryciu metalu blisko
    digitalWrite(buzzerPin, HIGH);
  } else if (average < yellowThreshold) {
    // Krótkie piski co 300ms w średniej odległości
    unsigned long currentMillis = millis();
    if (currentMillis - lastBeepTime >= yellowBeepInterval) {
      buzzerState = !buzzerState;
      digitalWrite(buzzerPin, buzzerState ? HIGH : LOW);
      lastBeepTime = currentMillis;
    }
  } else {
    // Brak dźwięku
    digitalWrite(buzzerPin, LOW);
    buzzerState = false;
  }

  // Odśwież OLED tylko jeśli coś się zmieniło
  if (abs(average - lastReading) > 5 || abs(threshold - lastThreshold) > 5) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Pomiar: ");
    display.println(average);
    display.print("Prog: ");
    display.println(threshold);
    display.print("Czulosc: ");
    display.print(map(potValue, 0, 1023, 0, 100));
    display.println("%");

    display.setCursor(0, 40);
    if (average < redThreshold) {
      display.println("!!! METAL BLISKO !!!");
    } else if (average < yellowThreshold) {
      display.println("Metal w poblizu");
    } else {
      display.println("Brak metalu");
    }

    display.display();

    // Zapamiętaj ostatnie wartości do porównania
    lastReading = average;
    lastThreshold = threshold;
  }

  delay(50); // Małe opóźnienie dla stabilności
}
