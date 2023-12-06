#include "driver/i2s.h"
#include "SPIFFS.h"
#include "SPI.h"
#include "SD.h"
#include "FS.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "FPGA.h"
#include "menu.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define I2S_SAMPLE_RATE (16000)

#define I2S_SAMPLE_BITS (16) // Resolution
#define I2S_READ_LEN    (16 * 1024)
#define I2S_RECORD_TIME (8) //Seconds
#define I2S_CHANNEL_NUM (1)
#define I2S_FLASH_RECORD_SIZE (I2S_CHANNEL_NUM * I2S_SAMPLE_RATE * I2S_SAMPLE_BITS / 8 * I2S_RECORD_TIME)

// INMP microphone.
#define I2S_WS   17
#define I2S_SD   5
#define I2S_SCK  32
#define I2S_PORT I2S_NUM_0

// This is the SD card setup.
//#define SD_CS         13
//#define SPI_MOSI      15
//#define SPI_MISO      2
//#define SPI_SCK       14
#define SCROLL 34
#define ENTER  35
// Safe pins: 16,18,19,21

// These are the new pins, note that MISO has been switched from 2 to 12.
#define SD_CS         13
#define SPI_MOSI      15
#define SPI_MISO      12
#define SPI_SCK       14

#define US_DELAY 250
#define DEBOUNCE_DELAY 50000
#define DEBUG

// adding temp GPIO below
#define temperature     39
#define ADC_FREF_mv     3300.0
#define ADC_RESOLUTION  4096.0
#define THERM_OFFSET    500.0
#define THERM_SCALE     0.1


const char *ssid = "********";
const char *password = "********";
// TODO add this to FPGA class.
uint8_t char_map[CHAR_MEM_SIZE];
// TODO add Portuguese, Arabic, Korean, Polish, German because we know people speak these languages.
// - Tom
char language_options[][13] = { "  ", "ZH", "FR", "EN", "ES", "PT", "AR", "KO", "PL", "JP", "DE", "RU", "  " };
uint8_t language_options_size = sizeof(language_options) / sizeof(language_options[0]) - 2;

char language_options_display[][7] = { "  ", "PO", "FR", "EN", "ES", "HR", "  " };
uint8_t language_options_display_size = sizeof(language_options_display) / sizeof(language_options_display[0]) - 2;

uint8_t middle_row = (DISPLAY_HEIGHT_GRIDS / 2) - 1;
uint8_t middle_col = (DISPLAY_LENGTH_GRIDS / 2) - 1;
char whole_screen[DISPLAY_NUM_GRIDS];

uint8_t button_position;
uint8_t button_position_2;
volatile bool button_pressed;
volatile bool enter_pressed;
bool translate_mode;
bool temp_check_mode;

// Things that need to be global.
SPIClass SPI2;
// See here for size:
// https://arduinojson.org/v6/assistant/#/step1
StaticJsonDocument<2048> jsonDoc;
HTTPClient transcribe_client;
HTTPClient translate_client;
// SD
void initSDCard();
// I2S.
void i2sInit();
void i2sRecording(const char* filename);
// Microsoft API.
void wifi_connect();
void microsoft_connect(int from_lang, int to_lang);
const char * uploadFile(const char* filename);
const char * translate(const char* text);

// FPGA Functions
void fpga_init();
void fpga_transmit(const char* text);
void displayStarterScreen();
void write_screen();
void scroll_ISR();
void enter_ISR();
void wordAtPosition(char* input, uint8_t row_pos, uint8_t col_pos);
void user_reset();
void menuLoop();
void clearScreen_buff();
void logo_loop();
void temp_loop();
void mainMenu();
// void display_temperature();
// void temp_check_ISR();
// void consecutive_write();
// void resetBuffAndCursor();

// Helper Functions
const int headerSize = 44;
byte header[headerSize];
void i2s_adc_data_scale(uint8_t * d_buff, uint8_t* s_buff, uint32_t len);
void wavHeader(byte* header, int wavSize);
void listDir(fs::FS &fs, const char * dirname, uint8_t levels);
String getTranscribeLanguage(int from_lang);
String getTranslateLanguage(int from_lang, int to_lang);
int from_lang = 3;
int to_lang   = 2;

void setup() {
  Serial.begin(115200);
#ifdef DEBUG
  Serial.println("[setup]: hello 473.");
#endif
  fpga_init();
  mainMenu();

  clearScreen_buff();
  wordAtPosition("INIT SD CARD...", 0, 0, 15);
  write_screen();
  delay(1000);

  // Initialization of I2S and SD card
  initSDCard();
#ifdef DEBUG
  Serial.println("[setup]: initial contents of SD card: ");
#endif
  listDir(SD, "/", 0); // Maybe listDir doesn't work because it doesn't close the file?
  char filename[] = "/test_0000.wav";

  clearScreen_buff();
  wordAtPosition("WIFI CONNECT...", 0, 0, 15);
  write_screen();
  delay(1000);

  wifi_connect();

  clearScreen_buff();
  wordAtPosition("API CONNECT...", 0, 0, 14);
  write_screen();
  delay(1000);
   
  microsoft_connect(from_lang, to_lang);

  clearScreen_buff();
  wordAtPosition("RECORDING...", 0, 0, 12);
  write_screen();
  delay(1000);

  bool first_time = true;
  for (int i = 0; i < 15; i++) {
     i2sRecording(filename);
     if (first_time) {
      clearScreen_buff();
      wordAtPosition("TRANSCRIBING...", 0, 0, 15);
      write_screen();
      delay(1000);       
     }
     const char * transcribed_text = uploadFile(filename);
     const char* translated_text;
     if(from_lang==to_lang){
       translated_text = transcribed_text;
     }else{
       translated_text = translate(transcribed_text);
     }
     if (!transcribed_text) {
       Serial.println("[setup]: transcribe failed.");
     }
     // Warning: keeping WiFi connected while doing FPGA transmission may cause failure
     if(!translated_text){
       Serial.println("[setup]: translate failed.");
     }
     if (first_time) {
       user_reset();
       first_time = false;
     }
     fpga_transmit(translated_text);
   }
   transcribe_client.end();
   if (translate_client.connected()) {
     translate_client.end();
   }
   WiFi.disconnect();
}

void i2sInit() {
  static const i2s_config_t i2s_config = 
  {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
      .sample_rate = I2S_SAMPLE_RATE,                       // Note, all files must be this
      .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
      .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
      .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,             // high interrupt priority
      .dma_buf_count = 8,                                  // 8 buffers
      .dma_buf_len = 256,                                  // 256 bytes per buffer, so 2K of buffer space
      .use_apll=1 
  };

  // I2S configuration for the pins:
  static const i2s_pin_config_t pin_config = 
  {
      .bck_io_num = I2S_SCK,                           // The bit clock connectiom, goes to pin 27 of ESP32
      .ws_io_num = I2S_WS,                             // Word select, also known as word select or left right clock
      .data_out_num = -1,                              // Data out from the ESP32, connect to DIN on 38357A
      .data_in_num = I2S_SD                  
  };
#ifdef DEBUG
  Serial.println("Trying to install I2S driver.");
#endif
  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);   //install and start i2s driver
#ifdef DEBUG
  Serial.println("Trying to set I2S pin.");
#endif
  i2s_set_pin(I2S_PORT, &pin_config);                   //Set the pin configuration
#ifdef DEBUG
  Serial.println("I2S Init Done");
#endif
}

void i2sRecording(const char* filename)
{
    i2sInit();
    int i2s_read_len = I2S_READ_LEN;
    int flash_wr_size = 0;
    size_t bytes_read;
    File file = SD.open(filename, FILE_WRITE);
    if (!file)
    {
#ifdef DEBUG
      Serial.print("[i2sRecording]: Unable to open ");
      Serial.print(filename);
      Serial.println(" for writing.");
#endif
      return;
    }
    //  if(!file)
    //  {
    //    file = SD.open(filename, FILE_READ);
    //    if(!file)
    //    {
    //      Serial.println(filename);
    //      Serial.println("FILE IS NOT AVAILABLE!");
    //      return; // "FILE IS NOT AVAILABLE!";
    //    }
    //  }
#ifdef DEBUG
  Serial.println(" *** Recording Start *** ");
  Serial.println(i2s_read_len);
#endif
  char* i2s_read_buff = (char*) calloc(i2s_read_len, sizeof(char));
  uint8_t* flash_write_buff = (uint8_t*) calloc(i2s_read_len, sizeof(char));
  i2s_read(I2S_PORT, (void*) i2s_read_buff, i2s_read_len, &bytes_read, portMAX_DELAY);
  i2s_read(I2S_PORT, (void*) i2s_read_buff, i2s_read_len, &bytes_read, portMAX_DELAY);
    
    while (flash_wr_size < I2S_FLASH_RECORD_SIZE) {
        //read data from I2S bus, in this case, from ADC.
        i2s_read(I2S_PORT, (void*) i2s_read_buff, i2s_read_len, &bytes_read, portMAX_DELAY);
        // example_disp_buf((uint8_t*) i2s_read_buff, 64);
        //save original data from I2S(ADC) into flash.
        i2s_adc_data_scale(flash_write_buff, (uint8_t*)i2s_read_buff, i2s_read_len);
        file.write((const byte*)flash_write_buff/*i2s_read_buff*/, i2s_read_len);
        flash_wr_size += i2s_read_len;
        ets_printf("Sound recording %u%%\n", flash_wr_size * 100 / I2S_FLASH_RECORD_SIZE);
        // ets_printf("Never Used Stack Size: %u\n", uxTaskGetStackHighWaterMark(NULL));
    }
    wavHeader(header, flash_wr_size);
    file.seek(0);
    file.write(header, headerSize);
    Serial.println(" *** Recording Done *** ");
    file.close();
    free(i2s_read_buff);
    i2s_read_buff = NULL;
    free(flash_write_buff);
    flash_write_buff = NULL;

    i2s_driver_uninstall(I2S_PORT);
    //vTaskDelayUntil(&xLastWakeTime, xFrequency);
}

  void i2s_adc_data_scale(uint8_t * d_buff, uint8_t* s_buff, uint32_t len)
  {
      uint32_t j = 0;
      uint32_t dac_value = 0;
      for (int i = 0; i < len; i += 2) {
          dac_value = ((((uint16_t) (s_buff[i + 1] & 0xf) << 8) | ((s_buff[i + 0]))));
          d_buff[j++] = 0;
          d_buff[j++] = dac_value * 256 / 2048;
      }
  }

void wavHeader(byte* header, int wavSize) {
  header[0] = 'R';
  header[1] = 'I';
  header[2] = 'F';
  header[3] = 'F';
  unsigned int fileSize = wavSize + headerSize - 8;
  header[4] = (byte)(fileSize & 0xFF);
  header[5] = (byte)((fileSize >> 8) & 0xFF);
  header[6] = (byte)((fileSize >> 16) & 0xFF);
  header[7] = (byte)((fileSize >> 24) & 0xFF);
  header[8] = 'W';
  header[9] = 'A';
  header[10] = 'V';
  header[11] = 'E';
  header[12] = 'f';
  header[13] = 'm';
  header[14] = 't';
  header[15] = ' ';
  header[16] = 0x10;
  header[17] = 0x00;
  header[18] = 0x00;
  header[19] = 0x00;
  header[20] = 0x01;
  header[21] = 0x00;
  header[22] = 0x01;
  header[23] = 0x00;
  //changed here to record at 44.1khz.
  header[24] = 0x80; // 80 ac44
  header[25] = 0x3e; // 3e
  header[26] = 0x00;
  header[27] = 0x00;
  header[28] = 0x00; //00
  header[29] = 0x7D; //7D 125 7d00 15888
  header[30] = 0x00;  //00
  //finished change sections
  header[31] = 0x00;
  header[32] = 0x02;  
  header[33] = 0x00;
  header[34] = 0x10;
  header[35] = 0x00;
  header[36] = 'd';
  header[37] = 'a';
  header[38] = 't';
  header[39] = 'a';
  header[40] = (byte)(wavSize & 0xFF);
  header[41] = (byte)((wavSize >> 8) & 0xFF);
  header[42] = (byte)((wavSize >> 16) & 0xFF);
  header[43] = (byte)((wavSize >> 24) & 0xFF);
}

void initSDCard() {
  SPI2.begin(SPI_SCK, SPI_MISO, SPI_MOSI, SD_CS);
  if (!SD.begin(SD_CS, SPI2)) {
    Serial.println("Card Mount Failed");
    while(1) yield();
  }
  uint8_t cardType = SD.cardType();
  if(cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    while(1) yield();
  }
  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC){
    Serial.println("MMC");
  } else if (cardType == CARD_SD){
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC){
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
}

void listDir(fs::FS &fs, const char * dirname, uint8_t levels)
{
  Serial.printf("Listing directory: %s\n", dirname);
  File root = fs.open(dirname);
  if(!root)
  {
    Serial.println("Failed to open directory");
    return;
  }
  if(!root.isDirectory())
  {
    Serial.println("Not a directory");
    return;
  }
  File file = root.openNextFile();
  while(file)
  {
    if(file.isDirectory())
    {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if(levels)
      {
        listDir(fs, file.name(), levels -1);
      }
    }
    else
    {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

void wifi_connect() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
  }
  Serial.println(WiFi.localIP());
  // delay(100);
}

String getTranscribeLanguage(int from_lang){
  switch(from_lang){
    case 0: return "zh-CN";
    case 1: return "fr-FR";
    case 2: return "en-US";
    case 3: return "es-ES";
    case 4: return "pt-PT";
    case 5: return "ar-PS";
    case 6: return "ko-KR";
    case 7: return "pl-PL";
    case 8: return "ja-JP";
    case 9: return "de-DE";
    case 10: return "ru-RU";
    default: return "en-US";
  }
}

String getTranslateLanguage(int from_lang, int to_lang){
  String query1;
  String query2;
  switch(from_lang){
    case 0:
      query1 = "&from=zh-Hans";
      break;
    case 1:
      query1 = "&from=fr";
      break;
    case 2:
      query1 = "&from=en";
      break;
    case 3:
      query1 = "&from=es";
      break;
    case 4:
      query1 = "&from=pt-pt";
      break;
    case 5:
      query1 = "&from=ar";
      break;
    case 6:
      query1 = "&from=ko";
      break;
    case 7: 
      query1 = "&from=pl";
      break;
    case 8:
      query1 = "&from=ja";
      break;
    case 9:
      query1 = "&from=de";
      break;
    case 10:
      query1 = "&from=ru";
      break;
    default:
      query1 = "&from=en";
      break;
  }
  switch(to_lang){
    case 1:
      query2 = "&to=zh-Hans";
      break;
    case 2:
      query2 = "&to=en";
      break;
    case 3:
      query2 = "&to=fr";
      break;
    default:
      query2 = "&to=en";
      break;
  }
  return query1+query2;
}

void microsoft_connect(int from_lang, int to_lang) {
  String subscriptionKey1 = "********";
  String location1 = "northcentralus"; // "eastus";
  String language = getTranscribeLanguage(from_lang);
  
  String uri1 = "https://"+location1+".stt.speech.microsoft.com/speech/recognition/conversation/cognitiveservices/v1?language="+language+"&format=detailed";
  transcribe_client.begin(uri1);
  transcribe_client.addHeader("Ocp-Apim-Subscription-Key", subscriptionKey1);
  transcribe_client.addHeader("Ocp-Apim-Subscription-Region", location1);
  transcribe_client.addHeader("Content-Type", "audio/wav");
  if (from_lang == to_lang) return;
  String subscriptionKey2 = "********";
  String location2 ="northcentralus";
  String query = getTranslateLanguage(from_lang, to_lang);
  String uri2 = "https://api.cognitive.microsofttranslator.com/translate?api-version=3.0"+query;
  translate_client.begin(uri2);
  translate_client.addHeader("Ocp-Apim-Subscription-Key", subscriptionKey2);
  translate_client.addHeader("Ocp-Apim-Subscription-Region", location2);
  translate_client.addHeader("Content-Type", "application/json");
}

    
const char * uploadFile(const char * filename) {
  listDir(SD, "/", 0);
  File file = SD.open(filename, FILE_READ);
  if (!file) {
    Serial.print("[uploadFile]: unable to open ");
    Serial.print(filename);
    Serial.println(" for reading.");
    return NULL;
  }
  Serial.print("[uploadFile]: upload ");
  Serial.print(filename);
  Serial.println(" to Microsoft API.");
  Serial.println("[uploadFile]: initializing HTTP client for transcribing.");

  int httpResponseCode = transcribe_client.sendRequest("POST", &file, file.size());
  Serial.print("[uploadFile]: httpResponseCode : ");
  Serial.println(httpResponseCode);
  const char * transcribed_text = NULL;
  if (httpResponseCode == 200) {
    String response = transcribe_client.getString();
    Serial.print("[uploadFile]: httpResponse: ");
    Serial.println(response);
    deserializeJson(jsonDoc, response);
    // const char * json_text = jsonDoc["DisplayText"];
    // transcribed_text = (char *) malloc(strlen(json_text) + 1);
    // strcpy(transcribed_text, json_text);
    transcribed_text = jsonDoc["DisplayText"];
    Serial.print("[uploadFile]: transcribed text = ");
    Serial.println(transcribed_text);
  } else {
    Serial.println("[uploadFile]: invalid httpResponseCode, transcription failed.");
  }
  file.close();
  return transcribed_text;
}

const char* translate(const char* text){
  const char* translated_text;
  String to_post;
  to_post = String("[{'text': \"") + String(text) + String("\"}]");
  Serial.print("[Translation]: text to translate: ");
  Serial.println(String(text));
  Serial.println("[Translation]: initializing HTTP client for Translation.");

  char *tmp1 = (char *) calloc(to_post.length()+1, sizeof(char));
  for(int i = 0; i<to_post.length(); i++){
    tmp1[i] = (to_post.c_str())[i];
  }
  int httpResponseCode = translate_client.sendRequest("POST", (uint8_t *)tmp1, to_post.length());
  Serial.print("[Translation]: httpResponse: ");
  Serial.println(httpResponseCode);
  if (httpResponseCode == 200) {
    String response = translate_client.getString();
    deserializeJson(jsonDoc, response);
    translated_text = jsonDoc[0]["translations"][0]["text"];
    Serial.print("[Translation]: translated text = ");
    Serial.println(translated_text);
  } else {
    Serial.println("[Translation]: invalid httpResponseCode, translation failed.");
  }
  free(tmp1);
  if (translated_text != NULL) {
    return translated_text;
  } else {
    return text;
  }
}

void fpga_init() {
  pinMode(USER_RST, OUTPUT);
  pinMode(USER_CLK, OUTPUT);
  pinMode(USER_DATA_2, OUTPUT);
  pinMode(USER_DATA_1, OUTPUT);
  pinMode(USER_DATA_0, OUTPUT);

  pinMode(ENTER, INPUT_PULLDOWN);
  pinMode(SCROLL, INPUT_PULLDOWN);

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

  user_reset();
  Serial.println("[fpga_init]: done");
  button_position = (language_options_size/2)-1;
  button_position_2 = (language_options_display_size/2)-1;
}

void fpga_transmit(const char* text) {
  Serial.println("[fpga_transmit]: FPGA Transmitting");
  Serial.print("[fpga_transmit]: text = ");
  Serial.println(text);
  unsigned int msg_len = strlen(text);
  char* upper_text = (char*)malloc(msg_len+2);

  for(int i=0; i<msg_len; i++){
    upper_text[i] = toupper(text[i]);
  }
  upper_text[msg_len] = ' ';
  upper_text[msg_len+1] = '\0';
  Serial.println(upper_text);
  for(int i=0; i<msg_len+1; i++){
    uint8_t c = char_map[upper_text[i]];
    digitalWrite(USER_CLK, LOW);
    digitalWrite(USER_DATA_2, (c >> 2) & 1);
    digitalWrite(USER_DATA_1, (c >> 1) & 1);
    digitalWrite(USER_DATA_0, (c >> 0) & 1);
    delay(20);
    digitalWrite(USER_CLK, HIGH);
    delay(20);

    digitalWrite(USER_CLK, LOW);
    digitalWrite(USER_DATA_2, (c >> 5) & 1);
    digitalWrite(USER_DATA_1, (c >> 4) & 1);
    digitalWrite(USER_DATA_0, (c >> 3) & 1);
    digitalWrite(USER_CLK, LOW);
    delay(20);
    digitalWrite(USER_CLK, HIGH);
    delay(20);
  }
  // Hardcode another cycle to display the last character.
  // 1. Setup.
//  digitalWrite(USER_CLK, LOW);
//  delay(WRITE_DELAY);
  // 2. Rising edge.
//  digitalWrite(USER_CLK, HIGH);
  // 3. Hold.
//  delay(WRITE_DELAY);
  // Pull down the pins.
//  digitalWrite(USER_CLK, LOW);
//  digitalWrite(USER_DATA_2, LOW);
//  digitalWrite(USER_DATA_1, LOW);
//  digitalWrite(USER_DATA_0, LOW);
  free(upper_text);
}

void user_reset(){
  digitalWrite(USER_RST, LOW);
  digitalWrite(USER_CLK, LOW);
  digitalWrite(USER_DATA_2, LOW);
  digitalWrite(USER_DATA_1, LOW);
  digitalWrite(USER_DATA_0, LOW);
  delayMicroseconds(US_DELAY);
  digitalWrite(USER_RST, HIGH);
  delayMicroseconds(US_DELAY);
}

void clearScreen_buff(){
  
  for(int i = 0; i < DISPLAY_NUM_GRIDS; i++){
    whole_screen[i] = ' ';
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

void mainMenu(){
  attachInterrupt(digitalPinToInterrupt(SCROLL), scroll_ISR, RISING);
  attachInterrupt(digitalPinToInterrupt(ENTER), enter_ISR, RISING);
  enter_pressed = false;
  button_pressed = false;
  logo_loop();
  clearScreen_buff();
  temp_loop();
  clearScreen_buff();
  displayStarterScreen();
  menuLoop();
  detachInterrupt(digitalPinToInterrupt(ENTER));
  detachInterrupt(digitalPinToInterrupt(SCROLL));
}

void logo_loop(){
  // delayMicroseconds(8000);
  clearScreen_buff();
  // delayMicroseconds(8000);
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
  char temp[] = "  TRANSCRIBER   _         _      \\         \\      \\ GLASSES \\      \\__     __\\    /   \\___/   \\   \\___/   \\___/                                ";
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
  // delayMicroseconds(8000);
  write_screen();
  // delayMicroseconds(8000);
  // attachInterrupt(digitalPinToInterrupt(ENTER), enter_ISR, RISING);
  Serial.println("Inside of logo loop");
  Serial.println(enter_pressed);
  while(enter_pressed == false) {
      // delayMicroseconds(8000);
  }
  Serial.println("Out of logo loop");
  // detachInterrupt(digitalPinToInterrupt(ENTER)); // TODO test attaching interrupt in setup()???
  // delayMicroseconds(8000);
  // user_reset();
  enter_pressed = false;
}

void temp_loop(){
  int adcVal = analogRead(temperature);
  float tempC = (adcVal - THERM_OFFSET) * THERM_SCALE;
  float tempF = tempC * 9 / 5 + 32;
  char tempFdisplay[7]; 
  char tempCdisplay[7]; 
  dtostrf(tempF, 5, 2, tempFdisplay);
  dtostrf(tempC, 5, 2, tempCdisplay);
  Serial.println("Display_temperature");
  Serial.println(tempFdisplay);
  wordAtPosition("TEMP IN F", middle_row - 2, 0, 9);
  wordAtPosition(tempFdisplay, middle_row -1 , 0, 7);
  wordAtPosition("TEMP IN C", middle_row + 1, 0, 9);
  wordAtPosition(tempCdisplay, middle_row + 2 , 0, 7);
  
  write_screen();
  
  // attachInterrupt(digitalPinToInterrupt(ENTER), enter_ISR, RISING);
  while(enter_pressed == false) {
      // delayMicroseconds(8000);
  }
  Serial.println("Out of temp loop");
  enter_pressed = false;
  // detachInterrupt(digitalPinToInterrupt(ENTER));
}

void menuLoop(){
  // 1. Setup.
  translate_mode = false;
  temp_check_mode = false;
  // attachInterrupt(digitalPinToInterrupt(SCROLL), scroll_ISR, RISING);
  // attachInterrupt(digitalPinToInterrupt(ENTER), enter_ISR, RISING);
  while(!translate_mode){
    if (button_pressed && !translate_mode) {
      Serial.println("Scroll mode entered");
      // detachInterrupt(digitalPinToInterrupt(ENTER));
      // detachInterrupt(digitalPinToInterrupt(SCROLL));
      // delayMicroseconds(12000);
      displayStarterScreen();
      // attachInterrupt(digitalPinToInterrupt(SCROLL), scroll_ISR, RISING);
      // attachInterrupt(digitalPinToInterrupt(ENTER), enter_ISR, RISING);
      // delay(1000);
      button_pressed = false;
      Serial.println("button_position");
      Serial.println(button_position);
    }
    if(enter_pressed && !translate_mode){
      // detachInterrupt(digitalPinToInterrupt(ENTER));
      // detachInterrupt(digitalPinToInterrupt(SCROLL));
      Serial.println("Enter mode entered");
      // delay(1000);
      // user_reset();
      // delay(1000);
      enter_pressed = false;
      translate_mode = true;
    }
  }
  from_lang = button_position;
  Serial.print("Language selected: ");
  Serial.println(from_lang);
  // 4. Update next state.
}

void write_screen(){
  // user_reset();
  uint8_t col_num = 0;
  for(int i = 0; i < DISPLAY_NUM_GRIDS; i++){
    uint8_t user_data = char_map[whole_screen[i]];
  digitalWrite(USER_CLK, LOW);
  digitalWrite(USER_DATA_2, (user_data >> 2) & 1);
  digitalWrite(USER_DATA_1, (user_data >> 1) & 1);
  digitalWrite(USER_DATA_0, (user_data >> 0) & 1);
  delayMicroseconds(US_DELAY);
  // 2. Rising edge.
  digitalWrite(USER_CLK, HIGH);
  // 2. Hold.
  delayMicroseconds(US_DELAY);

  digitalWrite(USER_CLK, LOW);
  digitalWrite(USER_DATA_2, (user_data >> 5) & 1);
  digitalWrite(USER_DATA_1, (user_data >> 4) & 1);
  digitalWrite(USER_DATA_0, (user_data >> 3) & 1);
  delayMicroseconds(US_DELAY);
  // 2. Rising edge.
  digitalWrite(USER_CLK, HIGH);
  // 2. Hold.
  delayMicroseconds(US_DELAY);

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
    Serial.println(language_options[i-1]); 
    wordAtPosition(language_options[i - 1], middle_row + temp + 1, 2, strlen(language_options[i - 1]));
    temp++;
  }
  Serial.println("[MENU]: you passed the scroll loop"); 
  for(int i = 0; i < DISPLAY_NUM_GRIDS; i++){
    if(whole_screen[i] == ' '){
    Serial.print(".");
  }
  else{
    Serial.print(whole_screen[i]);    
  }
  }
  uint8_t temp2 = 0;
  Serial.println("Second column starts here");
  for(int i = 1; i < language_options_display_size + 1; i++){
      Serial.println(language_options_display[i]); 
  }
  Serial.println("Button_position_2 is below");
  Serial.println(button_position_2);
  for(uint8_t i = button_position_2 + 1; i < button_position_2; i++){
    Serial.println(language_options_display[i-1]); 
    wordAtPosition(language_options_display[i - 1], middle_row + temp2 + 1, 8, strlen(language_options_display[i - 1]));
    temp2++;
  }
  wordAtPosition("EN", middle_row + 2, 12, 2);
  wordAtPosition("TO", middle_row + 2, 7, 2);
  write_screen();
}

void enter_ISR(){
  delayMicroseconds(DEBOUNCE_DELAY);
  if (digitalRead(ENTER)) {
    enter_pressed = true;
  }
  else {
    enter_pressed = false;
  }
}

void scroll_ISR() {
  delayMicroseconds(DEBOUNCE_DELAY);
  if (digitalRead(SCROLL)) {
    button_pressed = true;
  }
  else {
    button_pressed = false;  
  }
}

void loop() {}
