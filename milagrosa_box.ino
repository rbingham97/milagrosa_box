// code for comms boxes at La Milagrosa
// Russell Bingham & Aaron Trujillo
// 7/30/22

int num_buttons = 5;
int message_play = 11; // assuming pin 11 is available -- used for message record/send/play
int message_available = 13; // assuming 13 is available -- lights up when a message is ready to be played
int input_pins[5] = {2, 4, 7, 8, 12}; // switches
int output_pins[5] = {3, 5, 6, 9, 10}; // lights (PWM)
int brightnesses[5] = {0, 0, 0, 0, 0}; // light states -- we will need to update this to hold each colors' brightness
bool flash = true;

// a message will be a sequence of 100 bytes where each byte
// corresponds to a 100ms
int max_message_length = 100;
byte message[max_message_length]; // byte stream used to hold the current message for sends
int totalBytesInMessage = 0; // used for sends and receives

/*
 * States:
 *  0 = Base state, no messages are being sent or received
 *  1 = Record message of max 10 seconds
 *  2 = Send message over from one device to the other
 *  3 = Receiver device lights up, indicating available message
 *  4 = Receiver device plays message
 *  5 = State 5 is a sort of lock state that will allow us to only have one message
 *  be set at a time to avoid odd edge cases
 */
int current_state = 0; 


// recordMessage checks for user inputs every 100ms
// If a button is pressed, it is lit up, and writes 
// to a unique bit of the output.
// Example: Button 1 and button 4 of buttons 0-4 are 
// pressed; expected output == 0x00010010
byte recordMessage() {
  byte output = 0x00;
  int buttonInput;
  int brightness;
  for (int i = 0; i < num_buttons; i++) {
    buttonInput = digitalRead(input_pins[i]);
    brightness = buttonInput ? brightnesses[i] : 0;
    analogWrite(output_pins[i], brightness);
    output |= buttonInput << i;
  }
  delay(100);
  return output;
}

// playMessage takes in a byte that was read from
// serial and lights up the corresponding light for
// 100ms. 
// Example: input 0x00010010 will light up Button 1 
// and button 4 of buttons 0-4
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
        Serial.read();
        // turn on light, indicating message is being transmitted over
        digitalWrite(message_available, HIGH);
        current_state = 5;
        break;
      }
      // check to record message
      if (digitalRead(message_play)) {
        Serial.write(0x80);
        current_state = 1;
      }
      break;
    case 1: // record and send message
      for (int i = 0; i < max_message_length; ++i) {
        totalBytesInMessage = i + 1;
        // check for message completion
        if (digitalRead(message_play)) {
          current_state = 2;
          break;
        }
        message[i] = recordMessage();
      }
      for (int i = 0; i < num_buttons; i++) {
        analogWrite(output_pins[i], 0);
      }
      Serial.write(totalBytesInMessage);
      current_state = 2;
      break;
    case 2: // message delivery
      Serial.write(0xFF);
      Serial.write(message, totalBytesInMessage);
      // only one message can be sent at a time. To avoid multiple sends,
      // sender waits until receiver plays the sent message. Flash light
      // until message is recieved
      if (Serial.available() > 0) {
        Serial.read();
        current_state = 0;
      } else if (flash) {
        digitalWrite(message_available, HIGH);
      } else {
        digitalWrite(message_available, LOW);
      }
      flash = !flash;
      delay(500);
      break;
    case 3: // message receieved -- flash receive light until ack'd
      // stay in receive state until receiver plays message
      if (digitalRead(message_play)) {
        // turn off light once receiver acknowledges message
        digitalWrite(message_available, LOW);
        Serial.write(0xA0);
        current_state = 4;
        break;
      } else if (flash) {
        digitalWrite(message_available, HIGH);
      } else {
        digitalWrite(message_available, LOW);
      }
      flash = !flash;
      delay(500);
      break;
    case 4: // play message
      while(Serial.read() != 0xFF){
        continue;
      }
      for (int i = 0; i < totalBytesInMessage; i++) {
        playMessage(Serial.read());
      }
      current_state = 0;
      break;
    case 5: // invariant state -- only one device can send a messge at a time
      if (Serial.available() > 0) {
        totalBytesInMessage = Serial.read();
        current_state = 3;
      }
      break;
  }
}
