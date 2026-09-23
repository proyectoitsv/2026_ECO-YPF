#include "Corriente/Corriente.h"

// =====================================================
// HARDWARE
// =====================================================
const int PIN_SENSOR = 2;

// =====================================================
// MODO DE ALIMENTACIÓN DEL SENSOR
// =====================================================
const bool ALIMENTACION_5V = true;

static const float vccSensor = ALIMENTACION_5V ? 5.0 : 3.3;
static const float sensibilidad_mV_A_5V = 40.0;  // ACS758-050B a 5V (datasheet)
static const float sensibilidad_mV_A = sensibilidad_mV_A_5V * (vccSensor / 5.0);

static const float vccADC = 3.3;          // referencia del ADC del micro
static const int   resolucionADC = 4095;  // 12 bits

static const float cuentasPorAmpere = (sensibilidad_mV_A / 1000.0) * (resolucionADC / vccADC);

// Calibración inicial
static const int MUESTRAS_CAL_INICIAL   = 500;
static const int RETARDO_CAL_INICIAL_MS = 2;

// Autocalibración dinámica
static const float UMBRAL_REPOSO_A   = 0.15;
static const int   CICLOS_REPOSO_REQ = 5;
static const float ALPHA_AJUSTE      = 0.05;

// Filtro anti-pico (media recortada)
static const int N_MUESTRAS  = 15;
static const int N_DESCARTAR = 2;

// Estado interno
static int   valorADC[N_MUESTRAS];
static int   j = 0;
static float adcReposo = 0;
static int   ciclosEnReposo = 0;
static float corrienteActual = 0;
static int   promedioActual = 0;

static float calibrarOffset(int muestras, int retardoMs) {
  long suma = 0;
  for (int i = 0; i < muestras; i++) {
    suma += analogRead(PIN_SENSOR);
    delay(retardoMs);
  }
  return (float)suma / muestras;
}

static void ordenarMuestras(int arr[], int n) {
  for (int i = 0; i < n - 1; i++) {
    for (int k = 0; k < n - i - 1; k++) {
      if (arr[k] > arr[k + 1]) {
        int temp = arr[k];
        arr[k] = arr[k + 1];
        arr[k + 1] = temp;
      }
    }
  }
}

static int mediaRecortada(int arr[], int n, int descartar) {
  ordenarMuestras(arr, n);
  long suma = 0;
  int cuenta = 0;
  for (int i = descartar; i < n - descartar; i++) {
    suma += arr[i];
    cuenta++;
  }
  return suma / cuenta;
}

void inicializarSensorCorriente() {
  pinMode(PIN_SENSOR, INPUT);
  adcReposo = calibrarOffset(MUESTRAS_CAL_INICIAL, RETARDO_CAL_INICIAL_MS);
}

bool actualizarSensorCorriente() {
  valorADC[j] = analogRead(PIN_SENSOR);
  j++;

  if (j >= N_MUESTRAS) {
    promedioActual = mediaRecortada(valorADC, N_MUESTRAS, N_DESCARTAR);
    corrienteActual = (promedioActual - adcReposo) / cuentasPorAmpere;

    if (abs(corrienteActual) < UMBRAL_REPOSO_A) {
      ciclosEnReposo++;
      if (ciclosEnReposo >= CICLOS_REPOSO_REQ) {
        adcReposo = adcReposo + ALPHA_AJUSTE * (promedioActual - adcReposo);
      }
    } else {
      ciclosEnReposo = 0;
    }

    j = 0;
    return true;
  }

  return false;
}

float obtenerCorriente() {
  return corrienteActual;
}

int obtenerPromedioADC() {
  return promedioActual;
}

float obtenerOffset() {
  return adcReposo;
}

void recalibrarSensorCorriente() {
  adcReposo = calibrarOffset(MUESTRAS_CAL_INICIAL, RETARDO_CAL_INICIAL_MS);
  ciclosEnReposo = 0;
}