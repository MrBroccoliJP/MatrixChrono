/*
 * MatrixChrono - IoT Smart Home Clock System
 * Copyright (c) 2025 João Fernandes
 * 
 * This work is licensed under the Creative Commons Attribution-NonCommercial 
 * 4.0 International License. To view a copy of this license, visit:
 * http://creativecommons.org/licenses/by-nc/4.0/
 */

const char MAIN_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<meta charset="utf-8">
<body>

<h2>Broccoli Clock Página de Definições<h2>
<h3> João Fernandes 2021</h3>

<form action="/action_page">
  Nome da cidade:<br>
  <input type="text" name="CIDADE">
  <br><br>
  API KEY:<br>
  <input type="text" name="APIKEY">
  <br><br>
  SETPOSIX: [Portugal: CET-1CEST,M3.5.0/1,M10.5.0/2]<br>
  <input type="text" name="SETPOSIX">
  <br><br>
  Idioma:
  <input type="text" name="Language">
  <br><br>
  Ligar Sensor de Temperatura: 
  <input type="Checkbox" name="SensorEnable" value=1 checked="checked">
  <br>
  Ligar Mudança de Hora: 
  <input type="Checkbox" name="DSTEnable" value=1 checked="checked">
  <br>
  Ligar Botao: 
  <input type="Checkbox" name="BTNEnable" value=1 checked="checked">
  <br>
  <br>
  MUDANÇA DE HORA MANUAL:<br>
  <input type="number" name="ClockHourOverride">
  <br>
  <br>
  Luminosidade (0 a 15):<br>
  <input type="number" name="intensity">
  <br><br>
  LIGAR MODO NOTURNO?
  <input type="Checkbox" name="NightModeEnable" value=1 checked="checked">
  <br>
  Das : <input type="number" name="StartNightMode" value=0>h  Às   <input type="number" name="StopNightMode" value=0>h
  <br>
  Ligar modo noturno com ecrã desligado: 
  <input type="Checkbox" name="NightmodeScreenOff" value=0 >
  <br><br>
  Frequência de sincronização (Dias): <input type="number" name="SyncDay">
  <br><br>
  Tipo de letra:(1 - Normal, 2 - Minimalista, 3 - BOLD)  
  <input type="number" name="Font">
  <br><br>
   Reiniciar dispositivo depois das mudanças?-->
  <input type="Checkbox" name="RESET" value=1 >
  <br>
  <br><br>
  <input type="Submit" value="Submeter">
</form> 
  <br><br>
<a href='/Reboot'> Reiniciar</a><br>
  <br><br>
  <a href='/turnOnOffscreen'> Ligar/Desligar Ecrã</a>
<br><br>
<a href='/Sensor'> Leituras do Sensor </a>
<br><br><br>
<a href='/DELETEALL'> RESET PARA DADOS DE FÁBRICA </a>
)=====";

//</body>
//</html>
