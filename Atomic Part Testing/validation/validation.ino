/* 6-pin ESP headers
#define USER_RST 23
#define USER_CLK 22
#define USER_DATA_5 18
#define USER_DATA_4 32
#define USER_DATA_3 33
#define USER_DATA_2 25
#define USER_DATA_1 19
#define USER_DATA_0 21
*/

// 3-pin ESP headers
#define USER_RST 23
#define USER_CLK 22
#define USER_DATA_2 18
#define USER_DATA_1 19
#define USER_DATA_0 21

/* 6-pin Arduino headers
#define USER_RST 2
#define USER_CLK 12
#define USER_DATA_5 13
#define USER_DATA_4 4
#define USER_DATA_3 5
#define USER_DATA_2 6
#define USER_DATA_1 7
#define USER_DATA_0 8
*/

/* 3-pin Arduino headers
#define USER_RST 2
#define USER_CLK 12
#define USER_DATA_2 5
#define USER_DATA_1 7
#define USER_DATA_0 8
*/

/* Constants */
#define WRITE_DELAY 10
#define CLS_DELAY 10
#define CHAR_MEM_SIZE 128

uint8_t char_map[128];
char msg[] = "HELLO WORLD!";
unsigned int msg_len;
unsigned int msg_idx;
uint8_t user_data;

void setup() {
  // put your setup code here, to run once:
  pinMode(USER_RST, OUTPUT);
  pinMode(USER_CLK, OUTPUT);
  pinMode(USER_DATA_2, OUTPUT);
  pinMode(USER_DATA_1, OUTPUT);
  pinMode(USER_DATA_0, OUTPUT);

  msg_idx = 0;
  msg_len = strlen(msg);

  // Space must be zero because that's the reset value.
  char_map[' '] = 0;
  // English uppercase.
  char_map['A'] = 1;  char_map['B'] = 2;  char_map['C'] = 3;  char_map['D'] = 4;  char_map['E'] = 5;
  char_map['F'] = 6;  char_map['G'] = 7;  char_map['H'] = 8;  char_map['I'] = 9;  char_map['J'] = 10;
  char_map['K'] = 11; char_map['L'] = 12; char_map['M'] = 13; char_map['N'] = 14; char_map['O'] = 15;
  char_map['P'] = 16; char_map['Q'] = 17; char_map['R'] = 18; char_map['S'] = 19; char_map['T'] = 20;
  char_map['U'] = 21; char_map['V'] = 22; char_map['W'] = 23; char_map['X'] = 24; char_map['Y'] = 25;
  char_map['Z'] = 26;
  // Numbers.
  char_map['0'] = 27; char_map['1'] = 28; char_map['2'] = 29; char_map['3'] = 30; char_map['4'] = 31;
  char_map['5'] = 32; char_map['6'] = 33; char_map['7'] = 34; char_map['8'] = 35; char_map['9'] = 36;
  // Special characters.
  char_map['.']  = 37; char_map[','] = 38; char_map['?'] = 39; char_map['!'] = 40; char_map['/'] = 41;
  char_map['\\'] = 42; char_map['_'] = 43; char_map['-'] = 44; char_map['|'] = 45; char_map['o'] = 46;
  char_map[':']  = 47; char_map['\''] = 48;

  user_data = 0;

  fpga_reset();
  fpga_transmit("HELLO WORLD ");
  fpga_transmit("THIS IS TRANSCRIBER GLASSES ");
  fpga_transmit("FOR EECS 473");
  fpga_transmit("WHY DOES THIS WORK EVERY OTHER TIME ");
  fpga_transmit("NO IDEA WHY ");
}

void fpga_reset() {
  digitalWrite(USER_RST, LOW);
  digitalWrite(USER_CLK, LOW);
  digitalWrite(USER_DATA_2, LOW);
  digitalWrite(USER_DATA_1, LOW);
  digitalWrite(USER_DATA_0, LOW);
  delay(500);
  digitalWrite(USER_RST, HIGH);
  delay(500);
}

void fpga_transmit(const char * text) {
  // Pull down the pins
  digitalWrite(USER_CLK, LOW);
  digitalWrite(USER_DATA_2, LOW);
  digitalWrite(USER_DATA_1, LOW);
  digitalWrite(USER_DATA_0, LOW);
  delay(500);
  size_t n = strlen(text);
  for (size_t i = 0; i < n; i++) {
    char c = text[i];
    uint8_t d = char_map[c];
    // Write lower bits.
    // 1. Setup.
    digitalWrite(USER_CLK, LOW);
    digitalWrite(USER_DATA_2, (d >> 2) & 1);
    digitalWrite(USER_DATA_1, (d >> 1) & 1);
    digitalWrite(USER_DATA_0, (d >> 0) & 1);
    delay(WRITE_DELAY);
    // 2. Rising edge.
    digitalWrite(USER_CLK, HIGH);
    // 3. Hold.
    delay(WRITE_DELAY);
    // Write upper bits.
    // 1. Setup
    digitalWrite(USER_CLK, LOW);
    digitalWrite(USER_DATA_2, (d >> 5) & 1);
    digitalWrite(USER_DATA_1, (d >> 4) & 1);
    digitalWrite(USER_DATA_0, (d >> 3) & 1);
    delay(WRITE_DELAY);
    // 2. Rising edge.
    digitalWrite(USER_CLK, HIGH);
    // 3. Hold.
    delay(WRITE_DELAY);
  }
  // Hardcode another cycle to display the last character.
  // 1. Setup.
//  digitalWrite(USER_CLK, LOW);
//  delay(WRITE_DELAY);
  // 2. Rising edge.
//  digitalWrite(USER_CLK, HIGH);
  // 3. Hold.
//  delay(WRITE_DELAY);
}

void loop() {
  return;
  // Write lower bits.
  // 1. Setup.
  digitalWrite(USER_CLK, LOW);
  digitalWrite(USER_DATA_2, (user_data >> 2) & 1);
  digitalWrite(USER_DATA_1, (user_data >> 1) & 1);
  digitalWrite(USER_DATA_0, (user_data >> 0) & 1);
  delay(WRITE_DELAY);
  // 2. Rising edge.
  digitalWrite(USER_CLK, HIGH);
  // 3. Hold.
  delay(WRITE_DELAY);
  // Write upper bits.
  // 1. Setup
  digitalWrite(USER_CLK, LOW);
  digitalWrite(USER_DATA_2, (user_data >> 5) & 1);
  digitalWrite(USER_DATA_1, (user_data >> 4) & 1);
  digitalWrite(USER_DATA_0, (user_data >> 3) & 1);
  delay(WRITE_DELAY);
  // 2. Rising edge.
  digitalWrite(USER_CLK, HIGH);
  // 3. Hold.
  delay(WRITE_DELAY);
  // 4. Update next state.
  user_data = (user_data == 48) ? 0 : (user_data + 1);
  // msg_idx = msg_idx == msg_len - 1 ? 0 : msg_idx + 1;
}
