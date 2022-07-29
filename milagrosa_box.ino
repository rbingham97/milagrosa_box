// code for comms boxes at La Milagrosa
// Russell Bingham
// 7/27/22

int num_buttons = 5;
int input_pins[5] = {2, 4, 7, 8, 12}; // switches
int output_pins[5] = {3, 5, 6, 9, 10}; // lights (PWM)
int states[5] = {0, 0, 0, 0, 0}; // FSM states
int brightnesses[5] = {0, 0, 0, 0, 0}; // light states
int directions[5] = {1, 1, 1, 1, 1}; // up/down blinking
// b if that button has been pressed, reset to a on send
char staging[5] = {'a', 'a', 'a', 'a', 'a'}; // part of debouncing
int last_val[5] = {0, 0, 0, 0, 0}; // what the button was last cycle
char press_ID [5] = {'1', '2', '3', '4', '5'};
char incoming_notif = '0';

int max_brightness = 100; 

int num_states = 4; // off, solid, blinking, fading -- in order
int slopes[4] = {0, 0, 3, 3};
int offsets[4] = {0, max_brightness, 0, max_brightness};
int execute_offset[4] = {1, 1, 0, 0};

int holder = 0;

void update_state(int button, int state) {
  states[button] = state;
  if (execute_offset[state] == 1) {
    brightnesses[button] = offsets[state];
  }
}

void setup() {
  Serial.begin(9600);

  for (int i = 0; i < num_buttons; i++) {
    pinMode(input_pins[i], INPUT);
    pinMode(output_pins[i], OUTPUT);
  }
}

void loop() {

  // read any press_IDs from remote
  if (Serial.available() > 0) {
    incoming_notif = Serial.read();
    for (int i = 0; i < num_buttons; i++) {
      if (press_ID[i] == incoming_notif) {
        if (states[i] == 0) {
          update_state(i, 2); // off to blinking
        }
        else if (states[i] == 1) {
          update_state(i, 3); // solid to fading
        }      
        break;
      }
    }
  }

  // check for pressed buttons locally
  for (int i = 0; i < num_buttons; i++) {
    holder = digitalRead(input_pins[i]);
    if (holder == HIGH && last_val[i] != HIGH) {
      staging[i] = 'b';
      if (states[i] == 0) { // off to solid
        update_state(i, 1);
      }
      else if (states[i] == 2) { // blinking to off
        update_state(i, 3);
      }
    }
    last_val[i] = holder;
  }

  // update lights
  for (int i = 0; i < num_buttons; i++) {
    if (brightnesses[i] >= max_brightness) {
      directions[i] = -1;
    }
    else if (brightnesses[i] <= slopes[i]) {
      directions[i] = 1;
      if (states[i] == 3) {
        update_state(i, 0);
      }
    }
    
    brightnesses[i] += directions[i]*slopes[states[i]];
    analogWrite(output_pins[i], brightnesses[i]);
  }

  // send press_ID(s)  
  for (int i = 0; i < num_buttons; i++) {
    if (staging[i] == 'b') { // send press notification
      Serial.write(press_ID[i]);
      staging[i] = 'a';
    }
  }

  
  
  delay(30);
}
