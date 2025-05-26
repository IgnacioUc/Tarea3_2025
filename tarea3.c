#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <locale.h>

// Constantes
#define MAX_LARGO_NOMBRE 40
#define MAX_LARGO_DESCRIPCION 200
#define MAX_LARGO_LECTURA 1024
#define MAX_CAMPOS 9
#define MAX_OBJETOS 4
#define MAX_INVENTARIO 10
#define TIEMPO_INICIAL 30
#define ARCHIVO_CSV "graphquest.csv"

// Enumeración para direcciones
enum Direcciones {
    SALIR = 0,
    ARRIBA = 1,
    ABAJO = 2,
    IZQUIERDA = 3,
    DERECHA = 4
};

// Definición de estructuras en el orden correcto

// Primero la estructura Objeto (base para las demás)
typedef struct {
    char nombre[MAX_LARGO_NOMBRE];
    int peso;
    int valor;
} Objeto;

// Luego Inventario que usa Objeto
typedef struct {
    Objeto objetos[MAX_INVENTARIO];
    int cantidad;
    int tiempo_restante;
} Inventario;

// Finalmente Escenario que también usa Objeto
typedef struct {
    int id;
    char nombre[MAX_LARGO_NOMBRE];
    char descripcion[MAX_LARGO_DESCRIPCION];
    int arriba;
    int abajo;
    int izquierda;
    int derecha;
    Objeto objetos[MAX_OBJETOS];
    bool es_final;
} Escenario;

// Prototipos de funciones
bool parsear_booleano(const char* str);
void parsear_objetos(const char* str, Escenario* escena);
int cargar_escenarios(const char* archivo_csv, Escenario** escenarios, int* num_escenarios);
Escenario* buscar_escenario_por_id(Escenario* escenarios, int num_escenarios, int id);
void mostrar_escenario_actual(const Escenario* escena);
void limpiar_buffer_entrada();
void liberar_escenarios(Escenario* escenarios);
int parse_csv_line(char* line, char** fields, int max_fields);
void eliminar_comillas(char* str);
void mostrar_inventario(const Inventario* inv);
int recoger_objetos(Escenario* escena, Inventario* inv);

// Implementación de funciones

void eliminar_comillas(char* str) {
    if (str == NULL || str[0] == '\0') return;

    size_t len = strlen(str);
    if (str[0] == '"' && str[len-1] == '"') {
        memmove(str, str+1, len-2);
        str[len-2] = '\0';
    }
}

bool parsear_booleano(const char* str) {
    if (str == NULL || str[0] == '\0') return false;
    return (tolower(str[0]) == 't' || str[0] == '1' || tolower(str[0]) == 's' ||
           strcasecmp(str, "si") == 0 || strcasecmp(str, "yes") == 0);
}

void parsear_objetos(const char* str, Escenario* escena) {
    if (str == NULL || str[0] == '\0') return;

    char copia[MAX_LARGO_LECTURA];
    strncpy(copia, str, MAX_LARGO_LECTURA);
    copia[MAX_LARGO_LECTURA - 1] = '\0';
    eliminar_comillas(copia);

    char* token = strtok(copia, ";");
    int i = 0;

    while (token != NULL && i < MAX_OBJETOS) {
        while (*token == ' ') token++;

        char nombre[MAX_LARGO_NOMBRE] = {0};
        int valor = 0, peso = 0;

        if (sscanf(token, "%[^,],%d,%d", nombre, &valor, &peso) == 3) {
            size_t len = strlen(nombre);
            while (len > 0 && isspace(nombre[len - 1])) {
                nombre[--len] = '\0';
            }

            strncpy(escena->objetos[i].nombre, nombre, MAX_LARGO_NOMBRE - 1);
            escena->objetos[i].valor = valor;
            escena->objetos[i].peso = peso;
            i++;
        }
        token = strtok(NULL, ";");
    }
}

int parse_csv_line(char* line, char** fields, int max_fields) {
    int field_count = 0;
    char* start = line;
    int in_quotes = 0;

    for (char* p = line; *p && field_count < max_fields; p++) {
        if (*p == '"' && (p == line || *(p-1) != '\\')) {
            in_quotes = !in_quotes;
        } else if (*p == ',' && !in_quotes) {
            *p = '\0';
            fields[field_count++] = start;
            start = p + 1;
        }
    }

    if (field_count < max_fields) {
        fields[field_count++] = start;
    }

    return field_count;
}

int cargar_escenarios(const char* archivo_csv, Escenario** escenarios, int* num_escenarios) {
    FILE* archivo = fopen(archivo_csv, "r");
    if (archivo == NULL) {
        fprintf(stderr, "Error: No se pudo abrir el archivo %s\n", archivo_csv);
        perror("Detalle");
        return -1;
    }

    char linea[MAX_LARGO_LECTURA];
    *num_escenarios = 0;

    // Saltar la línea de encabezado
    if (fgets(linea, MAX_LARGO_LECTURA, archivo) == NULL) {
        fclose(archivo);
        fprintf(stderr, "Error: Archivo vacío o sin encabezado\n");
        return -1;
    }

    // Primera pasada: contar líneas válidas
    while (fgets(linea, MAX_LARGO_LECTURA, archivo) != NULL) {
        char* fields[MAX_CAMPOS] = {0};
        if (parse_csv_line(linea, fields, MAX_CAMPOS) >= MAX_CAMPOS) {
            (*num_escenarios)++;
        }
    }

    if (*num_escenarios <= 0) {
        fclose(archivo);
        fprintf(stderr, "Error: El archivo no contiene líneas válidas con %d campos\n", MAX_CAMPOS);
        return -1;
    }

    *escenarios = (Escenario*)calloc(*num_escenarios, sizeof(Escenario));
    if (*escenarios == NULL) {
        fclose(archivo);
        perror("Error al asignar memoria para escenarios");
        return -1;
    }

    rewind(archivo);
    fgets(linea, MAX_LARGO_LECTURA, archivo); // Saltar encabezado nuevamente

    int indice_escenario = 0;
    bool tiene_id_1 = false;

    while (fgets(linea, MAX_LARGO_LECTURA, archivo) != NULL && indice_escenario < *num_escenarios) {
        linea[strcspn(linea, "\n")] = '\0';

        char* fields[MAX_CAMPOS] = {0};
        int num_fields = parse_csv_line(linea, fields, MAX_CAMPOS);

        if (num_fields >= MAX_CAMPOS) {
            (*escenarios)[indice_escenario].id = atoi(fields[0]);
            if ((*escenarios)[indice_escenario].id == 1) tiene_id_1 = true;

            // Procesar nombre
            strncpy((*escenarios)[indice_escenario].nombre, fields[1], MAX_LARGO_NOMBRE - 1);
            eliminar_comillas((*escenarios)[indice_escenario].nombre);

            // Procesar descripción
            strncpy((*escenarios)[indice_escenario].descripcion, fields[2], MAX_LARGO_DESCRIPCION - 1);
            eliminar_comillas((*escenarios)[indice_escenario].descripcion);

            // Inicializar objetos
            for (int k = 0; k < MAX_OBJETOS; k++) {
                (*escenarios)[indice_escenario].objetos[k].nombre[0] = '\0';
                (*escenarios)[indice_escenario].objetos[k].peso = 0;
                (*escenarios)[indice_escenario].objetos[k].valor = 0;
            }

            // Procesar objetos
            parsear_objetos(fields[3], &(*escenarios)[indice_escenario]);

            // Procesar direcciones
            (*escenarios)[indice_escenario].arriba = atoi(fields[4]);
            (*escenarios)[indice_escenario].abajo = atoi(fields[5]);
            (*escenarios)[indice_escenario].izquierda = atoi(fields[6]);
            (*escenarios)[indice_escenario].derecha = atoi(fields[7]);

            // Procesar es_final
            (*escenarios)[indice_escenario].es_final = parsear_booleano(fields[8]);

            // Asegurar terminación nula
            (*escenarios)[indice_escenario].nombre[MAX_LARGO_NOMBRE - 1] = '\0';
            (*escenarios)[indice_escenario].descripcion[MAX_LARGO_DESCRIPCION - 1] = '\0';

            indice_escenario++;
        }
    }

    fclose(archivo);

    if (!tiene_id_1) {
        fprintf(stderr, "Error: No se encontró un escenario con ID 1 en el archivo\n");
        free(*escenarios);
        *escenarios = NULL;
        return -1;
    }

    return 0;
}

Escenario* buscar_escenario_por_id(Escenario* escenarios, int num_escenarios, int id) {
    for (int i = 0; i < num_escenarios; i++) {
        if (escenarios[i].id == id) {
            return &escenarios[i];
        }
    }
    return NULL;
}

void mostrar_escenario_actual(const Escenario* escena) {
    printf("\n=== %s ===\n", escena->nombre);
    printf("%s\n", escena->descripcion);

    // Mostrar objetos disponibles
    printf("\nObjetos disponibles:\n");
    bool hay_objetos = false;
    for (int i = 0; i < MAX_OBJETOS; i++) {
        if (escena->objetos[i].nombre[0] != '\0') {
            printf("- %s (Valor: %d, Peso: %d)\n",
                   escena->objetos[i].nombre,
                   escena->objetos[i].valor,
                   escena->objetos[i].peso);
            hay_objetos = true;
        }
    }
    if (!hay_objetos) {
        printf("No hay objetos en este escenario.\n");
    }

}


void limpiar_buffer_entrada() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void liberar_escenarios(Escenario* escenarios) {
    if (escenarios != NULL) {
        free(escenarios);
    }
}

void mostrar_inventario(const Inventario* inv) {
    printf("\n=== INVENTARIO (Tiempo: %d) ===\n", inv->tiempo_restante);
    if (inv->cantidad == 0) {
        printf("Vacío\n");
        return;
    }
    
    for (int i = 0; i < inv->cantidad; i++) {
        printf("- %s (Valor: %d, Peso: %d)\n",
               inv->objetos[i].nombre,
               inv->objetos[i].valor,
               inv->objetos[i].peso);
    }
}

int recoger_objetos(Escenario* escena, Inventario* inv) {
    if (inv->cantidad >= MAX_INVENTARIO) {
        printf("¡Inventario lleno! No puedes llevar más objetos.\n");
        return 0;
    }

    printf("\nObjetos disponibles para recoger:\n");
    int disponibles = 0;
    int indices[MAX_OBJETOS] = {0};
    
    for (int i = 0; i < MAX_OBJETOS; i++) {
        if (escena->objetos[i].nombre[0] != '\0') {
            printf("%d. %s (Valor: %d, Peso: %d)\n",
                   i+1,
                   escena->objetos[i].nombre,
                   escena->objetos[i].valor,
                   escena->objetos[i].peso);
            indices[disponibles++] = i;
        }
    }

    if (disponibles == 0) {
        printf("No hay objetos para recoger aquí.\n");
        return 0;
    }

    printf("Elige el número del objeto a recoger (0 para cancelar): ");
    int eleccion;
    if (scanf("%d", &eleccion) != 1 || eleccion < 0 || eleccion > disponibles) {
        printf("Selección inválida.\n");
        limpiar_buffer_entrada();
        return 0;
    }

    if (eleccion == 0) {
        return 0;
    }

    int indice_real = indices[eleccion-1];
    inv->objetos[inv->cantidad] = escena->objetos[indice_real];
    inv->cantidad++;
    printf("¡Has recogido: %s!\n", escena->objetos[indice_real].nombre);
    
    // Eliminar el objeto del escenario
    escena->objetos[indice_real].nombre[0] = '\0';
    
    return 1; // Se descuenta 1 de tiempo
}

int main(void) {
    setlocale(LC_ALL, "");

    Escenario* escenarios = NULL;
    int num_escenarios = 0;
    Inventario inventario = {0};
    inventario.tiempo_restante = TIEMPO_INICIAL;

    printf("Cargando escenarios desde: %s\n", ARCHIVO_CSV);

    if (cargar_escenarios(ARCHIVO_CSV, &escenarios, &num_escenarios) != 0) {
        fprintf(stderr, "Error al cargar escenarios. Verifique el archivo CSV.\n");
        return EXIT_FAILURE;
    }

    Escenario* escenario_actual = buscar_escenario_por_id(escenarios, num_escenarios, 1);
    if (escenario_actual == NULL) {
        fprintf(stderr, "Error crítico: Escenario inicial no encontrado después de carga exitosa\n");
        liberar_escenarios(escenarios);
        return EXIT_FAILURE;
    }

    bool ejecutando = true;
    while (ejecutando && inventario.tiempo_restante > 0) {
        printf("\n=== Tiempo restante: %d ===\n", inventario.tiempo_restante);
        mostrar_escenario_actual(escenario_actual);

        printf("\nOpciones:\n");
        printf("1-4. Moverse (Arriba/Abajo/Izquierda/Derecha)\n");
        printf("5. Examinar objetos del escenario\n");
        printf("6. Recoger objetos\n");
        printf("7. Ver inventario\n");  // Nueva opción para ver inventario
        printf("8. Usar objeto del inventario\n");
        printf("0. Salir\n");

        int opcion;
        printf("\nElige una opción: ");
        if (scanf("%d", &opcion) != 1) {
            printf("Entrada inválida. Por favor ingrese un número.\n");
            limpiar_buffer_entrada();
            continue;
        }
        limpiar_buffer_entrada();


        switch (opcion) {
            case 1: // Arriba
            case 2: // Abajo
            case 3: // Izquierda
            case 4: { // Derecha
                int destino_id = 0;
                switch (opcion) {
                    case 1: destino_id = escenario_actual->arriba; break;
                    case 2: destino_id = escenario_actual->abajo; break;
                    case 3: destino_id = escenario_actual->izquierda; break;
                    case 4: destino_id = escenario_actual->derecha; break;
                }

                if (destino_id == 0) {
                    printf("No puedes ir en esa dirección\n");
                } else {
                    Escenario* nuevo = buscar_escenario_por_id(escenarios, num_escenarios, destino_id);
                    if (nuevo == NULL) {
                        printf("Error: Escenario destino no encontrado\n");
                    } else {
                        escenario_actual = nuevo;
                        inventario.tiempo_restante--; // Movimiento consume tiempo
                        if (escenario_actual->es_final) {
                            printf("\n=== FIN DEL JUEGO ===\n");
                            printf("¡Felicidades! Has llegado a: %s\n", escenario_actual->nombre);
                            ejecutando = false;
                        }
                    }
                }
                break;
            }

            case 5: { // Examinar objetos
                printf("\nObjetos en este escenario:\n");
                bool hay_objetos = false;
                for (int i = 0; i < MAX_OBJETOS; i++) {
                    if (escenario_actual->objetos[i].nombre[0] != '\0') {
                        printf("- %s (Valor: %d, Peso: %d)\n",
                               escenario_actual->objetos[i].nombre,
                               escenario_actual->objetos[i].valor,
                               escenario_actual->objetos[i].peso);
                        hay_objetos = true;
                    }
                }
                if (!hay_objetos) {
                    printf("No hay objetos en este escenario.\n");
                }
                break;
            }

            case 6: { // Recoger objetos
                inventario.tiempo_restante -= recoger_objetos(escenario_actual, &inventario);
                break;
            }

            case 7: { // Ver inventario (nueva opción)
                mostrar_inventario(&inventario);
                break;
            }

            case 8: { // Usar objeto (antes era 7)
                if (inventario.cantidad == 0) {
                    printf("No tienes objetos en tu inventario.\n");
                    break;
                }

                printf("\nObjetos en tu inventario:\n");
                for (int i = 0; i < inventario.cantidad; i++) {
                    printf("%d. %s\n", i+1, inventario.objetos[i].nombre);
                }

                printf("Elige el objeto a usar (0 para cancelar): ");
                int eleccion;
                if (scanf("%d", &eleccion) != 1 || eleccion < 0 || eleccion > inventario.cantidad) {
                    printf("Selección inválida.\n");
                    limpiar_buffer_entrada();
                    break;
                }

                if (eleccion > 0) {
                    printf("Has usado: %s\n", inventario.objetos[eleccion-1].nombre);
                    inventario.tiempo_restante--; // Usar objeto consume tiempo
                }
                break;
            }

            case 0: // Salir
                ejecutando = false;
                break;

            default:
                printf("Opción no válida\n");
                break;
        }
    }
    if (inventario.tiempo_restante <= 0) {
        printf("\n¡Se te acabó el tiempo! Game Over.\n");
    }

    // Mostrar resumen final
    printf("\n=== RESULTADO FINAL ===\n");
    printf("Escenario final: %s\n", escenario_actual->nombre);
    printf("Objetos recolectados: %d/%d\n", inventario.cantidad, MAX_INVENTARIO);
    
    int valor_total = 0;
    for (int i = 0; i < inventario.cantidad; i++) {
        valor_total += inventario.objetos[i].valor;
    }
    printf("Valor total de tu inventario: %d\n", valor_total);

    liberar_escenarios(escenarios);
    return EXIT_SUCCESS;
}
