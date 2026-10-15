#pragma once
#include <stdint.h>
#include <stddef.h>
namespace RegistroConfig {
constexpr uint32_t CAPTURA_MS = 1000, SYNC_MS = 2000, REINTENTO_MS = 10000;
constexpr unsigned COLA = 32, COLA_MONITOR = 2;
constexpr uint8_t CS = 5, SCK = 18, MISO = 19, MOSI = 23;
constexpr uint32_t SPI_HZ = 1000000;
constexpr size_t MAX_CSV = 2048;
constexpr uint32_t FRESCO_ADC_MS = 1000, FRESCO_ENERGIA_MS = 500;
constexpr uint32_t FRESCO_PCNT_MS = 2500, FRESCO_TEMP_MS = 15000;
constexpr uint32_t FRESCO_GPS_MS = 3000, FRESCO_HORA_MS = 3000;
constexpr bool MONITOR_CSV = true, REPORTES_EXTENSOS = false;
constexpr uint32_t TEMPERATURA_MS = 5000;
constexpr char BASE_COMMIT[] = "e4ca276507141481e6e8dc57f8706e6747a8eb69";
constexpr char VERSION[] = "microsd-1";
}
