#include <Arduino.h>
#include <math.h>

const byte PIN_SENSOR = A0;

const float VREF_ADC = 5.0;
const float FACTOR_CALIBRACION = 124.5;

const unsigned int NUM_MUESTRAS = 400;
const unsigned long PERIODO_MUESTREO_US = 250;

unsigned int muestras[NUM_MUESTRAS];

void adquirirMuestras() {
  unsigned long siguiente = micros();

  for (unsigned int i = 0; i < NUM_MUESTRAS; i++) {
    while ((long)(micros() - siguiente) < 0) {
      // Esperar al instante de muestreo
    }

    muestras[i] = analogRead(PIN_SENSOR);
    siguiente += PERIODO_MUESTREO_US;
  }
}

void procesarMuestras() {
  unsigned long suma = 0;
  unsigned int minimo = 1023;
  unsigned int maximo = 0;

  for (unsigned int i = 0; i < NUM_MUESTRAS; i++) {
    suma += muestras[i];

    if (muestras[i] < minimo) minimo = muestras[i];
    if (muestras[i] > maximo) maximo = muestras[i];
  }

  float promedio = (float)suma / NUM_MUESTRAS;
  double sumaCuadrados = 0;

  for (unsigned int i = 0; i < NUM_MUESTRAS; i++) {
    float ac = muestras[i] - promedio;
    sumaCuadrados += ac * ac;
  }

  float rmsADC = sqrt(sumaCuadrados / NUM_MUESTRAS);
  float rmsSensor = rmsADC * VREF_ADC / 1023.0;
  float voltajeRed = rmsSensor * FACTOR_CALIBRACION;

  // Detectar cruces ascendentes por el nivel promedio
  float sumaPeriodos = 0;
  unsigned int cantidadPeriodos = 0;
  float cruceAnterior = -1;

  for (unsigned int i = 1; i < NUM_MUESTRAS; i++) {
    float anterior = muestras[i - 1];
    float actual = muestras[i];

    if (anterior < promedio && actual >= promedio) {
      float diferencia = actual - anterior;
      if (diferencia <= 0) continue;

      float fraccion = (promedio - anterior) / diferencia;
      float posicion = (i - 1) + fraccion;

      if (cruceAnterior >= 0) {
        sumaPeriodos += posicion - cruceAnterior;
        cantidadPeriodos++;
      }

      cruceAnterior = posicion;
    }
  }

  float frecuencia = 0;

  if (cantidadPeriodos > 0 && sumaPeriodos > 0) {
    float periodoPromedio = sumaPeriodos / cantidadPeriodos;
    frecuencia = 1000000.0 /
        (periodoPromedio * PERIODO_MUESTREO_US);
  }

  Serial.print("Offset DC: ");
  Serial.print(promedio * VREF_ADC / 1023.0, 3);
  Serial.println(" V");

  Serial.print("Minimo ADC: ");
  Serial.println(minimo);

  Serial.print("Maximo ADC: ");
  Serial.println(maximo);

  Serial.print("Vrms sensor: ");
  Serial.print(rmsSensor, 4);
  Serial.println(" V");

  Serial.print("Voltaje de red estimado: ");
  Serial.print(voltajeRed, 2);
  Serial.println(" V RMS");

  Serial.print("Frecuencia: ");
  if (frecuencia > 0) {
    Serial.print(frecuencia, 2);
    Serial.println(" Hz");
  } else {
    Serial.println("No detectada");
  }

  Serial.println("--------------------");
}

void setup() {
  Serial.begin(9600);
  analogReference(DEFAULT);
  pinMode(PIN_SENSOR, INPUT);

  Serial.println("Monitor de voltaje AC");
  Serial.println("Arduino UNO - A0");
}

void loop() {
  adquirirMuestras();
  procesarMuestras();
}
