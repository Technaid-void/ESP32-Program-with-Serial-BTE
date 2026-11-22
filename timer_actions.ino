void reduce_time(int which_timer)
{
  if(amount_of_timer[which_timer] > 0)
  {
    char current_timer_state = timer_state[which_timer][0];
    if(current_timer_state == 'N')
    {
      reduce_timer_reduct(which_timer);
    }
    else if(current_timer_state == 'P')
    {
      return;
    }
    else
    {
      char pin_specified[3];
      pin_specified[0] = timer_state[which_timer][1];
      pin_specified[1] = timer_state[which_timer][2];
      pin_specified[2] = '\0';

      int pin_specified_int = atoi(pin_specified);
      bool read = digitalRead(pin_specified_int);

      if(current_timer_state == 'F' && read)
      {
        reduce_timer_reduct(which_timer);
      }
      else if(current_timer_state == 'I' && !read)
      {
        reduce_timer_reduct(which_timer);
      }
      else
        return;

    }
  }
}

void reduce_timer_reduct(int which_timer)
{
  timer_val[which_timer] -= 1;
  if(timer_val[which_timer] <= 0)
  {
    do_timer_action(which_timer);
  }
}

void do_timer_action(int which_timer)
{
  if(amount_of_timer[which_timer] <=99)
    amount_of_timer[which_timer] -= 1;

  timer_val[which_timer] = timer_max[which_timer];
  const char *command = which_timer_action[which_timer];

  //if none then ignore
  if(strcmp(command, "none") == 0)
    return;

  //list of actions here
  //T is toggle
  //if after is a number then get the pin number
  if(strncmp(command, "TT", 2) == 0)
  {
    char tmp[5];
    if(timer_pin[which_timer])
    {
      timer_pin[which_timer] = false;
      snprintf(tmp, sizeof(tmp), "TF%c%c", command[2], command[3]);
    }
    else
    {
      timer_pin[which_timer] = true;
      snprintf(tmp, sizeof(tmp), "TN%c%c", command[2], command[3]);
    }
    tmp[4] = '\0';
    normal_operation(tmp);
    return;
  }

  normal_operation(command);
}

void change_timer_state(int which_timer, char new_state[])
{
  timer_state[which_timer][0] = new_state[0];
  timer_state[which_timer][1] = new_state[1];
  timer_state[which_timer][2] = new_state[2];
  timer_state[which_timer][3] = '\0';
}