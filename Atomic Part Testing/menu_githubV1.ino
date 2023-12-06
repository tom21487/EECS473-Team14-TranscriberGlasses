//below pins were on arduino for testing

#define USER_RST 7
#define USER_CLK 8
#define USER_DATA_4 13
#define USER_DATA_3 12
#define USER_DATA_2 11
#define USER_DATA_1 10
#define USER_DATA_0 9

// below pins on ESP
#define SCROLL 34
#define ENTER 35


#define WRITE_DELAY 50
#define CLS_DELAY 10
#define CHAR_MEM_SIZE 128

#define ADC_VREF_mV     3300.0 // in millivolt
#define ADC_RESOLUTION  4096.0
#define THERM_OFFSET    500.0
#define THERM_SCALE     0.1




uint8_t char_map[128];
char language_options[][7] = { "  ", "ZH", "FR", "EN", "ES", "HR", "  " };
uint8_t language_options_size = sizeof(language_options) / sizeof(language_options[0]) - 2;
char msg[] = "WE ARE IN TRANSLATE MODE";
unsigned int msg_len;
unsigned int msg_idx;


const uint8_t DISPLAY_LENGTH_GRIDS = 16;
const uint8_t DISPLAY_HEIGHT_GRIDS = 8;
const uint8_t DISPLAY_NUM_GRIDS = DISPLAY_HEIGHT_GRIDS * DISPLAY_LENGTH_GRIDS;
uint8_t middle_row = (DISPLAY_HEIGHT_GRIDS / 2) - 1;
uint8_t middle_col = (DISPLAY_LENGTH_GRIDS / 2) - 1;
char whole_screen[DISPLAY_NUM_GRIDS];



uint8_t button_position;
bool button_pressed;
bool enter_pressed;
bool translate_mode;
bool temp_check_mode;


void pin_setup();
void char_map_setup();
void selecting_language();
void displayStarterScreen();
void write_screen();
void scroll_ISR();
void enter_ISR();
void wordAtPosition(char* input, uint8_t row_pos, uint8_t col_pos);
void user_reset();
void menuLoop();
void translateLoop();
void display_temperature();
void temp_check_ISR();
void clearScreen_buff();
void consecutive_write();
void resetBuffAndCursor();
void logo_loop();

  void
  setup() {
  init();
  Serial.begin(19200);
   while (!Serial) {
    ;  // wait for serial port to connect. Needed for native USB, on LEONARDO, MICRO, YUN, and other 32u4 based boards.
    }
  Serial.println("Setup started");
  delay(1000);

  pin_setup();
  char_map_setup();
  // below is the message which should eventually get replaced by translated message
  

  button_position = (language_options_size / 2) - 1;
    
  // This reset sequence clears the character memory.
  uint8_t space = char_map[' '];
  digitalWrite(USER_DATA_4, (space >> 4) & 1);
  digitalWrite(USER_DATA_3, (space >> 3) & 1);
  digitalWrite(USER_DATA_2, (space >> 2) & 1);
  digitalWrite(USER_DATA_1, (space >> 1) & 1);
  digitalWrite(USER_DATA_0, (space >> 0) & 1);
  user_reset();
  clearScreen_buff();
  delay(1000);
  
  
  

 /* enter_pressed = false;
  translate_mode = false;
  button_pressed = false;
  */ 
  user_reset();
  delay(1000);
Serial.println("Setup ended");

  logo_loop();
  user_reset();
  clearScreen_buff();
  delay(1000);
  displayStarterScreen();
  delay(1000);
  menuLoop();
  translateLoop();
  
}

void loop() {
  // 1. Setup.
  // nothing is here
}
void logo_loop(){
  enter_pressed = false;
  user_reset();
  delayMicroseconds(8000);
  clearScreen_buff();
  delayMicroseconds(8000);
  Serial.println(enter_pressed);
  /* below text is
  "                                 TRANSCRIBER   
                                  _         _     
                                   \         \    
                                    \ GLASSES \   
                                     \__     __\  
                                    /   \___/   \ 
                                    \___/   \___/ 
                                                  
                                                  "
                                  */
  char temp[DISPLAY_NUM_GRIDS] = "  TRANSCRIBER   _         _      \\         \\      \\ GLASSES \\      \\__     __\\    /   \\___/   \\   \\___/   \\___/                                ";
  int j = 0;
  for(int i = 0; i < DISPLAY_NUM_GRIDS; i++){
      if(j == 16){
        j = 0;
        Serial.println("");
      }
      Serial.print(temp[i]);
      
    j++;
  }
  wordAtPosition(temp, 0, 0, DISPLAY_NUM_GRIDS);  
  delayMicroseconds(8000);
  attachInterrupt(digitalPinToInterrupt(ENTER), enter_ISR, RISING);
  Serial.println("Inside of logo loop");
  enter_pressed = false;
  Serial.println(enter_pressed);
  while(enter_pressed == false){
    
     Serial.println(enter_pressed);
    
  }
  Serial.println("Out of logo loop");
  detachInterrupt(digitalPinToInterrupt(ENTER));
  delayMicroseconds(8000);
  user_reset();
  delayMicroseconds(8000);
}

void menuLoop(){
  // 1. Setup.
  enter_pressed = false;
  translate_mode = false;
  button_pressed = false;
  temp_check_mode = false;
  attachInterrupt(digitalPinToInterrupt(SCROLL), scroll_ISR, RISING);
  attachInterrupt(digitalPinToInterrupt(ENTER), enter_ISR, RISING);
  while(!translate_mode){
    if(button_pressed && !translate_mode){
    Serial.println("Scroll mode entered");
    
    noInterrupts();
    displayStarterScreen();
    interrupts();
  
    delay(1000);
    button_pressed = false;
    

  }
  if(enter_pressed && !translate_mode){
    detachInterrupt(digitalPinToInterrupt(ENTER));
    detachInterrupt(digitalPinToInterrupt(SCROLL));
    
    Serial.println("Enter mode entered");
    clearScreen_buff();
    delay(1000);
    user_reset();
    delay(1000);
    enter_pressed = false;
    translate_mode = true;
  }
  }


  
  
  // 4. Update next state.


}

void translateLoop(){
  temp_check_mode = false;
  attachInterrupt(digitalPinToInterrupt(ENTER), temp_check_ISR, RISING);
  // if temp button is odd
  uint8_t temp_button = 1;
  msg_idx = 0;
  msg_len = strlen(msg);  
  while(1){
    
    if(temp_check_mode == true){
      Serial.print("Before math:");
      Serial.println(temp_button);
      temp_button = (temp_button + 1) % 2;
      Serial.print("after math:");
      Serial.println(temp_button);
        if(temp_button % 2 == 1){
          Serial.println("TEMP CHECK MODE ENTERED MODULO 1");
          noInterrupts();
          display_temperature();
          interrupts();
          delay(1000);
          temp_check_mode = false;
        }
        else if(temp_button % 2 == 0){
          resetBuffAndCursor();
          Serial.println("TEMP CHECK MODE ENTERED MODULO 2");
          delay(1000);
          temp_check_mode = false;
        }
             
    }
    if(temp_button % 2 == 0){
      consecutive_write();  
    }
        
    }  
}

  void resetBuffAndCursor(){
    clearScreen_buff();
   // write_screen();
    user_reset();
    delay(1000);
  }
  


void write_screen(){
uint8_t col_num = 0;
  for(int i = 0; i < DISPLAY_NUM_GRIDS; i++){
    uint8_t user_data = char_map[whole_screen[i]];
  digitalWrite(USER_CLK, LOW);
  digitalWrite(USER_DATA_4, (user_data >> 4) & 1);
  digitalWrite(USER_DATA_3, (user_data >> 3) & 1);
  digitalWrite(USER_DATA_2, (user_data >> 2) & 1);
  digitalWrite(USER_DATA_1, (user_data >> 1) & 1);
  digitalWrite(USER_DATA_0, (user_data >> 0) & 1);
  delayMicroseconds(8000);
  // 2. Rising edge.
  digitalWrite(USER_CLK, HIGH);
  // 2. Hold.
  delayMicroseconds(8000);

  // 4. Update next state.
  if(whole_screen[i] == ' '){
    Serial.print(".");
  }
  else{
    Serial.print(whole_screen[i]);    
  }
  
  
  col_num++;
  if(col_num > 15){
    col_num = 0;
    Serial.println("");
    }
  }

 
}



void wordAtPosition(char* input, uint8_t row_pos, uint8_t col_pos, uint8_t size){

uint8_t posTracker = row_pos*DISPLAY_LENGTH_GRIDS + col_pos;


char* iter = input;


for(int i = 0; i < size; i++){
  
  whole_screen[posTracker] = iter[i];
  posTracker += 1;
  
}

    
}




void displayStarterScreen(){

  
Serial.println("display start");
    for(int i = 1; i < language_options_size + 1; i++){
    Serial.println(language_options[i]);   
  }

  
  button_position = (button_position + 1) % language_options_size;
  Serial.println("button_position below");
  Serial.println(button_position);
  uint8_t temp = 0;
  for(uint8_t i = button_position + 1; i < button_position + 4; i++){
    
   


    wordAtPosition(language_options[i - 1], middle_row + temp + 1, 2, strlen(language_options[i - 1]));
    temp++;
  }

  for(int i = 0; i < DISPLAY_NUM_GRIDS; i++){
    if(whole_screen[i] == ' '){
    Serial.print(".");
  }
  else{
    Serial.print(whole_screen[i]);    
  }
  }

    wordAtPosition("EN", middle_row + 2, 12, 2);
    wordAtPosition("TO", middle_row + 2, 8, 2);
  write_screen();
}







void consecutive_write(){
  uint8_t user_data = char_map[msg[msg_idx]];
  digitalWrite(USER_CLK, LOW);
  digitalWrite(USER_DATA_4, (user_data >> 4) & 1);
  digitalWrite(USER_DATA_3, (user_data >> 3) & 1);
  digitalWrite(USER_DATA_2, (user_data >> 2) & 1);
  digitalWrite(USER_DATA_1, (user_data >> 1) & 1);
  digitalWrite(USER_DATA_0, (user_data >> 0) & 1);
  delayMicroseconds(8000);
  delayMicroseconds(8000);
  delayMicroseconds(8000);
  delayMicroseconds(8000);

  // 2. Rising edge.
  digitalWrite(USER_CLK, HIGH);
  // 2. Hold.
  delayMicroseconds(8000);
  delayMicroseconds(8000);
  delayMicroseconds(8000);
  delayMicroseconds(8000);
  // 4. Update next state.
  msg_idx = msg_idx == msg_len - 1 ? 0 : msg_idx + 1;
}

void display_temperature(){

    int adcVal = analogRead(A5);
    float tempC = (adcVal - THERM_OFFSET) * THERM_SCALE;

    // convert the °C to °F
    float tempF = tempC * 9 / 5 + 32;
    char tempCdisplay[7]; 
    char tempFdisplay[7];
    Serial.println(adcVal);
    dtostrf(tempC, 5, 2, tempCdisplay);
    dtostrf(tempF, 5, 2, tempFdisplay);
    Serial.println(tempCdisplay);
    Serial.println(tempFdisplay);
    Serial.println("Display_temperature function");
    user_reset();
    delayMicroseconds(8000);
    wordAtPosition("TEMP IS", middle_row + 2, 0, 7);
    wordAtPosition("TEMPFDISPLAY", middle_row + 2, 9, 7);
    write_screen();
    delayMicroseconds(8000);
}



 void pin_setup(){
  pinMode(USER_RST, OUTPUT);
  pinMode(USER_CLK, OUTPUT);
  pinMode(USER_DATA_4, OUTPUT);
  pinMode(USER_DATA_3, OUTPUT);
  pinMode(USER_DATA_2, OUTPUT);
  pinMode(USER_DATA_1, OUTPUT);
  pinMode(USER_DATA_0, OUTPUT);
  

  pinMode(ENTER, INPUT_PULLUP);
  pinMode(SCROLL, INPUT_PULLUP);
  }

  
// sets up the global char map to be used.
void char_map_setup(){
  char_map[' '] = 0;
  char_map['.'] = 1;
  char_map[','] = 2;
  char_map['#'] = 3;
  char_map['!'] = 4;
  char_map['*'] = 5;
  char_map['A'] = 6;
  char_map['B'] = 7;
  char_map['C'] = 8;
  char_map['D'] = 9;
  char_map['E'] = 10;
  char_map['F'] = 11;
  char_map['G'] = 12;
  char_map['H'] = 13;
  char_map['I'] = 14;
  char_map['J'] = 15;
  char_map['K'] = 16;
  char_map['L'] = 17;
  char_map['M'] = 18;
  char_map['N'] = 19;
  char_map['O'] = 20;
  char_map['P'] = 21;
  char_map['Q'] = 22;
  char_map['R'] = 23;
  char_map['S'] = 24;
  char_map['T'] = 25;
  char_map['U'] = 26;
  char_map['V'] = 27;
  char_map['W'] = 28;
  char_map['X'] = 29;
  char_map['Y'] = 30;
  char_map['Z'] = 31;

}

// clearScreen_buff clears the enitrity of thechar buffer which is used to write the screen with
void clearScreen_buff(){
  
  for(int i = 0; i < DISPLAY_NUM_GRIDS; i++){
    whole_screen[i] = '-';
  }
  
}


// user reset triggers the FPGA internall interrupt
void user_reset(){
  digitalWrite(USER_RST, LOW);
  delayMicroseconds(8000);
  digitalWrite(USER_RST, HIGH);
}


void scroll_ISR() {
  button_pressed = true;
}

void enter_ISR() {
  Serial.println("IN ENTER INTERRUPT");
  enter_pressed = true;
}

void temp_check_ISR(){
  temp_check_mode = true;
}
