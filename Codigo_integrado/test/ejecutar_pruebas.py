#!/usr/bin/env python3
"""Pruebas en PC. Primero instalar dependencias con `pio run` o `pio pkg install`."""
from pathlib import Path
import argparse
import csv
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser()
p.add_argument('--ejemplos', type=Path, help='Directorio opcional para exportar fixtures simulados')
args = p.parse_args()
if args.ejemplos:
    args.ejemplos = args.ejemplos.resolve()
    args.ejemplos.mkdir(parents=True, exist_ok=True)
gps = root / '.pio/libdeps/esp32dev/TinyGPSPlus/src'
if not gps.is_dir():
    p.error('Falta TinyGPSPlus: ejecutar pio run o pio pkg install en Codigo_integrado')

comunes = ['g++', '-std=c++11', '-Wall', '-Wextra', '-Werror']
casos = [
    ('registro', ['-Itest/display_host', 'test/prueba_registro.cpp', 'src/Registro/RelojCore.cpp',
      'src/Registro/Resumen.cpp', 'src/Registro/Archivo.cpp', 'src/Registro/CSV.cpp',
      'src/Sensores/Energia/Energia.cpp']),
    ('tareas_sd', ['-Wno-misleading-indentation', '-Itest/sd_host', 'test/prueba_tareas_sd.cpp',
      'src/Registro/CSV.cpp', 'src/Registro/Panel.cpp', 'src/Registro/RelojCore.cpp', 'src/Registro/Reloj.cpp', 'src/Registro/Archivo.cpp']),
    ('display', ['-Wno-sign-compare', '-Itest/display_host', 'test/prueba_display_host.cpp',
      'src/Rtos/ColaDisplay.cpp', 'src/Display/Display_COM11441.cpp', 'src/Tareas/TareaDisplay.cpp',
      'src/Sensores/Energia/Energia.cpp']),
    ('gps', ['-DARDUINO=10819', '-Wno-implicit-fallthrough', '-Itest/gps_host', '-Itest/sd_host',
      '-I' + str(gps), 'test/prueba_gps.cpp', 'src/Sensores/GPS/GPS.cpp',
      'src/Registro/RelojCore.cpp', str(gps / 'TinyGPS++.cpp')]),
    ('temperatura', ['-Itest/temperatura_host', '-Itest/sd_host', 'test/prueba_temperatura.cpp',
      'src/Sensores/Temperatura/Temperatura.cpp']),
]
with tempfile.TemporaryDirectory(prefix='ecoypf_pruebas_') as tmp:
    for nombre, fuentes in casos:
        binario = str(Path(tmp) / nombre)
        print(f'PRUEBA {nombre}', flush=True)
        subprocess.run(comunes + fuentes + ['-o', binario], cwd=root, check=True)
        parametros = []
        if args.ejemplos and nombre == 'registro':
            parametros = [str(args.ejemplos / 'datos_SIMULADOS.csv'), str(Path(tmp) / 'extremos_TIPOS_NO_FISICOS.csv')]
        if args.ejemplos and nombre == 'tareas_sd':
            parametros = [str(args.ejemplos / 'metadatos_SIMULADOS.txt')]
        subprocess.run([binario] + parametros, cwd=root, check=True)
    if args.ejemplos:
        with (args.ejemplos / 'datos_SIMULADOS.csv').open(newline='', encoding='utf-8') as f:
            filas = list(csv.reader(f))
        if len(filas[0]) != 70 or any(len(fila) != 70 for fila in filas[1:]):
            raise SystemExit('CSV simulado: se esperaba un esquema estable de 70 columnas')
        if filas[0][-1] != 'registros_perdidos' or any(fila[0] != '2' for fila in filas[1:]):
            raise SystemExit('CSV simulado: esquema o version inesperados')
        if [int(f[2]) for f in filas[1:]] != [0, 1, 2, 3, 7]:
            raise SystemExit('CSV simulado: secuencia y hueco de prueba incorrectos')
        if filas[1][4:9] != ['', '', '', 'SIN_HORA', '']:
            raise SystemExit('CSV simulado: se esperaba inicio sin hora')
        if not (args.ejemplos / 'metadatos_SIMULADOS.txt').is_file():
            raise SystemExit('No se generó fixture de metadatos')
print('RESULTADO: 5 grupos de pruebas de software aprobados. Sin ensayos fisicos.', flush=True)
