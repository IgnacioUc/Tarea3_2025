# Tarea 3

Este es un juego de exploración de mazmorras en C, donde el jugador recorre un laberinto representado como un grafo. Cada sala tiene descripciones, objetos con valor y peso, y conexiones hacia otras salas. El objetivo es recolectar el mayor puntaje posible antes de que se acabe el tiempo, teniendo en cuenta que el peso del inventario ralentiza al jugador.

## Compilación

Asegúrate de tener un compilador de C instalado. Luego, en la terminal, ejecuta:

```bash
gcc tarea3.c -o tarea3  -lm
```

Esto generará el ejecutable `tarea3`.

## Ejecución

Para iniciar el juego, ejecuta:

```bash
./tarea3
```

## Archivos

- `main.c`: Código fuente principal.
- `laberinto.csv`: Archivo CSV con la descripción del laberinto (salas, objetos, conexiones).
- `README.md`: Este archivo con instrucciones.

## Reglas del juego

- Comienzas en una sala inicial.
- Puedes moverte en las direcciones norte, sur, este u oeste, si hay conexión.
- Recoge objetos para aumentar tu puntaje.
- El peso de los objetos reduce tu tiempo restante por sala.
- El juego termina al llegar a una sala final o al agotarse el tiempo.

## CSV de entrada (`laberinto.csv`)

El archivo debe tener el siguiente formato por fila:

```
ID;Descripción;Objeto(s);Peso(s);Valor(es);Norte;Sur;Este;Oeste;EsFinal
```

Ejemplo:

```
2,Libreria,"Montones de libros polvorientos y estanterias torcidas. Algunos libros parecen gritar '¡lee!', otros solo quieren aplastarte si se caen.","Libro antiguo,6,2",-1,6,-1,3,No
```

## Autor

- Nombre: Ignacio Calderon
