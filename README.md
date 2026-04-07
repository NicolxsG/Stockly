# SMIC_2610_Guzman_Torres
En este repositorio encontrará todo lo relacionado al proyecto de SMIC el cual se llamará STOCKLY realizado por los estudiantes Jerson Nicolás Guzmán Muñoz Y Giovanny Alejandro Torres Villarraga.

El nombre Stockly surge de la palabra ‘stock’, que representa el inventario, combinada con el sufijo ‘-ly’, común en plataformas tecnológicas. El nombre refleja un sistema moderno orientado a la gestión inteligente de inventario en tiempo real.

 # Descripción del proyecto:
En supermercados y tiendas de autoservicio, la disponibilidad de productos en exhibición influye directamente en la experiencia del cliente y en la posibilidad de concretar ventas. Cuando una góndola presenta niveles bajos de inventario y no se detecta a tiempo, se generan pérdidas por desabastecimiento, retrasos en reposición y baja eficiencia operativa. 

Con el fin de aportar una solución, se propone el desarrollo de una góndola inteligente basada en ESP32. El sistema adquiere información mediante una celda de carga y un sensor infrarrojo, procesa los datos localmente y los envía por WiFi a una interfaz de monitoreo. De esta manera, el microcontrolador no solo mide variables, sino que integra sensado, procesamiento digital, toma de decisiones y comunicación IoT.

# Diagrama 
Se anexa imagen del diagrama de bloques inicial para el desarrollo del proyecto


![Diagrama del sistema](docs/Diagrama_de_Bloques_STOCKLY.png)

El objetivo de implementar esto es: 
- Integrar una celda de carga con módulo HX711 para medir el peso total de los productos.
- Implementar un sensor infrarrojo para detectar eventos de interacción o retiro de productos.
- Programar el ESP32 para adquirir, procesar y transmitir datos por WiFi.
- Estimar la cantidad de productos disponibles a partir del peso total medido.
- Generar alertas de stock bajo y prioridad de reposición.
- Visualizar variables y KPIs en un dashboard remoto

