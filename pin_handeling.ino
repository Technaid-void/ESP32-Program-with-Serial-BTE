void setPin_out(int pin, int state)
{
  bool free_space = false;
  bool already_set = false;

  //check if pin is an input already, if so free it
  for(int i =0; i<inputs_allowed; i++)
    if(input_pins[i] == pin)
      free_input[i];
  for(int i = 0; i<outputs_allowed; i++){
    if(output_pins[i] == pin){
      already_set = true;
      break;
    }
  }

  if(!already_set){
    //add it to the array
    for(int i = 0; i<outputs_allowed; i++)
    {
      if(output_pins[i] == -1)
      {
        output_pins[i] = pin;
        free_space = true;
        break;
      }
    }
    //if there's no free space to save then it won't
    if(!free_space)
    {
      SerialBT.printf("No more output space left, please free up a pin first");
      return;
    }
  }
  pinMode(pin, OUTPUT);
  digitalWrite(pin, state);
}


void setPin_in(int pin)
{
  bool free_space = false;
  bool already_set = false;

  //check if pin is an output already, if so free it
  for(int i = 0; i<outputs_allowed; i++)
    if(output_pins[i] == pin)
      free_output[i];
  for(int i = 0; i<inputs_allowed; i++){
    if(input_pins[i] == pin){
      already_set = true;
      break;
    }
  }

  if(!already_set){
    for(int i = 0; i<inputs_allowed; i++)
    {
      if(input_pins[i] == -1)
      {
        input_pins[i] = pin;
        free_space = true;
        break;
      }
    }

    if(!free_space)
    {
      SerialBT.printf("No more input space left, please free up a pin first");
      return;
    }
  }
  pinMode(pin, INPUT);
}

void set_forward(int inpin, int outpin)
{
  bool free_space = false;
  for(int i =0; i<forward_allowed; i++)
  {
    if(forward_pins[i] == 0)
    {
      inpin = inpin * 100;
      int pin = inpin + outpin;
      forward_pins[i] = pin;
      free_space = true;
      break;
    }
  }
  if(!free_space)
    return;
}

void set_inverse(int inpin, int outpin)
{
  bool free_space = false;
  for(int i =0; i<inverse_allowed; i++)
  {
    if(inverse_pins[i] == 0)
    {
      inpin = inpin * 100;
      int pin = inpin + outpin;
      inverse_pins[i] = pin;
      free_space = true;
      break;
    }
  }
  if(!free_space)
    return;
}

void reset_inputs()
{
  for(int i = 0; i<inputs_allowed; i++)
  {
    input_pins[i] = -1;
  }
}

void reset_outputs()
{
  for(int i=0; i<outputs_allowed; i++)
  {
    output_pins[i] = -1;
  }
}

void reset_forward()
{
  for(int i =0; i<forward_allowed; i++)
  {
    forward_pins[i] = 0;
  }
}

void reset_inverse()
{
  for(int i =0; i<inverse_allowed; i++)
  {
    inverse_pins[i] = 0;
  }
}

void reset_Allpins()
{
  reset_inputs();
  reset_outputs();
  reset_forward();
  reset_inverse();
}

void free_input(int index)
{
  //if it's not in the array then it's not going to be referenced anyway
  input_pins[index] = -1;
}

void free_output(int index)
{
  //input consumes less power
  pinMode(output_pins[index], INPUT);
  output_pins[index] = -1;
}

void free_input_pin(int pin)
{
  for(int i=0; i<inputs_allowed; i++)
  {
    if(input_pins[i] == pin)
    {
      free_input(i);
    }
  }
}

void free_output_pin(int pin)
{
  for(int i =0; i<outputs_allowed; i++)
  {
    if(output_pins[i] == pin)
    {
      free_output(i);
    }
  }
}

void free_pin(int pin)
{
  free_input_pin(pin);
  free_output_pin(pin);
}
