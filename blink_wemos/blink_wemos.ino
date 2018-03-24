/*
 * Simple sketch to test Wemos and NodeMCU
 */

void setup() {
 
  pinMode(D0, OUTPUT); //Declare Pin mode
 
}
 
void loop() {
 
  digitalWrite(D0, HIGH);   //Turn the LED on
  delay(1000);              //Wait 1 second
  digitalWrite(D0, LOW);    //Turn the LED off
  delay(1000);              //Wait 1 second
 
}
