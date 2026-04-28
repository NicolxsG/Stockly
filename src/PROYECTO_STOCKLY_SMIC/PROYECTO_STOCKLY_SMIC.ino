#include "HX711.h"

// --- Pines Celda y Sensores ---
const int DOUT_PIN = 21;
const int SCK_PIN = 3;
const int IR_PIN = 19;
const int LED_STATUS = 5;

// --- Pines Puente H (Motores DC) ---
// Cada motor usa 2 pines para el sentido de giro
const int M1_IN1 = 18; const int M1_IN2 = 17;
const int M2_IN1 = 16; const int M2_IN2 = 4;
const int M3_IN1 = 0;  const int M3_IN2 = 2;

// --- Pines Ultrasonido ---
const int TRIGS[] = {13, 14, 27};
const int ECHOS[] = {12, 15, 26};

// --- Configuración ---
const float PESO_UNITARIO = 0.250; 
const float CALIBRACION = 2280.f;
const int DIST_UMBRAL = 10; 
const unsigned long MAX_TIME = 6000; // Timeout de 6 segundos

HX711 scale;

void setup() {
  Serial.begin(115200);
  
  pinMode(IR_PIN, INPUT);
  pinMode(LED_STATUS, OUTPUT);
  
  // Configurar motores
  pinMode(M1_IN1, OUTPUT); pinMode(M1_IN2, OUTPUT);
  pinMode(M2_IN1, OUTPUT); pinMode(M2_IN2, OUTPUT);
  pinMode(M3_IN1, OUTPUT); pinMode(M3_IN2, OUTPUT);

  for(int i=0; i<3; i++) {
    pinMode(TRIGS[i], OUTPUT);
    pinMode(ECHOS[i], INPUT);
  }

  scale.begin(DOUT_PIN, SCK_PIN);
  scale.set_scale(CALIBRACION);
  scale.tare(); // Pone a cero la góndola [cite: 130]
}

float obtenerDistancia(int i) {
  digitalWrite(TRIGS[i], LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGS[i], HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGS[i], LOW);
  return pulseIn(ECHOS[i], HIGH) * 0.034 / 2;
}

void controlarMotor(int id, bool encender) {
  int p1, p2;
  if(id == 0) { p1 = M1_IN1; p2 = M1_IN2; }
  else if(id == 1) { p1 = M2_IN1; p2 = M2_IN2; }
  else { p1 = M3_IN1; p2 = M3_IN2; }

  if(encender) {
    digitalWrite(p1, HIGH); // Giro hacia adelante
    digitalWrite(p2, LOW);
  } else {
    digitalWrite(p1, LOW);
    digitalWrite(p2, LOW);
  }
}

void moverBanda(int id) {
  unsigned long t_inicio = millis();
  
  // Mover mientras no haya producto al frente
  while (obtenerDistancia(id) > DIST_UMBRAL) {
    if (millis() - t_inicio > MAX_TIME) break; // Evita giro infinito
    controlarMotor(id, true);
    delay(50); 
  }
  controlarMotor(id, false);
}

void loop() {
  float peso = scale.get_units(5);
  int stock = (peso > 0.05) ? (int)(peso / PESO_UNITARIO) : 0; // 
  bool hayGente = (digitalRead(IR_PIN) == LOW); // [cite: 132]

  // Lógica: Si falta stock y nadie está tocando la góndola [cite: 140]
  if (stock < 6 && !hayGente) {
    for(int i=0; i<3; i++) {
      if (obtenerDistancia(i) > DIST_UMBRAL) {
        moverBanda(i);
      }
    }
  }

  // Alerta visual de stock bajo [cite: 135, 151]
  digitalWrite(LED_STATUS, (stock <= 2) ? HIGH : LOW);

  Serial.print("Peso: "); Serial.print(peso);
  Serial.print(" | Stock: "); Serial.println(stock);
  
  delay(1000);
}