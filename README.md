# TP1--SO
el tp 1 de la materia sistemas operativos 1c2023
Grupo Joaquin Girod, Chris Ijjas, Ignacio Bruzone, Iñaki Bengolea


- ACLARACION : actualmente hay un problema con los buffers, creo que es culpa del strtok, hay que pasarlo a getline()
- ACLARACION2 : no borren los printfs, es mas practico debuggear con eso que ir uno por uno los 3 mil ciclos

- ultimo comando de testing: 'make clean; rm results.txt ;make all; ./master.out ./testFiles/*; make clean'

- STDOUT/consola : Tiene info de pasaje de archivos al pipe

- results.txt : Tiene lo que queremos pero esta el problema de los buffers cuando el initialLoad es mayor a 1 ( linea 50 )

- comando de bash que tira chatgpt, para crear la carpeta con los 20 archivos y que cada uno tenga un poco de texto ( algo de texto para la codificacion, creo que afecta ) ( Joaco no uso ese script, no sabe si funca ) :

#!/bin/bash

# Create directory
mkdir testFiles

# Loop through letters A to T
for i in {65..84}; do
  # Create file with respective letter
  touch testFiles/$(printf '%b' $(printf '\\x%x' $i)).txt

  # Write letter 20 times to file
  printf $(printf '\\x%x' $i)%.0s {1..20} >> testFiles/$(printf '%b' $(printf '\\x%x' $i)).txt
done





