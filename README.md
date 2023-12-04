# RxSPA
Código receptor de tramas provenientes del módulo LoRa transmisor.
Este código se ejecuta en un microcontrolador ESP32.

# Objetivos
- [x] Recibir las tramas hexadecimales y convertirlas a ASCII
- [x] Enviar las tramas ASCII a un servidor web

# Funcionamiento
* Se conecta a una red WiFi utilizando
* Se configura el módulo LoRA como Rx
* Se envía un comando AT para pedir datos al modulo conectado a los sensores (polling)
* Recibe la respuesta y enriquece las tramas recibidas para enviarlas al seridor web en formato json

> [!IMPORTANT]
> Comprobar que los módulos se encuentran en un rango de alcance correcto y el usuario seleccionado tenga los permisos necesarios para realizar inserts y selects en la base de datos.

> [!NOTE]
> La conexión a la red wifi debe realizarse mediante un smartphone u otro dispositivo smart capaz de conectarse al ESP e indicarle la red wifi a utilizar.
> Además, para agregar las credenciales debe crearse un archivo de nombre `secrets.h` en la misma ruta que `main.cpp`. El mismo debe tener el siguiente formato:
>  ```h
> #define DB_USER "nombre_de_usuario"
> #define DB_PASS "contraseña"
> ```

> [!WARNING]
> Por defecto se encuentra configurado para enviar consultas de datos cada 1 hora, excepto a las 00:00 donde además envía la orden de realizar riego.

> [!TIP]
> Es recomendable modificar la frecuencia de envío de peticiones a un tiempo menor a una hora en el momento en que se estén instalando los equipos a fin de comprobar la conexión entre los módulos.
> Esto se realiza modificando en [main](./src/main.cpp)
> ```cpp
>   // Execute irrAlarm once a day at 00 hs
>  if (hour.equals(IRR_HOUR) && !sendedDay.equals(day)) {
>    sendedDay = day;
>    sendedHour = hour;
>    irrAlarm();
>  }
>  
>  // Execute pollAlarm every one hour
>  if (minutes.equals(POLL_MINUTES) && !hour.equals(sendedHour)) {
>    sendedHour = hour;
>    pollAlarm();
>  }
>  ```

