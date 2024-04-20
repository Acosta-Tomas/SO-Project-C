# tp-scaffold

Esta es una plantilla de proyecto diseñada para generar un TP de Sistemas
Operativos de la UTN FRBA.

## Primer Checkpoint

La idea que tuvimos para este primer checkpoint fue hacer un flujo de conexiones sincrónicas donde el orden para inciar los procesos es:

1. Se inicia la **memoria** con el servidor listo para aceptar clientes.
2. Se inicia el **CPU** con el servidor listo para aceptar clientes. 
3. Se inicia el **Kernel** con el servidor listo para aceptar clientes.
4. Se incia el **I/O**.

### I/O
--- 
Ni bien inicia el modulo I/O este se conecta con el Kernel y se queda esperando a ingresar por teclado mensajes como en el TP0. Una vez mandamo el mensaje se finaliza el proceso y pasamos a kernel

### Kernel
---
El kernel ya tiene la conexión con I/O pero estaba en espera a recibir mensajes, una vez recibido los mensajes y finalizada la conexion por parte del cliente. Se conecta con la CPU y le pasa dichos mensajes. Una vez enviados vuelve a estar listo para aceptar nuevos clientes.

> *Importante es que esuvimos probando threads, a si que se vera que existe el server y clien thread. El client thread se incializa y termina rápdio, mientras que le server thread es el que las conexiones.*

### CPU
---
El kernel se conecta para enviar los mensajes que recibio de I/O y hace exactamente lo mismo que el modulo anterior. Solo forwardea los mensajes a la memoria.

#### Memoria
---
Finalmente este módulo recibe la información atraves del CPU y una vez que finaliza la conexion el cliente queda listo para escuchar nuevos clientes.