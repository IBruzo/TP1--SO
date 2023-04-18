# TP1--SO
El TP 1 de la materia sistemas operativos 1C2023
Grupo : Joaquin Girod, Chris Ijjas, Ignacio Bruzone, Iñaki Bengolea

- Definicion de Archivos Ejecutables : 
El ejecutable master toma por parametros los archivos que se desean codificar, utiliza una cantidad arbitraria de procesos esclavos para aprovechar la independencia de este mecanismo y paralelizar la codificacion, aumentando la eficiencia.
El ejecutable vista toma por parametro la cantidad de archivos ingresados a master e imprime por salida estandar el resultado de las codificaciones que logra el master en tiempo real. Esto lo logra utilizando metodos de sincronizacion y memoria compartida. Se puede ejecutar tanto de forma aislada como por medio de un pipe.

- Formato de Ejecucion :
./master [ filesToHash ]
./vista [ argc ]

- Comando utilizado para la creacion de los Archivos de Prueba : 
rm testFiles/*; cd testFiles/;for i in {1..87}; do echo "This is some sample text for file $i" > file_$i.txt; done; cd ..

- Comandos utilizados para ejecutar :
Opcion 1 :  make clean; make all; ./master.out ./testFiles/*
Opcion 2 :  make clean; make all; ./master.out ./testFiles/* | ./vista.out
Opcion 3 :  make clean; make all; ./master.out ./testFiles/* 
            ./vista.out [ argc ]     (( en otra terminal ))

