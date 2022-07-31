// code for comms boxes at La Milagrosa
// Russell Bingham & Aaron Trujillo
// 7/30/22

int num_buttons = 5;
int message_play = 11; // assuming pin 13 is available -- used for message record/send/play
int message_available = 13; // assuming 14 is available
int message_receive = 15; // an output for the light when a message comes in
int input_pins[5] = {2, 4, 7, 8, 12}; // switches
int output_pins[5] = {3, 5, 6, 9, 10}; // lights (PWM)
int brightnesses[5] = {0, 0, 0, 0, 0}; // light states

byte message[100];

int max_brightness = 100; 
int current_state = 0;

int totalBytesInMessage = 0;

byte recordMessage() {
  byte output;
  for (int i = 0; i < num_buttons; i++) {
    output |= digitalRead(input_pins[i]) << i;
  }
  delay(100);
  return output;
}

void playMessage(byte lightSequence) {
  int brightness;
  for (int i = 0; i < num_buttons; i++) {
    brightness = lightSequence >> i ? brightnesses[i] : 0;
    analogWrite(output_pins[i], brightness);
  }
  delay(100);
}

void setup() {
  Serial.begin(9600);

  for (int i = 0; i < num_buttons; i++) {
    pinMode(input_pins[i], INPUT);
    pinMode(output_pins[i], OUTPUT);
  }
  pinMode(message_play, INPUT);
  pinMode(message_available, OUTPUT);
}

void loop() {
  switch (current_state) {
    case 0: // base state
      // check for incoming message
      if (Serial.available() > 0) {
        totalBytesInMessage = Serial.read();
        // turn on light, indicating message has been received
        digitalWrite(message_available, HIGH);
        current_state = 3;
        break;
      }
      // check to record message
      if (digitalRead(message_play)) {
        current_state = 1;
      }
      break;
    case 1: // record and send message
      for (int i = 0; i < 100; ++i) {
        totalBytesInMessage = i;
        // check for message completion
        if (digitalRead(message_play)) {
          current_state = 2;
          break;
        }
        message[i] = recordMessage();
      }
      Serial.write(totalBytesInMessage);
      current_state = 2;
      break;
    case 2: // message delivery
      Serial.write(0xFF);
      Serial.write(message, totalBytesInMessage);
      // only one message can be sent at a time. To avoid multiple sends,
      // sender waits until receiver plays the sent message
      if (Serial.available() > 0) {
        current_state = 0;
      }
      break;
    case 3: // message receieved
      // stay in receive state until receiver plays message
      if (digitalRead(message_play)) {
        // turn off light once receiver acknowledges message
        digitalWrite(message_available, LOW);
        current_state = 4;
      }
      break;
    case 4: // play message
      while(Serial.read() != 0xFF){
        continue;
      }
      for (int i = 0; i < totalBytesInMessage; i++) {
        playMessage(Serial.read());
      }
      break;
  }
}
