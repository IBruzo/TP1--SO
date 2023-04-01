# TP1--SO
el tp 1 de la materia sistemas operativos 1c2023
Grupo Joaquin Girod, Chris Ijjas, Ignacio Bruzone, Iñaki Bengolea


Tips y Nociones :
- Compilar con -Wall, utilizar Valgrind y utilizar PVS-Studio
- Hay dos puntos de división, el primera instancia cada slave toma una cantidad >1 de archivos, y en segunda instancia el master regula una cantidad mayor de archivos. Por ejemplo, cada slave toma 3 archivos, hay 3 slaves, pero la cantidad de archivos es 18, entonces en al iniciar el proceso 9 de los 18 archivos pasan a los slaves, y los restantes 9 se van alocando a medida que se liberan.
