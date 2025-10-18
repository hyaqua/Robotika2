#include <LiquidCrystal.h>
#include <EEPROM.h>


// -- LCD Variables --
const int rs = 4, en = 5, d4 = 6, d5 = 7, d6 = 8, d7 = 9;  // setting pins for the lcd
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

char line1[16];
char line2[16];
bool update_text = true;


// -- Potenciometer variables --
const int pot_pins[] = { A3, A2, A1, A0 };

int pot_vals[4];

char pot_string[4];


// -- Button variables --
int btn_pins[] = { 2, 3 };  // Red, yellow, 1,2

unsigned long btn_last_int_time[] = { 0L, 0L };

unsigned long debounce_time = 100;

bool button_pressed[] = {false,false};


// -- Menu variables --
int major_mode = 0;

int minor_mode = 0;


// -- Score variables --
struct highscore {
  char name[5];  // The name of the player
  byte tries;     // How many rounds it took to win the game
  unsigned int time;      // Time in seconds
};

int score_count = 10;
highscore scores[10];

bool read_from_eeprom = false;

char name[5];
int name_pos = 0;

int scores_pos = 0;

// -- Game variables --

bool playing = false; // used to know if to update game time

unsigned int calc_time;
unsigned long play_time = 0L;  // 1 = 31ms, 2 = 62ms :d

int number[4];  // The number to guess

// variables for guesses
char last_guess[4];
int correct;
int wrong_place;
int wrong;

int rounds = 15;
int curr_round = 0;


void put_text(char *dest, const char *src, int offset) {
  // we're only using this for line1 and line2 so destination length will always be 16
  int dest_len = 16;
  if (offset < 0 || offset >= dest_len) return;  // if offset is too small or too large do nothing;
  // putting before src_len to not waste cycles :d
  int src_len = strlen(src);

  int copy_len = min(src_len, dest_len - offset);  // check if we can copy the whole text, or just a part of it;

  update_text = true;

  strncpy(dest + offset, src, copy_len);
}

void reset_lines() {
  put_text(line1, "                ", 0);
  put_text(line2, "                ", 0);
}

void check_crc() {
  unsigned int saved_crc;
  EEPROM.get(0, saved_crc); // we retrieve what we currently have in crc
  unsigned int calculated_crc = get_eeprom_crc(2, sizeof(highscore) * score_count); // Skip first two bytes because they contain the crc

  read_from_eeprom = (saved_crc == calculated_crc);
}

void set_crc(){
  unsigned int calculated_crc = get_eeprom_crc(2,sizeof(highscore) * score_count);
  unsigned int saved_crc;
  EEPROM.get(0, saved_crc);
  if(saved_crc != calculated_crc){
    EEPROM.put(0,calculated_crc);
  }
}

unsigned int get_eeprom_crc(int offset, size_t data_len) // gets crc of the items passewd
{
    unsigned int crc = 0;
    
    static const unsigned int crc_table[16] = {
    0x0000, 0xcc01, 0xd801, 0x1400, 0xf001, 0x3c00, 0x2800, 0xe401,
    0xa001, 0x6c00, 0x7800, 0xb401, 0x5000, 0x9c01, 0x8801, 0x4400
    };

    data_len = data_len + offset;

    for (int idx = offset; idx < data_len; ++idx){
      crc = crc_table[(crc ^ EEPROM[idx]) & 0x0f] ^ (crc >> 4);
      crc = crc_table[(crc ^ (EEPROM[idx] >> 4)) & 0x0f] ^ (crc >> 4);
    }


    return crc & 0xffff;
}

void get_scores(){
  check_crc();
  if(!read_from_eeprom) return;
  int address = 2;
  for(int i = 0; i < score_count; i++){
    EEPROM.get(address, scores[i]);
    address += sizeof(highscore);
  }
}

void write_scores(){
  int address = 2;
  highscore temp_score;
  for(int i = 0; i < score_count; i++){
    EEPROM.get(address, temp_score);
    if(temp_score.name != scores[i].name || temp_score.time != scores[i].time || temp_score.tries != scores[i].tries){
      EEPROM.put(address, scores[i]);
    }
    address+= sizeof(highscore);
  }
  set_crc();
}

void add_score(){
  strncpy(scores[9].name, name, 5);
  scores[9].tries = curr_round;
  scores[9].time = calc_time;

  sort_scores();
}
bool is_viable_score(){
  return (scores[9].time == 0 || calc_time < scores[9].time || curr_round < scores[9].tries);
}
bool is_score_empty(int idx){
  return scores[idx].name[0] == '\0' || scores[idx].name[0] == 0xFF; // If first byte of name is 0 or 255 we can count it as empty
}
void sort_scores(){
  for(int i = 0; i< score_count-1; i++){
    for(int j = i+1; j < score_count; j++){
      if(!is_score_empty(j)){
        if(is_score_empty(i) || scores[j].time < scores[i].time){
          highscore temp = scores[i];
          scores[i] = scores[j];
          scores[j] = temp;
        }
      }
    }
  }
}
void calculate_play_time(){
  calc_time = (play_time*31)/100; // convert to 100ms intervals, for game_timekeeping
}
void setup() {
  // put your setup code here, to run once:
  cli();
  TCCR1A = 0;  // set entire TCCR1A register to 0
  TCCR1B = 0;  // same for TCCR1B
  TCNT1 = 0;   //initialize counter value to 0
  // set compare match register for 1hz increments
  OCR1A = 625;  // = (16*10^6) / (1*1024) - 1 (must be <65536) | Currently triggers every second
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS10 and CS12 bits for 1024 prescaler
  TCCR1B |= (1 << CS12) | (1 << CS10);
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
  sei();

  Serial.begin(9600);  // for printing curr status;

  attachInterrupt(digitalPinToInterrupt(btn_pins[0]), btn1_interrupt, RISING);
  attachInterrupt(digitalPinToInterrupt(btn_pins[1]), btn2_interrupt, RISING);

  lcd.begin(16, 2);
  reset_lines();

  randomSeed(analogRead(A5));
  get_scores();
  sort_scores();
}


ISR(TIMER1_COMPA_vect) {  // Idea is to print to lcd on each interrupt, average time is 31ms for each interrupt soo.
  get_pots();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
  if(playing){
    play_time++;
  }
}

int debug_minor = -1;
int debug_major = -1;
void debug(){

  if(minor_mode != debug_minor || major_mode != debug_major){
    Serial.print("min ");
    Serial.print(minor_mode);
    Serial.print("\t\tmaj ");
    Serial.println(major_mode);

    debug_minor = minor_mode;
    debug_major = major_mode;
  
  }
  
}

void loop() {

  debug();
  switch(major_mode){
    case 0: // Welcome screen
      if(minor_mode == 0){ // Clear screen just in case
        reset_lines();
        minor_mode++;
      }
      if(minor_mode == 1){ // Set text then wait for a button press
        put_text(line1, "New game? btn1", 0);
        put_text(line2, "Highscores? btn2", 0);
        minor_mode++;
      }

      if(button_pressed[0]){
        button_pressed[0] = false;
        set_major_mode(1);
      }
      if(button_pressed[1]){
        button_pressed[1] = false;
        set_major_mode(2);
      }
    break;
    case 1: // Game screen

      if(minor_mode == 0){
        reset_lines();
        reset_game();
        minor_mode++;
      }
      if(minor_mode == 1){
        put_text(line1, "Next screen will", 0);
        put_text(line2, "show icon hints", 0);
      }
      if(minor_mode == 2){
        put_text(line1, "$=correct %=bad ", 0);
        put_text(line2, "position X=wrong", 0);
      }
      if(minor_mode == 3){
        put_text(line1, pot_string, 6);
      }
      if(minor_mode == 4){
        check_guess();
        if(correct == 4){
          minor_mode++;
          playing = false;
          reset_lines();
        } else if(curr_round < rounds){
          put_text(line2, last_guess, 0);
          // XXXX $:x %y X:z
          // 0123456789ABCDEF
          put_text(line2, "|$:",4);
          put_text(line2, "%:",9);
          put_text(line2, "X:",12);
          
          line2[7] = '0' + correct;
          line2[10] = '0' + wrong_place;
          line2[14] = '0' + wrong;

          minor_mode = 3;
        }
        else{
          minor_mode++;
          playing = false;
          reset_lines();
        }
      }
      if(minor_mode == 5){ // won or lost
        if(correct == 4){ // we won
          put_text(line1, "You won!",4);
        } else{ // lost
          put_text(line1, "You lost", 4);
        }
        minor_mode++;
      } if(minor_mode == 6){
        if(correct == 4){
          calculate_play_time();
          if(is_viable_score()){
            put_text(line2, "save score? btn1",0);
            minor_mode = 8; // go to add score screen
          } else{
            correct = 0; // else wait for button to leave game
          }
        }
      } if(minor_mode == 7){
        set_major_mode(0);
      } if(minor_mode == 8){
        // wait for button;
      } if(minor_mode == 9){
        set_major_mode(3); // create new score;
      }

      if(button_pressed[0]){
        minor_mode++;
        reset_lines();
        button_pressed[0] = false;
      }
      if(button_pressed[1]){
        if(strcmp(pot_string,"2212") == 0){ // Cheats >:)
          number[0] = 2; number[1] = 2; number[2] = 2; number[3] = 2;
        } else{
          set_major_mode(0);
          reset_lines();
        }
        button_pressed[1] = false;
      }

    break;

    case 2: // Score screen
      // if(is_score_empty(minor_mode)){
      //   set_major_mode(0);
      // } else{
      //   put_text()
      // }
      if(minor_mode == 0){
        scores_pos = 0;
        reset_lines();
        minor_mode++;
      }
      if(minor_mode == 1){
        if(is_score_empty(scores_pos)){
          minor_mode = 4;
        } else{
          put_text(line1, scores[scores_pos].name, 0);
          put_text(line1, " rounds: ", 5);
          char temp[5];
          sprintf(temp, "%d", scores[scores_pos].tries);
          put_text(line1,temp, 14);
          sprintf(line2,"time: %d.%d", scores[scores_pos].time/10, scores[scores_pos].time%10);
          minor_mode++;
        }
      }
      if(minor_mode == 2){
        // wait for button to do something
      }
      if(minor_mode == 3){
        scores_pos++;
        if(scores_pos >= score_count){
          minor_mode++;
        } else{
          minor_mode = 1;
        }
      }
      if(minor_mode == 4){
        //               0123456789ABCDEF
        put_text(line1, "No scores, press", 0);
        put_text(line2, "btn1/2 to exit",0);
        minor_mode++;
      }
      if(minor_mode == 5 ){

      } if(minor_mode == 6){
        set_major_mode(0);
      }


      if(button_pressed[0]){
        reset_lines();
        minor_mode++;
        button_pressed[0] = false;
      }
      if(button_pressed[1]){
        reset_lines();
        set_major_mode(0);
        button_pressed[1] = false;
      }
    break;

    case 3: // Name entry screen
      if(minor_mode == 0){
        //               0123456789ABCDEF
        put_text(line1, "change chr: btn1",0);
        put_text(line2, "move pos:   btn2",0);
        strncpy(name, "AAAAA", 5);
        minor_mode++;
      }
      if(minor_mode == 1){
        if(button_pressed[0] || button_pressed[1]){
          reset_lines();
          button_pressed[0] = false;
          button_pressed[1] = false;
          put_text(line1, "Name:",5);
          put_text(line2, name, 5);
          minor_mode++;
        }
      }
      if(minor_mode == 2){
        if(button_pressed[0]){
          name[name_pos] = name[name_pos] + 1;
          if(name[name_pos] > 90){
            name[name_pos] = 32;
          }
          put_text(line2, name, 5);
          button_pressed[0] = false;
        }
        if(button_pressed[1]){
          name_pos++;
          reset_lines();
          if(name_pos >= 5){
            minor_mode++;
            button_pressed[1] = false;
          }
          else{
            minor_mode = 1;
          }

        }
      }
      if(minor_mode == 3){

        Serial.println(name);
        Serial.println(curr_round);
        Serial.println(calc_time);
        add_score();
        write_scores();
        set_major_mode(0);
      }
    break;
  }
}

void reset_game(){
  play_time = 0;
  correct = 0;
  wrong_place = 0;
  wrong = 0;
  curr_round = 0;
  for(int i =0; i < 4; i++){
    number[i] = random(0,10);
  }
  playing = true;
}

void check_guess(){
  strncpy(last_guess, pot_string, 4);
  correct = 0;
  wrong_place = 0;
  wrong = 0;
  for(int i = 0; i < 4; i++){
    if(pot_vals[i] == number[i]){
      correct++;
    }
    else {
      int idx = index(number, 4, pot_vals[i]);
      if(idx != -1){
        if(pot_vals[i] != number[i]){
          wrong_place++;
        } else wrong++;
      } else wrong++;
    }
  }
  
  curr_round++;

}

int index(int arr[],int len, int num){
  for(int i = 0; i < len; i++){
    if(arr[i] == num){
      return i;
    }
  }
  return -1;
}

void set_major_mode(int mode){
  major_mode = mode;
  minor_mode = 0;
}

void get_pots() {  // Get values from potenciometers to an array
  for (int i = 0; i < 4; i++) {
    pot_vals[i] = map(analogRead(pot_pins[i]), 0, 1023, 0, 9);
    pot_string[i] = '0' + pot_vals[i];
  }
}

void btn1_interrupt() {
  unsigned long curr_time = millis();
  if (curr_time - btn_last_int_time[0] > debounce_time) {
    button_pressed[0] = true;
    Serial.println("BTN1_PRESSED");
  }
  btn_last_int_time[0] = curr_time;
}

void btn2_interrupt() {
  unsigned long curr_time = millis();
  if (curr_time - btn_last_int_time[1] > debounce_time) {
    button_pressed[1] = true;
    Serial.println("BTN2_PRESSED");
  }
  btn_last_int_time[1] = curr_time;
}