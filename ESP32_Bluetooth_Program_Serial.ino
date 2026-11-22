//ESP32
#include "BluetoothSerial.h"

#include "EEPROM.h"

#include "esp_timer.h"

#include <cstring>

#define int_size sizeof(int)
#define char_size sizeof(char)

// Check if Bluetooth is available
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

// Check Serial Port Profile
#if !defined(CONFIG_BT_SPP_ENABLED)
#error Serial Port Profile for Bluetooth is not available or not enabled. It is only available for the ESP32 chip.
#endif

//------------------------------------------------------------------------------------------------------
const char init_value = 'c';
bool waiting_for_password = true;

int main_page = 0;
int current_state = main_page;
//default password
char password[5] = "1234";
BluetoothSerial SerialBT;

//------------------------------------------------------------------------------------------------------
//timer stuff
esp_timer_handle_t my_timer;
void onTimer(void* arg) {
 //Reduce the timer for every enabled timer
  for(int i =0; i<4; i++)
    reduce_time(i);  
}
const int timer_amount = 4;


//100 or higher amount means infinite, since personal set is 99
volatile int timer_val[timer_amount]       = {0, 0, 0, 0}; //current value
volatile int timer_max[timer_amount]       = {0, 0, 0, 0}; //maximum value
volatile int amount_of_timer[timer_amount] = {0, 0, 0, 0}; //how many times each one runs
volatile int amount_of_timer_saved[timer_amount] = {0, 0, 0, 0}; //this is to save, don't save limited ones
volatile bool timer_pin[timer_amount]      = {false, false, false, false};  //if it's off or on right now
volatile char timer_state[timer_amount][4] = {"N00", "N00", "N00", "N00"};        //F is forward (turn on timer when input is high), I is inverse (turn on timer when input is low), N is always on, P is always off, number after it is which pin

//none = nothing
//TNxx = turn on pin xx
//TFxx = turn off pin xx
//TTxx = toggle pin xx
char which_timer_action[timer_amount][5] = {"none", "none", "none", "none"};
char which_timer_action_saved[timer_amount][5] = {"none", "none", "none", "none"}; //only save the ones for unlimited timers
//------------------------------------------------------------------------------------------------------
#define inputs_allowed 10
#define outputs_allowed 10
//-1 means empty
int input_pins[inputs_allowed] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
int output_pins[outputs_allowed] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

//input will turn on output whether forward or inverse
#define forward_allowed 10
#define inverse_allowed 10
int forward_pins[forward_allowed] = {0,0,0,0,0,0,0,0,0,0};
int inverse_pins[inverse_allowed] = {0,0,0,0,0,0,0,0,0,0};
//------------------------------------------------------------------------------------------------------
#define EEPROM_SIZE 1024
int init_addr, pass_addr, input_pins_addr, output_pins_addr, forward_pins_addr, inverse_pins_addr,timer_max_addr, amount_of_timer_addr, which_timer_action_addr, timer_state_addr;
//------------------------------------------------------------------------------------------------------
void setup() {

  Serial.begin(115200);
  Serial.println("Starting bluetooth");
  // Start Bluetooth with a device name
  if (!SerialBT.begin("MyESP32")) {
    Serial.println("Bluetooth failed to start");
    esp_restart();
  }
  Serial.println("Bluetooth started");
  delay(200);

  Serial.println("Reading eeprom");
  //try to start eeprom
  init_eeprom();
  delay(100);
  Serial.println("Read eeprom successfully");
  check_if_first_run();
  Serial.println("If first run checked");
  delay(100);
  Serial.println("Loading settings");
  load_settings();
  Serial.println("Settings loaded");
  delay(100);
  yield();
  //read the eeprom values, for testing
  //leading to crashes since it's too big rn
  //display_eeprom_options();
  //delay(100);


  Serial.println("Starting timer");
  const esp_timer_create_args_t timer_args = {
    .callback = &onTimer,
    .arg = NULL,
    .dispatch_method = ESP_TIMER_TASK,
    .name = "periodic_timer"
  };
  
  esp_timer_create(&timer_args, &my_timer);
  esp_timer_start_periodic(my_timer, 1000000); // every 1 second

  Serial.println("Timer started");
  Serial.println("Board ready to be used");

}

void loop() {

  //Receive code from phone and display it
  if (SerialBT.available()) {
    //This is the data received
    String new_bt_line = SerialBT.readString();
    new_bt_line.trim();
    Serial.print(new_bt_line);

    if(waiting_for_password)
    { 
      if(new_bt_line == password)
      {
        waiting_for_password = false;
        Serial.println("Access Accepted");
        SerialBT.println("Password Correct");
      }
      else
      {
        Serial.println("Access Denied");
        SerialBT.println("Wrong Password; Try Again.");
      }
    }
    else
      normal_operation(new_bt_line.c_str());
  }
  do_looped_action();
}

void do_looped_action()
{
  for(int i =0; i<forward_allowed; i++)
  {
    int pin_in = forward_pins[i]/100;
    int pin_out = forward_pins[i]%100;

    digitalWrite(pin_out, digitalRead(pin_in));
  }
  for(int i =0; i<inverse_allowed; i++)
  {
    int pin_in = inverse_pins[i]/100;
    int pin_out = inverse_pins[i]%100;

    digitalWrite(pin_out, !digitalRead(pin_in));
  }
}

void normal_operation(const char *input){
  switch(input[0])
  {
    //set a pin as input
    case 'I': {
      //put the error check on the app
      int pin = atoi(&input[1]);
      setPin_in(pin);
      SerialBT.printf("Changed pin %d to input \n", pin);
      break;
    }
    //set a pin as output
    case 'O':{
      int pin = atoi(&input[1]);
      setPin_out(pin, LOW);
      SerialBT.printf("Changed pin %d to output \n", pin);
      break;
    }

    //set a pin as output and then output
    case 'T': {
      char action_buffer = input[1];
      char pinnum_buffer[3];

      strncpy(pinnum_buffer, input+2, 2);
      pinnum_buffer[2] = '\0';

      int pin_affected = atoi(pinnum_buffer);
      //TN is auto turn on
      if(action_buffer == 'N')
      {
        setPin_out(pin_affected, HIGH);
        SerialBT.printf("Turned on pin %d \n", pin_affected);
        break;
      }

      //TF is auto turn off
      else if(action_buffer == 'F')
      {
        setPin_out(pin_affected, LOW);
        SerialBT.printf("Turned off pin %d \n", pin_affected);
        break;
      }
      break;
    }

    //start a timer
    case 'S': {
      //full format is
      //S 1 00000000 none 100
      //timernumber, maxnumber, action, amount(default infinite)
      //max number is 00,000,000 format that means a maximum of around 27777 hours max (8 digits)
      char timer_buffer[2];
      strncpy(timer_buffer, input+1,1);
      timer_buffer[1] = '\0';
      int timer_specified = atoi(timer_buffer);

      char buffer[9];
      strncpy(buffer, input + 2, 8);
      buffer[8] = '\0';
      int max_time = atoi(buffer);

      char action_chosen[5];
      strncpy(action_chosen, input + 10, 4);
      action_chosen[4] = '\0';
      
      char buffer_2[4];
      strncpy(buffer_2, input + 14, 3);
      buffer_2[3] = '\0';
      int amount_specified = atoi(buffer_2);

      Serial.printf("Timer specified: %d \nMax Time: %d \nAction Chosen: %s \nAmount Specified: %d\n", timer_specified, max_time, action_chosen, amount_specified);
      SerialBT.printf("Timer specified: %d \nMax Time: %d \nAction Chosen: %s \nAmount Specified: %d\n", timer_specified, max_time, action_chosen, amount_specified);

      timer_max[timer_specified] = max_time;
      amount_of_timer[timer_specified] = amount_specified;
      strncpy(which_timer_action[timer_specified], action_chosen, 4);
      which_timer_action[timer_specified][4] = '\0';

      if(amount_specified > 99)
        save_settings();

      break;
    }
    //Display all pins, if a pin is specified then free it up
    case 'F': {
      char buffer[3];
      strncpy(buffer, input+1, 2);
      buffer[2] = '\0';

      char secondbuffer;
      secondbuffer = buffer[0];


      int pin_specified = atoi(buffer);

      if( (pin_specified == NULL || pin_specified == 0) && secondbuffer != '0')
      {
        char input_list[3*inputs_allowed + 1] = "";
        char output_list[3*outputs_allowed + 1] = "";

        for(int i =0; i<inputs_allowed; i++)
        {
          if(input_pins[i] != -1)
            {
              char temp[4];
              sprintf(temp, "%d\n", input_pins[i]);
              strcat(input_list, temp);
            }
        }

        for(int i =0; i<outputs_allowed; i++)
        {
          if(output_pins[i] != -1)
            {
              char temp[4];
              sprintf(temp, "%d\n", output_pins[i]);
              strcat(output_list, temp);
            }
        }

        char List_of_pins_occupied[(40)+(3*inputs_allowed + 1) + (3*outputs_allowed + 1)];

        sprintf(List_of_pins_occupied, "%s%s%s%s%s","Inputs Occupied:\n", input_list, "Outputs Occupied:\n", output_list, "That's all Assigned Pins. \n");
        SerialBT.println(List_of_pins_occupied);
      }
      else
      {
        free_pin(pin_specified);
        SerialBT.printf("Pin %d freed\n", pin_specified);
      }
      break;
    }
    //disconnect program
    case 'D':{
      char input_buffer[3];
      char output_buffer[3];

      strncpy(input_buffer, input+1, 2);
      strncpy(output_buffer, input+3, 2);

      input_buffer[2] = '\0';
      output_buffer[2] = '\0';

      int inpin_p = atoi(input_buffer);
      int outpin_p = atoi(output_buffer);

      if(inpin_p == NULL || outpin_p == NULL || (inpin_p == 0 && outpin_p == 0))
      {
        char forward_list[5*inputs_allowed + 1] = "";
        char inverse_list[5*outputs_allowed + 1] = "";
        for(int i =0; i<forward_allowed; i++)
        {
          if(forward_pins[i] != 0)
          {
            char temp[6];
            sprintf(temp, "%d\n", forward_pins[i]);
            strcat(forward_list, temp);
          }
        }
        for(int i=0; i<inverse_allowed; i++)
        {
            char temp[6];
            sprintf(temp, "%d\n", inverse_pins[i]);
            strcat(inverse_list, temp);
        }
        char List_of_connections_occupied[(40)+(5*forward_allowed + 1) + (5*inverse_allowed + 1)];
        sprintf(List_of_connections_occupied, "%s%s%s%s","Forward Occupied:\n", forward_list, "Inverse Occupied:\n", inverse_list);
        SerialBT.println(List_of_connections_occupied);
      }
      else
      {
        for(int i=0; i<forward_allowed; i++)
        {
          if(forward_pins[i]/100 == inpin_p && forward_pins[i]%100 == outpin_p)
          {
            forward_pins[i] = 0;
          }
        }
        for(int i=0; i<inverse_allowed; i++)
        {
          if(inverse_pins[i]/100 == inpin_p && inverse_pins[i]%100 == outpin_p)
          {
            inverse_pins[i] = 0;
          }
        }
      }
      break;
    }

    //program something to happen when it reads an input
    case 'P':{
      char command = input[1];
      //affect an output
      if(command == 'O')
      {
        //format is POIIOO(F/I) without ()
        //F is forward (turn on output when input is high), I is inverse (turn on output when input is low)
        char input_buffer[3];
        char output_buffer[3];
        char state_buffer[2];

        strncpy(input_buffer, input+2, 2);
        strncpy(output_buffer, input+4, 2);
        strncpy(state_buffer, input+6, 1);

        input_buffer[2] = '\0';
        output_buffer[2] = '\0';
        state_buffer[1] = '\0';

        int inpin_p = atoi(input_buffer);
        int outpin_p = atoi(output_buffer);
        char action = state_buffer[0];

        if(action == 'F')
        {
          set_forward(inpin_p, outpin_p);
          SerialBT.printf("Programmed %d in forward mode according to %d\n", outpin_p, inpin_p);
        }
        else if(action == 'I')
        {
          set_inverse(inpin_p, outpin_p);
          SerialBT.printf("Programmed %d in inverse mode according to %d\n", outpin_p, inpin_p);
        }
      }
      //affect a timer
      else if(command == 'T')
      {
        //format is PTXII(F/I/N/P) without ()
        //F is forward (turn on timer when input is high)
        //I is inverse (turn on timer when input is low)
        //N is always on
        //P is always off
        //XX is the pin 

        char which_timer_buffer[2];
        char which_input_buffer[3];
        char direction_buffer[2];

        strncpy(which_timer_buffer, input+2, 1);
        strncpy(which_input_buffer, input+3, 2);
        strncpy(direction_buffer, input+5,1);

        which_timer_buffer[1] = '\0';
        which_input_buffer[2] = '\0';
        direction_buffer[1] = '\0';

        int timer_p = atoi(which_timer_buffer);
        int which_pin = atoi(which_input_buffer);
        char state_char_buffer[4];
        state_char_buffer[0] = direction_buffer[0];
        state_char_buffer[1] = which_input_buffer[0];
        state_char_buffer[2] = which_input_buffer[1];
        state_char_buffer[3] = '\0';

        change_timer_state(timer_p, state_char_buffer);
        SerialBT.printf("Programmed timer %d in accordance to pin %d as %c mode\n", timer_p, which_pin, state_char_buffer[0]);

      }

      break;
    }
    //display timers info
    case 'C':
    {
      for(int i =0; i<timer_amount; i++)
      {
        /*
        //none = nothing
        //TNxx = turn on pin xx
        //TFxx = turn off pin xx
        //TOxx = toggle pin xx
        char which_timer_action[timer_amount][5] = {"none", "none", "none", "none"};
        char which_timer_action_saved[timer_amount][5] = {"none", "none", "none", "none"}; //only save the ones for unlimited timers*/

        //max time
        int hours=0;
        int minutes=0;
        int seconds=timer_max[i];

        while(seconds>60)
        {
          minutes = minutes + 1;
          seconds = seconds - 60;
        }

        while(minutes>60)
        {
          hours = hours +1;
          minutes = minutes - 60;
        }


        SerialBT.printf("Timer %d:\n ", i);
        SerialBT.printf("Max time: ");
        if(hours>0)
          Serial.printf("%dh%dm%ds\n ", hours, minutes, seconds);
        else if(minutes>60)
          SerialBT.printf("%dm%ds\n ", minutes, seconds);
        else
          SerialBT.printf("%ds\n ", seconds);

        //current time
        hours=0;
        minutes=0;
        seconds=timer_val[i];

        while(seconds>60)
        {
          minutes = minutes + 1;
          seconds = seconds - 60;
        }

        while(minutes>60)
        {
          hours = hours +1;
          minutes = minutes - 60;
        }

        SerialBT.printf("Current time: ");
        if(hours>0)
          SerialBT.printf("%dh%dm%ds\n ", hours, minutes, seconds);
        else if(minutes>60)
          SerialBT.printf("%dm%ds\n ", minutes, seconds);
        else
          SerialBT.printf("%ds\n ", seconds);

        int run_amount = amount_of_timer[i];
        if(run_amount <100)
          SerialBT.printf("Will run for %d more times\n ", run_amount);
        else
        {
          SerialBT.printf("Will run indefinetly (save able timer)\n ");
        }

        if(timer_pin[i])
          SerialBT.printf("Timer pin is on\n ");
        else
          SerialBT.printf("Timer pin is off\n ");

        SerialBT.printf("Timer is ");
        char state = timer_state[i][0];
        switch(state)
        {
          case 'N':
          {
            SerialBT.printf("running normally\n ");
            break;
          }
          case 'P':
          {
            SerialBT.printf("paused\n ");
            break;
          }
          case 'F':
          {
            SerialBT.printf("only runs when %c%c is high", timer_state[i][1], timer_state[i][2]);

            char pin_specified_f[3];
            pin_specified_f[0] = timer_state[i][1];
            pin_specified_f[1] = timer_state[i][2];
            pin_specified_f[2] = '\0';

            int pin_specified_int_f = atoi(pin_specified_f);
            bool read_f = digitalRead(pin_specified_int_f);

            if(read_f)
              SerialBT.printf("(timer currently on)\n ");
            else
              SerialBT.printf("(timer currently off)\n ");
            break;
          }
          
          case 'I':
          {
            Serial.printf("only runs when %c%c is low", timer_state[i][1], timer_state[i][2]);

            char pin_specified_i[3];
            pin_specified_i[0] = timer_state[i][1];
            pin_specified_i[1] = timer_state[i][2];
            pin_specified_i[2] = '\0';

            int pin_specified_int_i = atoi(pin_specified_i);
            bool read_i = digitalRead(pin_specified_int_i);

            if(!read_i)
              SerialBT.printf("(timer currently on)\n ");
            else
              SerialBT.printf("(timer currently off)\n ");

            break;
          }


          default:
            SerialBT.printf("in some unknown mode\n ");
        }

        char pin_c[3];
        pin_c[0] = which_timer_action[i][2];
        pin_c[1] = which_timer_action[i][3];
        pin_c[2] = '\0';
        int pin = atoi(pin_c);
        if(strcmp(which_timer_action[i],"none") == 0)
          SerialBT.printf("Does nothing\n");
        else if(strncmp(which_timer_action[i],"TN", 2) == 0)
          SerialBT.printf("Turns on pin %d\n", pin);
        else if(strncmp(which_timer_action[i],"TF", 2) == 0)
          SerialBT.printf("Turns off pin %d\n", pin);
        else if(strncmp(which_timer_action[i],"TT", 2) == 0)
          SerialBT.printf("Toggle pin %d\n", pin);
        else
          SerialBT.printf("Uknown char, does nothing\n");

      }
      break;
    }

    case 'W':
    {
      save_settings();

      SerialBT.println("Saved settings");
      break;
    }

    case 'L':
    {
      load_settings();
      SerialBT.println("Loaded settings");
      break;
    }

    case 'Q':
    {
      waiting_for_password = true;
      SerialBT.println("ESP locked up");
      delay(50);
      restartbluetooth();
      break;
    }

    case 'Z':
    {
      strncpy(password, input+1, 4);
      password[4] = '\0';
      SerialBT.printf("Password changed to %s, don't forget to write W to save changes\n",password);
      break;
    }

    case 'H':
    {
      const char *list_of_commands =
      R"rawliteral(
Commands:
Large letter means fixed command
Small letter means changes depending on action

Oxx   - Set pin XX as OUTPUT

Ixx   - Set pin XX as INPUT

TNxx  - Turn ON output xx

TFxx  - Turn OFF output xx

StssssssssTaxxfff - 
  Set timer t (0-3) to trigger every s seconds (8 digits)
  Doing action T(N/F/T) (N: turn on) (F: turn off) (T: toggle)
  On pin xx
  fff times (anything above 99 is infinite)

F     - List all occupied inputs and outputs

Fxx   - Free up pin xx from occupation (make it no longer input nor output)

C     - Display all timers info

Diioo - Disconnect the input pin ii from the output pin oo

POiioo(F/I) - Program a pin to output depending on an input 
  (F: Forward, turn on when reading 5v)
  (I: Inverse, turn off when reading 5v)

PTtii(F/I/N/P) - Program timer X to work depending on an input 
  (F: Forward, start timer when reading 5v)
  (I: Inverse, stop timer when reading 5v)
  (N: On, timer is always on)
  (P: Off, timer is always off)


W     - Write current settings (limited loops not included) and save changes

L     - Load settings from last saved

Q     - Lock the esp board to ask for pass

Zxxxx     - Change password to xxxx
      )rawliteral";
      SerialBT.println(list_of_commands);
      break;
    }

    default:
    {
      SerialBT.println("Command not available, type H to see command list. Operations are allowed");
      break;
    }

  }
}