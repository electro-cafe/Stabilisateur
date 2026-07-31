#include <Arduino.h>
#include <cmath> // pour utiliser la fonction pow() si nécessaire
#include <iostream>

// la valeur neutre du joysticke devrait etre 2048 mais dans les faits, après
// mesure, on a 3000. on a donc 25% de définition pour l'axe positif et 75% pour
// l'axe négatif.
//  Pin connected to the hw 504 joystick
const int JoystickX = 4;
const int JoystickY = 5;
const int JoystickButton = 13;

// variable for storing the joystick value
int joystickValueX = 0;
int joystickValueY = 0;
bool joystickButtonState = false;

int zeroedjoystickX = 0;
int zeroedjoystickY = 0;

int dataSample = 8;

// indique de combien de bit on va décaller. plus l'interval est grand plus la
// zone morte est grande.
int interval = 5;

void setup() {
  Serial.begin(115200);
  pinMode(JoystickButton, INPUT_PULLUP);
  delay(1000);
}

// put function definitions here:
void initializeNeutralPosition() {
  // utilisation de long pour éviter un overflow si la somme dépasse la capacité
  // d'un int
  long int sumX = 0;
  long int sumY = 0;

  for (int i = 0; i < dataSample; i++) {
    sumX += analogRead(JoystickX);
    sumY += analogRead(JoystickY);
    delay(100);
  }
  // Attribution aux variables globales
  zeroedjoystickX = sumX / dataSample;
  zeroedjoystickY = sumY / dataSample;
}

// afin d'éviter une / par interval suivi d'une * par interval ce qui demande
// plus de ressource, on peut faire du bit shifting. qui dis bit dit puissance
// de 2 donc l'interval doit être une puissance de 2.
void readJoystick(int x, int y, int z) {
  joystickValueX = (analogRead(x) - zeroedjoystickX) >> interval << interval;
  joystickValueY = (analogRead(y) - zeroedjoystickY) >> interval << interval;
  joystickButtonState = digitalRead(z);
  // affiche les valeurs dans teleplot sous forme de graph
  Serial.print(">joystickX:");
  Serial.println(joystickValueX);
  Serial.print(">joystickY:");
  Serial.println(joystickValueY);

  // affiche les valeurs dans le moniteur série
  // Serial.println(joystickValueX);
  // Serial.println(joystickValueY);
  // Serial.println(joystickButtonState);
}

int testInterval() {
  for (int i = 0; i < 4096; i++) {
    int valueScaled = i >> interval << interval;

    // Convertit le double de pow() en int avant de faire le modulo
    int divider = static_cast<int>(pow(2, interval));

    if (i % divider == 0) {
      Serial.print(">original:");
      Serial.println(i);
      Serial.print(">filtre:");
      Serial.println(valueScaled);
      delay(100); // Ajout d'un délai pour éviter de saturer le moniteur série
    }
  }
  return 0;
}

void loop() {

  // test de l'intervalle de filtrage
  // testInterval();

  // Reading joystick value
  readJoystick(JoystickX, JoystickY, JoystickButton);
  delay(50);
}
