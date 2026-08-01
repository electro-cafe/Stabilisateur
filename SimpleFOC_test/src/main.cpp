/**
 *
 * Position/angle motion control example
 * Steps:
 * 1) Configure the motor and sensor (encoder)
 * 2) Run the code
 * 3) Set the target angle (in radians) from serial terminal
 *
 *
 * NOTE :
 * > Arduino UNO example code for running velocity motion control using an
 * encoder with index significantly > Since Arduino UNO doesn't have enough
 * interrupt pins we have to use software interrupt library PciManager.
 *
 * > If running this code with Nucleo or Bluepill or any other board which has
 * more than 2 interrupt pins > you can supply doIndex directly to the
 * encoder.enableInterrupts(doA,doB,doIndex) and avoid using PciManger
 *
 * > If you don't want to use index pin initialize the encoder class without
 * index pin number: > For example: > - Encoder encoder = Encoder(2, 3, 8192);
 * > and initialize interrupts like this:
 * > - encoder.enableInterrupts(doA,doB)
 *
 * Check the docs.simplefoc.com for more info about the possible encoder
 * configuration.
 *
 */
#include <Arduino.h>
#include <SimpleFOC.h>

// quand on parle d'interuption on fait référence à une pin qui, lorsqu'elle
// change d'état, va interrompre le programme en cours pour exécuter une
// fonction spécifique. C'est un gain de ressource comparer à demander au
// processeur de vérifier l'état des pins toutes les x secondes. Dans ce cas
// précis, on utilise les pins A et B de l'encodeur pour générer des
// interruptions qui vont permettre de suivre la position du moteur en temps
// réel. L'encodeur envoie des signaux électriques qui changent d'état à chaque
// mouvement du moteur, et ces changements déclenchent les interruptions.

// création d'un objet moteur à partir de la classe BLDCMotor. L'argument
// représente le nombre de paires de pôles du rotor. comme on est sur du 12N14P
// pour 12 aimants et 14 bobines on a 7 paires de pôles. ça fait 7 paires
BLDCMotor motor = BLDCMotor(7);
// le 4ème argument du driver représente la pin enable. C'est elle qui allume /
// éteind le driver, donc le moteur. Elle ne contrôle pas le PWM, ça c'est les 3
// premières pins. le moteurs fonctionne via l'alimentation des coils triphasé
// en "alternance". Le driver ne peut pas envoyer une tension de manière
// analogique, c'est tout où rien. mais les coils via l'inductance tendent à
// conserver un moment ce qu'ils reçoivent, avec ça on arrive à créer des
// courbes sinusoidales quand on joue sur la fréquence du pwm. En contrôlant la
// fréquence du pwm on peut diminuer le pas de la courbe sinusoidale et donc
// accélérer la rotation du champ magnétique. Le PWM contrôle la vitesse et la
// tension du moteur

BLDCDriver3PWM driver = BLDCDriver3PWM(9, 5, 6, 8);
// Stepper motor & driver instance
// StepperMotor motor = StepperMotor(50);
// StepperDriver4PWM driver = StepperDriver4PWM(9, 5, 10, 6,  8);

// senseur (encoder) instance
// arguments: chip select pin= pin dédiée au sensor, bit resolution, angle
// register= adresse de stockage de l'angle
MagneticSensorSPI sensor = MagneticSensorSPI(5, 14, 0x3FFF);

// angle set point variable
float target_angle = 0;
// instantiate the commander
Commander command = Commander(Serial);
void doTarget(char *cmd) { command.scalar(&target_angle, cmd); }
// & permet de passer l'adresse de target_angle à la fonction scalar, pas une
// copie de la valeur de cette variable. Ainsi, la fonction scalar peut modifier
// directement la valeur de target_angle dans le programme principal.

void setup() {

  // initialize encoder sensor hardware
  // eventuellement ajouter SPI.begin() et #include <SPI.h> si le code ne
  // compile pas
  sensor.init();
  motor.linkSensor(&sensor);

  // driver config
  // power supply voltage [V]
  driver.voltage_power_supply = 20;
  driver.init();
  // link the motor and the driver
  motor.linkDriver(&driver);

  // aligning voltage [V]
  // Phase de calibrage, on donne un petit voltage pour éviter que le moteur
  // surchauffe. Le but est qu'il ai assez de couple pour tourner et s'aligner
  // sur le champ magnétique fixe.
  motor.voltage_sensor_align = 3;

  // set motion control loop to be used
  motor.controller = MotionControlType::angle;

  // contoller configuration
  // default parameters in defaults.h

  // velocity PI controller parameters
  motor.PID_velocity.P = 0.2f;
  motor.PID_velocity.I = 2;
  motor.PID_velocity.D = 0;
  // default voltage_power_supply
  motor.voltage_limit = 15; // afin de pas dépasser les 1 ampères. en prenant 15
                            // ohm comme resistance du moteur.
  // jerk control using voltage voltage ramp
  // default value is 300 volts per sec  ~ 0.3V per millisecond
  motor.PID_velocity.output_ramp = 1000;

  // velocity low pass filtering time constant
  motor.LPF_velocity.Tf = 0.01f;

  // angle P controller
  // P du système PID (eh oui il y en a 2) défini à quel point le moteur corrige
  // sa position lue via l'encoder comparée à sa position target. Plus la valeur
  // est grande, plus le moteur va corriger rapidement sa position, mais plus il
  // risque de vibrer et de surchauffer. Il faut trouver un compromis entre
  // rapidité et stabilité.
  motor.P_angle.P = 2; // a augmenter suivant la charge a déplacer.
  //  maximal velocity of the position control
  motor.velocity_limit = 4;

  // use monitoring with serial
  Serial.begin(115200);
  // comment out if not needed
  motor.useMonitoring(Serial);

  // initialize motor

  motor.init();
  // align encoder and start FOC
  motor.initFOC();

  // add target command T
  command.add('T', doTarget, "target angle");

  Serial.println(F("Motor ready."));
  Serial.println(F("Set the target angle using serial terminal:"));
  _delay(1000);
}

void loop() {
  // main FOC algorithm function
  // the faster you run this function the better
  // Arduino UNO loop  ~1kHz
  // Bluepill loop ~10kHz
  motor.loopFOC();

  // Motion control function
  // velocity, position or voltage (defined in motor.controller)
  // this function can be run at much lower frequency than loopFOC() function
  // You can also use motor.move() and set the motor.target in the code
  motor.move(target_angle);

  // function intended to be used with serial plotter to monitor motor variables
  // significantly slowing the execution down!!!!
  // motor.monitor();

  // user communication
  command.run();
}

//-- -- -- -- -- -- -- -- --

// tester encoder avec ce code:
// https://docs.simplefoc.com/test_sensor.

// encoder PWM. a combiner avec angle control.lino
// https://docs.simplefoc.com/test_sensor

//----------acces aux variables du moteur-------------------
// motor.shaft_angle; // motor angle
// motor.shaft_velocity; // motor velocity
//------------- acces aux variables du sensor---------------
// sensor.getAngle(); // motor angle
// sensor.getVelocity(); // motor velocity
//----------acces variable moteur SPI----------------------
// class MagneticSensorSPI{
// public:
//    // shaft velocity getter
//   float getVelocity();
// 	// shaft angle getter
//   float getAngle();
//}
