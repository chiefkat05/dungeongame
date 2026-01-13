echo 'building program'
set -e
gcc main.c -o game -lglfw -lGLEW -lGL -lm
./game