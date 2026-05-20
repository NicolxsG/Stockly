//////////////////////////////////////////////////////////////
// ESTANTERIA INTELIGENTE
// ESP32-S3
// THINGER.IO
// HX711 + 4 CELDAS DE CARGA
// 3 SENSORES HC-SR04
// 3 MOTORES BTS7960 / HW-039
//////////////////////////////////////////////////////////////

#include <HX711.h>
#include <ThingerESP32.h>

//////////////////////////////////////////////////////////////
// THINGER.IO
//////////////////////////////////////////////////////////////

#define USERNAME "GioTorr"
#define DEVICE_ID "esp32_OP_NG_GT"
#define DEVICE_CREDENTIAL "esp32123"

#define SSID "Gio_Galaxy S23"
#define PASSWORD "AlejoVilla20"

ThingerESP32 thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);

//////////////////////////////////////////////////////////////
// HX711
//////////////////////////////////////////////////////////////

#define DOUT 6
#define CLK 7

HX711 balanza;

//////////////////////////////////////////////////////////////
// PESO PRODUCTO
//////////////////////////////////////////////////////////////

#define PESO_PRODUCTO 200.0

//////////////////////////////////////////////////////////////
// ULTRASONIDOS
//////////////////////////////////////////////////////////////

// BANDA 1

#define PROBAR_SOLO_BANDA_1 true
#define TRIG1 10
#define ECHO1 11

// BANDA 2
#define TRIG2 12
#define ECHO2 13

// BANDA 3
#define TRIG3 14
#define ECHO3 15

//////////////////////////////////////////////////////////////
// BTS7960 / HW-039
//////////////////////////////////////////////////////////////

// MOTOR 1
#define M1_RPWM 4
#define M1_LPWM 5
#define M1_REN 16
#define M1_LEN 17

// MOTOR 2
#define M2_RPWM 18
#define M2_LPWM 8
#define M2_REN 19
#define M2_LEN 20

// MOTOR 3
#define M3_RPWM 21
#define M3_LPWM 35
#define M3_REN 36
#define M3_LEN 37

//////////////////////////////////////////////////////////////
// VARIABLES
//////////////////////////////////////////////////////////////

float pesoTotal = 0;

float productosEstimados = 0;

float distancia1 = 0;
float distancia2 = 0;
float distancia3 = 0;

bool motor1Activo = false;
bool motor2Activo = false;
bool motor3Activo = false;

float pesoDetectado = 0;
float stockPercent = 0;
String estadoStock = "";
unsigned long tiempoReposicion = 0;

bool alertaBajoStock = false;
bool estabaEnBajoStock = false;
unsigned long inicioAlerta = 0;

float pesoVacio = 0;
float pesoMaximo = 1000;


//////////////////////////////////////////////////////////////
// CONFIGURACION
//////////////////////////////////////////////////////////////

#define VELOCIDAD_MOTOR 180

// DISTANCIA EN CM PARA ACTIVAR
#define DISTANCIA_ACTIVACION 10

//////////////////////////////////////////////////////////////
// FUNCION ULTRASONIDO
//////////////////////////////////////////////////////////////

float medirDistancia(int trigPin, int echoPin){

  digitalWrite(trigPin, LOW);
  delayMicroseconds(5);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);

  digitalWrite(trigPin, LOW);

  long duracion = pulseIn(echoPin, HIGH, 30000);

  // SI NO HAY LECTURA
  if(duracion == 0){
    return 999;
  }

  float distancia = duracion * 0.034 / 2;

  return distancia;
}

//////////////////////////////////////////////////////////////
// MOTOR ADELANTE
//////////////////////////////////////////////////////////////

void moverMotorAdelante(
  int rpwm,
  int lpwm
){

  ledcWrite(rpwm, VELOCIDAD_MOTOR);

  ledcWrite(lpwm, 0);
}

//////////////////////////////////////////////////////////////
// DETENER MOTOR
//////////////////////////////////////////////////////////////

void detenerMotor(
  int rpwm,
  int lpwm
){

  ledcWrite(rpwm, 0);

  ledcWrite(lpwm, 0);
}
//////////////////////////////////////////////////////////////
//  THINGER.IO
//////////////////////////////////////////////////////////////

void calcularKPIs() {
  pesoDetectado = balanza.get_units(20);

  stockPercent = ((pesoDetectado - pesoVacio) / (pesoMaximo - pesoVacio)) * 100.0;

  if (stockPercent > 100) stockPercent = 100;
  if (stockPercent < 0) stockPercent = 0;

  if (stockPercent >= 70) {
    estadoStock = "ALTO";
    alertaBajoStock = false;
  } else if (stockPercent >= 30) {
    estadoStock = "MEDIO";
    alertaBajoStock = false;
  } else if (stockPercent > 5) {
    estadoStock = "BAJO";
    alertaBajoStock = true;
  } else {
    estadoStock = "VACIO";
    alertaBajoStock = true;
  }
}

void calcularTiempoReposicion() {
  if (stockPercent <= 30 && !estabaEnBajoStock) {
    estabaEnBajoStock = true;
    inicioAlerta = millis();
  }

  if (stockPercent > 70 && estabaEnBajoStock) {
    tiempoReposicion = (millis() - inicioAlerta) / 1000;
    estabaEnBajoStock = false;
  }
}

//////////////////////////////////////////////////////////////
// SETUP
//////////////////////////////////////////////////////////////

void setup() {

  Serial.begin(115200);

  //////////////////////////////////////////////////////////
  // WIFI
  //////////////////////////////////////////////////////////

  thing.add_wifi(SSID, PASSWORD);

  //////////////////////////////////////////////////////////
  // HX711
  //////////////////////////////////////////////////////////

  balanza.begin(DOUT, CLK);

  Serial.println("Calibrando balanza...");

  // FACTOR YA CALIBRADO POR TI
  balanza.set_scale(49.5);

  // HACER CERO
  balanza.tare(50);

  Serial.println("Balanza lista");

  //////////////////////////////////////////////////////////
  // ULTRASONIDOS
  //////////////////////////////////////////////////////////

  pinMode(TRIG1, OUTPUT);
  pinMode(ECHO1, INPUT);

  pinMode(TRIG2, OUTPUT);
  pinMode(ECHO2, INPUT);

  pinMode(TRIG3, OUTPUT);
  pinMode(ECHO3, INPUT);

  //////////////////////////////////////////////////////////
  // ENABLE BTS7960
  //////////////////////////////////////////////////////////

  pinMode(M1_REN, OUTPUT);
  pinMode(M1_LEN, OUTPUT);

  pinMode(M2_REN, OUTPUT);
  pinMode(M2_LEN, OUTPUT);

  pinMode(M3_REN, OUTPUT);
  pinMode(M3_LEN, OUTPUT);

  digitalWrite(M1_REN, HIGH);
  digitalWrite(M1_LEN, HIGH);

  digitalWrite(M2_REN, HIGH);
  digitalWrite(M2_LEN, HIGH);

  digitalWrite(M3_REN, HIGH);
  digitalWrite(M3_LEN, HIGH);

  //////////////////////////////////////////////////////////
  // PWM ESP32-S3
  //////////////////////////////////////////////////////////

  ledcAttach(M1_RPWM, 5000, 8);
  ledcAttach(M1_LPWM, 5000, 8);

  ledcAttach(M2_RPWM, 5000, 8);
  ledcAttach(M2_LPWM, 5000, 8);

  ledcAttach(M3_RPWM, 5000, 8);
  ledcAttach(M3_LPWM, 5000, 8);

  //////////////////////////////////////////////////////////
  // APAGAR MOTORES
  //////////////////////////////////////////////////////////

  detenerMotor(M1_RPWM, M1_LPWM);
  detenerMotor(M2_RPWM, M2_LPWM);
  detenerMotor(M3_RPWM, M3_LPWM);

  //////////////////////////////////////////////////////////
  // THINGER.IO
  //////////////////////////////////////////////////////////

  thing["peso_total"] >> [](pson &out){
    out = pesoTotal;
  };

  thing["productos_estimados"] >> [](pson &out){
    out = productosEstimados;
  };

  thing["distancia_b1"] >> [](pson &out){
    out = distancia1;
  };

  thing["distancia_b2"] >> [](pson &out){
    out = distancia2;
  };

  thing["distancia_b3"] >> [](pson &out){
    out = distancia3;
  };

  thing["motor_b1"] >> [](pson &out){
    out = motor1Activo;
  };

  thing["motor_b2"] >> [](pson &out){
    out = motor2Activo;
  };

  thing["motor_b3"] >> [](pson &out){
    out = motor3Activo;
  };

thing["peso_detectado"] >> [](pson& out){
  out = pesoDetectado;
};

thing["nivel_stock"] >> [](pson& out){
  out = stockPercent;
};

thing["estado"] >> [](pson& out){
  out = estadoStock;
};

thing["tiempo_reposicion"] >> [](pson& out){
  out = tiempoReposicion;
};

  Serial.println("Sistema iniciado");
}



//////////////////////////////////////////////////////////////
// LOOP
//////////////////////////////////////////////////////////////

void loop() {

  //////////////////////////////////////////////////////////
  // THINGER.IO
  //////////////////////////////////////////////////////////

  thing.handle();

  //////////////////////////////////////////////////////////
  // LEER PESO TOTAL
  //////////////////////////////////////////////////////////

  pesoTotal = balanza.get_units(20);

  // EVITAR NEGATIVOS
  if(pesoTotal < 0){
    pesoTotal = 0;
  }

  //////////////////////////////////////////////////////////
  // CALCULAR PRODUCTOS
  //////////////////////////////////////////////////////////

  productosEstimados = pesoTotal / PESO_PRODUCTO;

  //////////////////////////////////////////////////////////
  // LEER ULTRASONIDOS
  //////////////////////////////////////////////////////////

distancia1 = medirDistancia(TRIG1, ECHO1);

if (!PROBAR_SOLO_BANDA_1) {
  distancia2 = medirDistancia(TRIG2, ECHO2);
  distancia3 = medirDistancia(TRIG3, ECHO3);
}

  //////////////////////////////////////////////////////////
  // SERIAL MONITOR
  //////////////////////////////////////////////////////////

  Serial.println("====================================");

  Serial.print("Peso Total: ");
  Serial.print(pesoTotal);
  Serial.println(" g");

  Serial.print("Productos Estimados: ");
  Serial.println(productosEstimados);

  Serial.print("Distancia Banda 1: ");
  Serial.print(distancia1);
  Serial.println(" cm");

  Serial.print("Distancia Banda 2: ");
  Serial.print(distancia2);
  Serial.println(" cm");

  Serial.print("Distancia Banda 3: ");
  Serial.print(distancia3);
  Serial.println(" cm");

  //////////////////////////////////////////////////////////
  // CONTROL BANDA 1
  //////////////////////////////////////////////////////////

  if(distancia1 > DISTANCIA_ACTIVACION &&
     productosEstimados > 0){

    moverMotorAdelante(
      M1_RPWM,
      M1_LPWM
    );

    motor1Activo = true;
  }
  else{

    detenerMotor(
      M1_RPWM,
      M1_LPWM
    );

    motor1Activo = false;
  }

  //////////////////////////////////////////////////////////
  // CONTROL BANDA 2 y 3
  //////////////////////////////////////////////////////////
if (!PROBAR_SOLO_BANDA_1) {

  // CONTROL BANDA 2
  if(distancia2 > DISTANCIA_ACTIVACION && productosEstimados > 0){
    moverMotorAdelante(M2_RPWM, M2_LPWM);
    motor2Activo = true;
  } else {
    detenerMotor(M2_RPWM, M2_LPWM);
    motor2Activo = false;
  }

  // CONTROL BANDA 3
  if(distancia3 > DISTANCIA_ACTIVACION && productosEstimados > 0){
    moverMotorAdelante(M3_RPWM, M3_LPWM);
    motor3Activo = true;
  } else {
    detenerMotor(M3_RPWM, M3_LPWM);
    motor3Activo = false;
  }

} else {
  detenerMotor(M2_RPWM, M2_LPWM);
  detenerMotor(M3_RPWM, M3_LPWM);
  motor2Activo = false;
  motor3Activo = false;
}
  //////////////////////////////////////////////////////////
  // PEQUEÑA ESPERA
  //////////////////////////////////////////////////////////

  
  //////////////////////////////////////////////////////////
  // iOT
  //////////////////////////////////////////////////////////
  calcularKPIs();
  calcularTiempoReposicion();

  delay(200);
}