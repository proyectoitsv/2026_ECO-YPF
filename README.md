# Desafío ECO YPF Electrónica 2026

## Proyecto Integrador

**Instituto Técnico Salesiano Villada**  
**Proyecto Final de Electrónica — 7.º B · 2026**

**Integrantes:** Felipe Lamberghini Nicklison, Martino Righetti, Facundo Borgiattino y Tomás Carranza.  
**Profesores:** Federico Ferraro e Ignacio Gil.

---


## Descripción general del proyecto

El Desafío ECO YPF es una competencia educativa y deportiva en la que estudiantes de escuelas técnicas de Argentina diseñan, construyen y ponen en pista vehículos eléctricos. El Instituto Técnico Salesiano Villada participa con un monoplaza construido por estudiantes de distintas especialidades a partir de los componentes de propulsión y energía del vehículo.

El auto utiliza un motor eléctrico alimentado por un conjunto de baterías. La energía disponible debe administrarse durante las pruebas, por lo que conocer el comportamiento del vehículo ayuda al equipo a prepararlo, interpretar su consumo y tomar decisiones durante la competencia.

Como proyecto final de Electrónica se desarrolló un sistema embebido de adquisición de datos instalado en el auto. Su circuito recibe señales de sensores, el ESP32 las procesa, muestra al piloto la velocidad y una estimación de batería, y conserva las mediciones para que el equipo pueda analizarlas después. Así, el piloto cuenta con una referencia durante el recorrido y el equipo puede relacionar consumo, movimiento y temperaturas al revisar las pruebas.

El sistema está compuesto por una placa con un ESP32, sensores de tensión, corriente, velocidad y temperatura, un módulo GPS, un display y una tarjeta microSD. El programa recibe las mediciones y las utiliza para calcular la potencia y el consumo acumulado de las baterías.

## Objetivo del proyecto

El objetivo fue reunir estas funciones en un sistema que pudiera utilizarse desde la cabina del auto. El piloto necesitaba consultar la velocidad y tener una referencia de la batería disponible durante el recorrido. Para el equipo era necesario contar con un registro que permitiera relacionar el consumo con el movimiento del vehículo y revisar su comportamiento después de las pruebas. Para que posteriormente se pueda sumar la especialidad de Programación, para que puedan analizar y gestionar esos datos, haciendo que el proyecto sea de todo séptimo año, en un futuro.

Se buscó que la electrónica comenzara a trabajar al accionar su interruptor, sin depender de una computadora ni de comandos para iniciar cada medición. También debía conservar el consumo registrado cuando se apagara, para continuar desde el último valor guardado al volver a encenderla.
