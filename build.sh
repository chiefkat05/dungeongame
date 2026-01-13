echo 'building program'
set -e
gcc main.c -o main -lglfw -lGLEW -lGL -lm
./main