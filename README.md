# TP1--SO
el tp 1 de la materia sistemas operativos 1c2023
Grupo Joaquin Girod, Chris Ijjas, Ignacio Bruzone, Iñaki Bengolea

- ultimo comando de testing: make clean; rm results.txt; make all; ./master.out master.c master.c master.c master.c master.c master.c slave.c README.md master.c master.c; make clean

- ACLARACION : la salida no es estable porque hace polling, puede que haya menos archivos en el results.txt

-stdout :       Arguments received : 10
                #Slaves            : 2
                Initial Load       : 1
                Creacion de esclavo 1
                bytes read : 43
                bytes read : 43
                bytes read : 43
                bytes read : 43
                bytes read : 43
                bytes read : 43
                bytes read : 42
                bytes read : 44
                bytes read : 43
                bytes read : 43


-results.txt :  6c05908b8942065c876615d2d376b87f  master.c
                6c05908b8942065c876615d2d376b87f  master.c
                6c05908b8942065c876615d2d376b87f  master.c
                6c05908b8942065c876615d2d376b87f  master.c
                3ed222f2d2fb03a1795dbcc3a040533e  README.md
                6c05908b8942065c876615d2d376b87f  master.c
                6c05908b8942065c876615d2d376b87f  master.c
                6c05908b8942065c876615d2d376b87f  master.c
                8764be1ebdd99eac9d9418defd99c2e7  slave.c
                6c05908b8942065c876615d2d376b87f  master.c

