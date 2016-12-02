#include <Servo.h>


// Variables con valor inicial importante

int servo_points[3] = {10, 85, 170}; //ángulos de calibración del servo
int potenciometro_0 = A4; //pin del potenciometro
int servo_pin = 3; //pin digital del servo
float resolution = 20; // Resolución, en ms (tiempo del bucle)
float vel_max = 2;
float acc = 0.005;
float correct_servo = 25; //corrección del servo

//Variables de valor no importante

int val = 0; //Variable de entrada del Serial
int mode = 0; //mode 0: escuchar, mode 1: mover
bool expect_number = false;
String command = "";
int counter = 0;
String cadena = "" ;
String medida_pot = "";
float medida = 0;
float calibration[4] = {0,0,0,0};// mínimo, medio1, medio2, máximo
bool direct = true;
float scale_0 = 0;
float scale_1 = 0;
float servo_pos = 0;
float target = servo_points[1];
float dist = 0;
float vel = 0;
float topspeed = 0;
Servo servo; //Creamos un objeto Servo de nombre... servo

// FUNCIONES: -------

float scale (float medida, bool direct){
  float medium = 0;
  if (direct){medium = calibration[2];}
  else{medium = calibration[1];}
  
  scale_0 = (servo_points[1] - servo_points[0]) / (medium - calibration[0]);
  scale_1 = (servo_points[2] - servo_points[1]) / (calibration[3] - medium);
  
  if (medida > medium){
    return (servo_points[1] + (medida - medium)*scale_1);
  }
  else{
    return (servo_points[0] + (medida - calibration[0])*scale_0);
  }
}

float set_speed(float medida, float target, float vel, float vel_max, float acc, float topspeed){    
  float sign = 0;
  float target_speed = 0;
  float dist = 0;
  float max_dist = 0;
  float delta_v = 0;
  
  max_dist =  0.51 * topspeed * topspeed / acc;
  dist = target - medida;
  if (abs(dist) > 0.001){sign = (dist)/(abs(dist));}
  else{sign = 1;}
  
  if (abs(dist) > max_dist){target_speed = vel_max * sign;}
  else{target_speed = dist * topspeed / max_dist;}
  
  if (abs(target-medida)< 0.2){target_speed = 0;}
  
  delta_v = target_speed - vel ; 
  
  if (delta_v == 0){return vel;}
  
  sign = delta_v / (abs(delta_v));
  if (abs(delta_v) > acc){
    vel = vel + acc * sign;
    delta_v = acc * sign;
  }
  else{vel = vel + delta_v;}
  
  return vel;
} 
void calibrate()
{
   servo.write(correct_servo +servo_points[0]);
   delay(2000);
   calibration[0] = analogRead(potenciometro_0);
   servo.write(correct_servo +servo_points[1]);
   delay(2000);
   calibration[1] = analogRead(potenciometro_0);
   servo.write(correct_servo +servo_points[2]);
   delay(2000);
   calibration[3] = analogRead(potenciometro_0);   
   servo.write(correct_servo +servo_points[1]);
   delay(2000);
   calibration[2] = analogRead(potenciometro_0);
   servo_pos = servo_points[1];
}   

// INICIANDO ------------------------------------------------------------------------------------------------- INICIANDO 
void setup()
{
   //Ajustar valores respecto  a la resolución:
   vel_max = vel_max * resolution / 10;
   acc = acc * resolution / 10;
   servo.attach(servo_pin); //Conectamos el servo al pin digital
   mode = 0;

   Serial.begin(9600); //Iniciamos el serial
   Serial.println("READY");
}
//  BUCLE PRINCIPAL ----------------------------------------------------------------------------------------- BUCLE PRINCIPAL 
void loop()
{
  medida = analogRead(potenciometro_0);
  medida = scale( medida, direct);
  if (mode == 0) //-------------------------------------------------------------------------------------------mode escucha
  {
    if(Serial.available() > 0) //Detecta si hay alguna entrada por serial
    {
      command = Serial.readString();
      if (command == "calibrate"){
        calibrate();
        cadena = String("Calibrado: ");
        int i;
        for (i = 0; i < 4; i = i + 1) {
          cadena = cadena + calibration[i];
          cadena = cadena + " ";
         }
        Serial.println(cadena);
      }
      else if (command == "move"){
        Serial.println("Listo para recibir");
        while (Serial.available()==0) {} //Wait for user input
        val = Serial.parseInt();
        if (val > servo_points[2]){val = servo_points[2];}
        if (val < servo_points[0]){val = servo_points[0];}
        if (val > servo_pos){direct = false;}
        else if (val < servo_pos){direct = true;} //Mantiene el sentido si val == pos
        mode = 1;
        target = val;
        topspeed = 0;
        cadena = String("moviendo a ");
        cadena = cadena + target;
        cadena = cadena + " posicion: ";
        cadena = cadena + medida;
        cadena = cadena + " velocidad: ";
        cadena = cadena + "0.00";
        Serial.println(cadena);
      }
      else if (command == "status"){
        medida_pot = String("Modo escucha. Medida: ");
        medida_pot = medida_pot + medida;
        Serial.println(medida_pot);
      }
      else if (command == "force"){        
        Serial.println("Listo para recibir");
        int i;
        for (i = 0; i < 4; i = i + 1) {
          while (Serial.available()==0) {} //Wait for user input
          val = Serial.parseInt();
          calibration[i] = val;
        }
        cadena = String("Calibrado: ");
        for (i = 0; i < 4; i = i + 1) {
          cadena = cadena + calibration[i];
          cadena = cadena + " ";
         }
        Serial.println(cadena);
      }
    }//fin if serial available
  }//fin if mode escucha
  else if (mode == 1)// -------------------------------------------------------------------------------- mode movimiento 
  {
    vel =  set_speed(medida, target, vel, vel_max, acc, topspeed);
    if (abs(vel)>topspeed){topspeed = abs(vel);}
    servo_pos = servo_pos + vel;
    servo.write(correct_servo +servo_pos);
    dist = target - medida;
    dist = abs(dist);
    if ((abs(vel)< 0.01) and (dist < 0.2)){
      mode = 0;
      Serial.println("Alcanzado");
    }
    else{
      counter = counter + 1;
      if (counter == 5)
      {
        cadena = String("moviendo a ");
        cadena = cadena + target;
        cadena = cadena + " posicion: ";
        cadena = cadena + medida;
        cadena = cadena + " velocidad: ";
        cadena = cadena + vel;
        Serial.println(cadena);
        counter = 0;
      }
    }
  }
  // ---------------------------------------------------------------------------Tiempos de espera para el siguiente loop
  
  if (mode == 0)
  {
    //Serial.println("mode escucha");
    delay(50);
  }
  else if (mode == 1)
  {
    //Serial.println("mode movimiento");
    delay(resolution);
  }
  else
  {
    Serial.println("Atención mode no reconocido");
    delay(300);
  }
}

