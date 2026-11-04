#pragma once
#include <stdint.h>
// Se llaman antes de lanzar adquisicion. No montan ni esperan a la SD.
bool prepararRegistro();
void comenzarSesionRegistro();
bool crearTareasRegistro();
// Unico lector Serial RX: la tarea Energia llama a este despachador.
bool procesarComandoRegistro(const char*);
