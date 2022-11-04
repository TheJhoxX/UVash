# UVash
Práctica realizada para la asignatura de Estructura de Sistemas Operativos para la Universidad de Valladolid en el curso 2021-2022
Se trata de la programación de un shell que cuenta con modos:
- No interactivo: al ejecutar el compilado se le pasa un fichero que contiene los comandos que se quiere que se ejecuten
- Interactivo: el shell va solicitando los comandos en una nueva línea cada uno mediante la entrada estándar

Cada comando se ejecutará como un nuevo proceso.
El programa cuenta con la posibilidad de exportar la salida de comandos a ficheros, en este caso se indica: comando > ficheroSalida
