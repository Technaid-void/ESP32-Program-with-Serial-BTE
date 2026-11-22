//Int size = 2 bytes
//Bool size = 1 byte

//reminder arrays take pointers not the values

void init_eeprom(){
  int current_addr = 0;

  init_addr = current_addr;
  current_addr += char_size;

  // -- password --
  pass_addr = current_addr;
  current_addr += char_size * 5; // move by size

  // -- I/O --
  input_pins_addr = current_addr;
  current_addr += inputs_allowed * int_size;

  output_pins_addr = current_addr;
  current_addr += outputs_allowed * int_size;

  forward_pins_addr = current_addr;
  current_addr += forward_allowed * int_size;

  inverse_pins_addr = current_addr;
  current_addr += inverse_allowed* int_size;

  // -- time --
  timer_max_addr = current_addr;
  current_addr += timer_amount * int_size;

  amount_of_timer_addr = current_addr;
  current_addr += timer_amount * int_size;

  which_timer_action_addr = current_addr;
  current_addr += timer_amount * char_size * 5;

  timer_state_addr = current_addr;
  current_addr += timer_amount *char_size * 4;
  
  if (current_addr > EEPROM_SIZE) {
    while(true)
    {
      Serial.println("⚠️ EEPROM layout overflow!");
      delay(10000);
    }
  }
  while(!EEPROM.begin(EEPROM_SIZE)) {
    Serial.println("failed to initialize EEPROM");
    delay(5000);
  }
}

void only_get_unlimited_timers()
{
  for(int i = 0; i<timer_amount; i++)
    if(amount_of_timer[i] > 99)
      {
        amount_of_timer_saved[i] = amount_of_timer[i];
        for(int j=0; j<4;j++)
        {
          which_timer_action_saved[i][j] = which_timer_action[i][j];
        }
        which_timer_action_saved[i][4] = '\0';
      }
    else
    {
      amount_of_timer_saved[i] = 0;
      for(int j=0; j<4;j++)
        {
          which_timer_action_saved[i][j] = which_timer_action[i][j];
        }
        which_timer_action_saved[i][4] = '\0';
    }
}


void save_settings()
{
  only_get_unlimited_timers();
  yield();
  EEPROM.put(init_addr, init_value);
  yield();
  EEPROM.put(pass_addr, password);
  yield();
  EEPROM.put(input_pins_addr, input_pins);
  yield();
  EEPROM.put(output_pins_addr, output_pins);
  yield();
  EEPROM.put(forward_pins_addr, forward_pins);
  yield();
  EEPROM.put(inverse_pins_addr, inverse_pins);
  yield();
  EEPROM.put(timer_max_addr, timer_max);
  yield();
  EEPROM.put(amount_of_timer_addr, amount_of_timer_saved);
  yield();
  EEPROM.put(which_timer_action_addr, which_timer_action_saved);
  yield();
  EEPROM.put(timer_state_addr, timer_state);
  yield();
  EEPROM.commit();
  delay(100);
}

void load_settings()
{
  only_get_unlimited_timers();
  yield();
  EEPROM.get(pass_addr, password);
  yield();
  EEPROM.get(input_pins_addr, input_pins);
  yield();
  EEPROM.get(output_pins_addr, output_pins);
  yield();
  EEPROM.get(forward_pins_addr, forward_pins);
  yield();
  EEPROM.get(inverse_pins_addr, inverse_pins);
  yield();
  EEPROM.get(timer_max_addr, timer_max);
  yield();
  EEPROM.get(amount_of_timer_addr, amount_of_timer_saved);
  yield();
  EEPROM.get(which_timer_action_addr, which_timer_action_saved);
  yield();
  EEPROM.get(timer_state_addr, timer_state);
  yield();
}


void check_if_first_run()
{
  if (EEPROM.read(init_addr) != init_value){
    Serial.println("This is the first run");
    save_settings();
    Serial.println("Recorded eeprom");
  }
}

void display_eeprom_options()
{
  Serial.println(" bytes read from Flash . Values are:");
  for (int i = 0; i < EEPROM_SIZE; i++) {
    Serial.print((EEPROM.read(i)));
    Serial.print(" ");
    // Feed the watchdog every 32 iterations
    if ((i & 31) == 0) {
      yield();
    }
  }
  Serial.println();
}