# RxSPA
Código receptor de tramas provenientes del módulo LoRa transmisor.
Este código se ejecuta en un microcontrolador ESP8266.

# Objetivos
* Recibir las tramas hexadecimales y convertirlas a ASCII
* Enviar las tramas ASCII a un servidor web

# Potencial funcionamiento
* Al inicio se configura el módulo LoRA como Rx (también Tx en caso de que se use polling)
* Se realiza polling o simplemente se reciben las tramas
* Se convierten las tramas hexadecimales a ASCII
* El receptor se conecta a una red WiFi y envía las tramas a un servidor web