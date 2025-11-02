# ILLIUM ENGINE
https://github.com/bekun67/motor

Illium Engine és un motor de videojocs creat per a l'assignatura de Motors.
De moment es poden dur a terme les funcions bàsiques d'una engine, com carregar models i textures.  

Al iniciar l'engine es carreguen automàticament 3 models: una casa, un canó sense textura (checkerboard) i un canó amb textura (lenna)  
Si s'arrossega una textura sense posar-la assobre de cap model s'aplicarà al model més proper en un mínim de distància.  
Si hi ha cap error amb la textura o no troba l'arxiu posarà el checkerboard.  
Si el nom d'arxiu del model o la textura té un nom amb caràcters especials l'engine no el podrà carregar.  

Com que no em sabut aplicar el imgui per a fer l'UI em fet que per a seleccionar un game object s'hagi de clicar un número entre l'1 i el 9.

## INTEGRANTS
- Isaac Ramírez Prieto: https://github.com/bekun67
- Martí Mach Aymerich: https://github.com/0psycada
- Xavier Chaparro Foyo: https://github.com/xavifast05
- Clara Rodríguez Moreno: https://github.com/kopeke4

## CONTROLS
- Càmera:
  - Clic dret: Girar la càmera
  - W A S D + Clic dret: Navegar el model
  - F: La càmera se centra en la geometria
  - SHIFT: Duplica la velocitat de moviment
- Carregar models o textures: Arrossegar els arxius al programa (Drag and drop)
- Model:
  - Número de l'1 - 9: Seleccionar Game Object
  - X: Engrandir
  - Z: Empetitir
  - I J K L: Moure l'objecte en l'eix X i Z
  - U O: Moure l'objecte en l'eix Y
  - V B N M: Rotar l'objecte
