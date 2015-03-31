#include <avr/io.h>
#include <avr/interrupt.h>
#include <ucr/bit.h>
#include <ucr/timer.h>
#include <stdio.h>
#include <ucr/io.c>
//--------Find GCD function --------------------------------------------------
unsigned long int findGCD(unsigned long int a, unsigned long int b)
{
    unsigned long int c;
    while(1){
        c = a%b;
        if(c==0){return b;}
        a = b;
b = c;
    }
    return 0;
}
//--------End find GCD function ----------------------------------------------

//--------Task scheduler data structure---------------------------------------
// Struct for Tasks represent a running process in our simple real-time operating system.
typedef struct _task {
    /*Tasks should have members that include: state, period,
        a measurement of elapsed time, and a function pointer.*/
    signed char state; //Task's current state
    unsigned long int period; //Task period
    unsigned long int elapsedTime; //Time elapsed since last task tick
    int (*TickFct)(int); //Task tick function
} task;

//--------End Task scheduler data structure-----------------------------------

//--------Shared Variables----------------------------------------------------
    const unsigned char ship_l[8] = {
	    0x1C,
	    0x0F,
	    0x0F,
	    0x1C,
	    0x00,
	    0x00,
	    0x00,
	    0x00
    };
    const unsigned char ship_r[8] = {
	    0x00,
	    0x00,
	    0x00,
	    0x00,
	    0x1C,
	    0x0F,
	    0x0F,
	    0x1C
    };
    const unsigned char enemy_l[8] = {
	    0x0B,
	    0x1C,
	    0x1C,
	    0x0B,
	    0x00,
	    0x00,
	    0x00,
	    0x00
    };
    const unsigned char enemy_r[8] = {
	    0x00,
	    0x00,
	    0x00,
	    0x00,
	    0x0B,
	    0x1C,
	    0x1C,
	    0x0B
    };
    const unsigned char shield_left[8] = {
	    0x18,
	    0x0C,
	    0x0E,
	    0x0E,
	    0x06,
	    0x17,
	    0x1F,
	    0x0F
    };
    const unsigned char shield_right[8] = {
	    0x0F,
	    0x1F,
	    0x17,
	    0x06,
	    0x0E,
	    0x0E,
	    0x0C,
	    0x18
    };
    const unsigned char bullet_left[8] = {
	    0x00,
	    0x1F,
	    0x1F,
	    0x00,
	    0x00,
	    0x00,
	    0x00,
	    0x00
    };
    const unsigned char bullet_right[8] = {
	    0x00,
	    0x00,
	    0x00,
	    0x00,
	    0x00,
	    0x1F,
	    0x1F,
	    0x00
    };
	const unsigned char ammo_left[8] = {
		0x0E,
		0x1B,
		0x1B,
		0x0E,
		0x00,
		0x00,
		0x00,
		0x00
	};
	const unsigned char ammo_right[8] = {
		0x00,
		0x00,
		0x00,
		0x00,
		0x0E,
		0x1B,
		0x1B,
		0x0E
	};
	
	unsigned char game_over;

//--------End Shared Variables------------------------------------------------
//--------User defined FSMs---------------------------------------------------
unsigned tmp_val1;
unsigned tmp_val2;
unsigned char loop_cntr;
//C0 = Data
//C1 = Latch
//C2 = CLK
enum snes_ctrl {initial, data_push, l_pulse, latch_off, clk_pulse, clk_off, reset} state_ctrl;

void ctrl_logic(){
	switch (state_ctrl)
	{
		case -1:
		  state_ctrl = initial;
		  break;
		case initial:
		  PORTC = SetBit(PORTC, 1, 0);//latch
		  PORTC = SetBit(PORTC, 2, 0);//clk
		  tmp_val1 = 0;
		  tmp_val2 = 0;
		  loop_cntr = 0;
		  state_ctrl = l_pulse;
		  break;
		case l_pulse:
		  PORTC = SetBit(PORTC, 1, 1);
		  state_ctrl = latch_off;
		  break;
		case latch_off:
		  PORTC = SetBit(PORTC, 1, 0);
		  state_ctrl = data_push;
		  break;
		  case data_push:
		  if(loop_cntr < 12){
			if(loop_cntr < 8){
				tmp_val1 = SetBit(tmp_val1 , loop_cntr,GetBit(~PINC, 0));
				//PORTB = SetBit(PORTB,0,GetBit(~PINC,0));
			}
			else if(loop_cntr < 12){
				tmp_val2 = SetBit(tmp_val2, loop_cntr-8,GetBit(~PINC, 0));
			}
			++loop_cntr;
			state_ctrl = clk_pulse;
		  }
		  else{
			state_ctrl = reset;
		  }
		  break;
		case clk_pulse:
		  PORTC = SetBit(PORTC, 2, 1);
		  state_ctrl = clk_off;
		  break;
		case clk_off:
		  PORTC = SetBit(PORTC, 2, 0);
		  state_ctrl = data_push;
		  break;
		case reset:
		  PORTC = SetBit(PORTC, 2, 0);//clk
		  PORTC = SetBit(PORTC, 1, 0);//latch
		  state_ctrl = initial;
		  break;
		default:
		  state_ctrl = initial;
		  break;
	}
}
/////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////
unsigned char player_position;
unsigned char horizontal_pos = 2;
enum ship_movement {left_left, left_right, right_left, right_right } curr_position;
void player_movement(){
	switch(curr_position){
		case -1:
		  curr_position = left_left;
		break;
	    case left_left: //position 0
		if (GetBit(tmp_val1, 5)){
			LCD_Build(0, ship_r);
			LCD_Cursor(horizontal_pos);
			LCD_WriteData(0x00);
			LCD_Cursor(0);
			player_position = 1;
			curr_position = left_right;
		}
		else{
			LCD_Build(0, ship_l);
			LCD_Cursor(horizontal_pos);
			LCD_WriteData(0x00);
			LCD_Cursor(0);
			player_position = 0;
			curr_position = left_left;
		}
		break;
		case left_right: //position 1
		if (GetBit(tmp_val1, 5)){
			LCD_Build(0, ship_l);
			LCD_Cursor(horizontal_pos);
			LCD_WriteData(' ');
			LCD_Cursor(horizontal_pos+16);
			LCD_WriteData(0x00);
			LCD_Cursor(0);
			player_position = 2;
			curr_position = right_left;
		}
		else if (GetBit(tmp_val1, 4)){
			LCD_Build(0, ship_l);
			LCD_Cursor(horizontal_pos);
			LCD_WriteData(0x00);
			LCD_Cursor(0);
			player_position = 0;
			curr_position = left_left;
		}
		else{
			LCD_Build(0, ship_r);
			LCD_Cursor(horizontal_pos);
			LCD_WriteData(0x00);
			LCD_Cursor(0);
			player_position = 1;
			curr_position = left_right;
		}
		break;
		case right_left: //position 2
		if (GetBit(tmp_val1, 5)){
			LCD_Build(0, ship_r);
			LCD_Cursor(horizontal_pos+16);
			LCD_WriteData(0x00);
			LCD_Cursor(0);
			player_position = 3;
			curr_position = right_right;
		}
		else if (GetBit(tmp_val1, 4)){
			LCD_Build(0, ship_r);
			LCD_Cursor(horizontal_pos+16);
			LCD_WriteData(' ');
			LCD_Cursor(horizontal_pos);
			LCD_WriteData(0x00);
			LCD_Cursor(0);
			player_position = 1;
			curr_position = left_right;
		}
		else{
			LCD_Build(0, ship_l);
			LCD_Cursor(horizontal_pos+16);
			LCD_WriteData(0x00);
			LCD_Cursor(0);
			player_position = 2;
			curr_position = right_left;
		}
		break;
		case right_right: //position 3
		if (GetBit(tmp_val1, 4)){
			LCD_Build(0, ship_l);
			LCD_Cursor(horizontal_pos+16);
			LCD_WriteData(0x00);
			LCD_Cursor(0);
			player_position = 2;
			curr_position = right_left;
		}
		else{
			LCD_Build(0, ship_r);
			LCD_Cursor(horizontal_pos+16);
			LCD_WriteData(0x00);
			LCD_Cursor(0);
			player_position = 3;
			curr_position = right_right;
		}
		break;
		default:
		  curr_position = left_left;
		  break;
	}
};

enum ship_horiz_movement{action} horiz_pos;
void player_hoizontal(){
  switch(horiz_pos){
	  case -1:
	    horiz_pos = action;
		break;
	  case action:
		if(GetBit(tmp_val1, 6)){
	      if(horizontal_pos > 2 || (horizontal_pos+16) > 19){
		    LCD_Cursor(horizontal_pos);
			LCD_WriteData(' ');
			LCD_Cursor(horizontal_pos+16);
			LCD_WriteData(' ');
			LCD_Cursor(0);
			--horizontal_pos;
		  }
		}
		else if(GetBit(tmp_val1, 7)){
		  if (horizontal_pos < 15 || (horizontal_pos+16) < 30){
			LCD_Cursor(horizontal_pos);
			LCD_WriteData(' ');
			LCD_Cursor(horizontal_pos+16);
			LCD_WriteData(' ');
			LCD_Cursor(0);
		    ++horizontal_pos;
		  }
		}
		horiz_pos = action;
		break;
	  default:
	    horiz_pos = action;
	    break;
  }	
};

unsigned char position;
unsigned char enemy_location;
enum enemy_movement {position_spawn, forward_movement, despawn} enemy_position;
void enemy_ai(){
	switch(enemy_position){
		case -1:
		  enemy_position = position_spawn;
		  break;
		case position_spawn:
		  position = rand() % 4;
		  if (position == 0)
		  {
			LCD_Build(2, enemy_l);
			enemy_location = 15;
			LCD_Cursor(enemy_location);
			LCD_WriteData(0x02);
		  }
		  else if (position == 1)
		  {
			LCD_Build(2, enemy_r);
			enemy_location = 15;
			LCD_Cursor(enemy_location);
			LCD_WriteData(0x02);
		  }
		  else if (position == 2)
	 	  {
			LCD_Build(2, enemy_l);
			enemy_location = 31;
			LCD_Cursor(enemy_location);
			LCD_WriteData(0x02);
		  }
		  else{
			LCD_Build(2, enemy_r);
			enemy_location = 31;
			LCD_Cursor(enemy_location);
			LCD_WriteData(0x02);
		  }
		  LCD_Cursor(0);
		  enemy_position = forward_movement;
		  break;
		case forward_movement:
		  --enemy_location;
		  LCD_Cursor(enemy_location+1);
		  LCD_WriteData(' ');
		  LCD_Cursor(enemy_location);
		  LCD_WriteData(0x02);
		  LCD_Cursor(0);
		  if(enemy_location == 0 || enemy_location == 17){
			enemy_position = despawn;
		  }
		  else{
			enemy_position = forward_movement;
		  }
		  break;
		case despawn:
		  LCD_Cursor(enemy_location);
		  LCD_WriteData(' ');
		  LCD_Cursor(0);
		  enemy_position = position_spawn;
		  break;
		default:
		  enemy_position = position_spawn;
	}
};

unsigned char bullet_horizontal_position;
unsigned char bullet_location;
unsigned char shot_fired;
unsigned char num_shots;
enum bullet_shoot{create_bullet, forward_bullet_1, forward_bullet_2, despawn_bullet, stall_bullet, no_ammo} bullet_position;
void bullet_logic(){
  switch(bullet_position){
	  case -1:
	    bullet_position = create_bullet;
		break;
	  case create_bullet:
	    shot_fired = 0;
		if(num_shots == 0){
			bullet_position = no_ammo;
		}
	    if(GetBit(tmp_val1,0)){
			shot_fired = 1;
			if (num_shots > 0){
			  num_shots--;
			}
			bullet_horizontal_position = horizontal_pos+1;
			if(player_position < 2){
				if(player_position == 0){
					LCD_Build(1, bullet_left);
					bullet_location = 0;
				}
				else if (player_position == 1){
					LCD_Build(1, bullet_right);
					bullet_location = 1;
				}
				LCD_Cursor(bullet_horizontal_position+1);
				LCD_WriteData(0x01);
				LCD_Cursor(0);
				bullet_position = forward_bullet_1;
			}
			else if (player_position < 4){
				if(player_position == 2){
					LCD_Build(1, bullet_left);
					bullet_location = 2;
				}
				else if (player_position == 3){
					LCD_Build(1, bullet_right);
					bullet_location = 3;
				}
				bullet_horizontal_position += 16;
				LCD_Cursor(bullet_horizontal_position+1);
				LCD_WriteData(0x01);
				LCD_Cursor(0);
				bullet_position = forward_bullet_2;
			}
		}
		else{
		  bullet_position = create_bullet;
		}
		break;
	  case forward_bullet_1:
	    if(num_shots == 0){
		    bullet_position = no_ammo;
	    }
	    if(bullet_horizontal_position < 16){
		  ++bullet_horizontal_position;
		  LCD_Cursor(bullet_horizontal_position-1);
		  LCD_WriteData(' ');
	      LCD_Cursor(bullet_horizontal_position);
		  LCD_WriteData(0x01);
		  LCD_Cursor(0);
		  bullet_position = forward_bullet_1;
		}
		else {
		  bullet_position = despawn_bullet;
		}
		break;
		
	  case forward_bullet_2:
	    if(num_shots == 0){
		    bullet_position = no_ammo;
	    }
	    if((bullet_horizontal_position) < 32){
		  ++bullet_horizontal_position;
		  LCD_Cursor(bullet_horizontal_position-1);
		  LCD_WriteData(' ');
		  LCD_Cursor(bullet_horizontal_position);
		  LCD_WriteData(0x01);
		  LCD_Cursor(0);
		  bullet_position = forward_bullet_2;
	    }
	    else {
		  bullet_position = despawn_bullet;
	    }
	    break;
	  case despawn_bullet:
	    if(num_shots == 0){
		    bullet_position = no_ammo;
	    }
	    LCD_Cursor(16);
		LCD_WriteData(' ');
		LCD_Cursor(32);
		LCD_WriteData(' ');
		LCD_Cursor(bullet_horizontal_position);
		LCD_WriteData(' ');
		LCD_Cursor(0);
		shot_fired = 0;
		bullet_position = stall_bullet;
		break;
      case stall_bullet:
	    if(GetBit(tmp_val1, 0)){
		  bullet_position = stall_bullet;
		}
		else{
		  bullet_position = create_bullet;
		}
		break;
	  case no_ammo:
	    bullet_position = no_ammo;
		break;
	  default:
	    bullet_position = create_bullet;
		break;
  }	
};

unsigned char score;
unsigned char score_counter;
unsigned char difficulty;
unsigned char increase_shield;
enum score_system{initiate_score, check_score} score_checking;
void score_calc(){
	switch(score_checking){
	  case -1:
	    score_checking = initiate_score;
	    break;
	  case initiate_score:
	    score = 0;
	    score_counter = 0;
		difficulty = 0;
		increase_shield = 0;
		num_shots = 15;
	    score_checking = check_score;
	  case check_score:
	    
	    if (score == 1 && num_shots > 0){
		  ++num_shots;
		  score = 0;
	    }
	    PORTB = num_shots;
		if(score_counter == 10){
		  increase_shield = 1;
		  score_counter = 0;
		}
	    score_checking = check_score;
	    break;
	  default:
	    score_checking = initiate_score;
		break;
	}
};

enum enemy_death{check_collision} detection;
void death_calc(){
	switch (detection){
		case -1:
		  detection = check_collision;
		  break;
		case check_collision:
		  score = 0;
		  if(shot_fired && ((bullet_horizontal_position == enemy_location && bullet_location == position) || (bullet_horizontal_position+1 == enemy_location && bullet_location == position))){
			  enemy_position = despawn;
			  bullet_position = despawn_bullet;
			  score = 1;
		  }
		  detection = check_collision;
		  break;
		default:
		  detection = check_collision;
		  break;
	}
};

unsigned char shield_health;
enum shield {create_shield, shield_use, shield_destroy, shield_game_over} shield_state;
void deploy_shield(){
	switch(shield_state){
		case -1:
		  shield_state = create_shield;
		  break;
		case create_shield:
		  LCD_Build(3, shield_left);
		  LCD_Build(4, shield_right);
		  shield_health = 0x03;
		  shield_state = shield_use;
		  break;
		case shield_use:
		   LCD_Cursor(1);
		   LCD_WriteData(0x03);
		   LCD_Cursor(17);
		   LCD_WriteData(0x04);
		   LCD_Cursor(0);
		   PORTA = shield_health << 2;
		  if(shield_health < 3 && shield_health > 0){
		    if(increase_shield){
			  shield_health++;
			  increase_shield = 0;
		    }
			shield_state = shield_use;
		  }
		  if(enemy_location == 1 || enemy_location == 17){
			  shield_health--;
		  }
		  if (shield_health == 0){
		    shield_state = shield_destroy;
		  }
		  break;
		case shield_destroy:
		  LCD_ClearScreen();
		  PORTA = 0;
		  shield_state = shield_game_over;
		  break;
		case shield_game_over:
		  shield_state = shield_game_over;
		  break;
		default:
		  shield_state = create_shield;
		  break;
	}
};

enum game_over_screen {display_game_over} print_GO;
void print_game_over(){
	switch(print_GO){
		case -1:
		  print_GO = display_game_over;
		  break;
		case display_game_over:
		  LCD_DisplayString(1, "GAME OVER       Press start");
		  PORTA = 0;
		  PORTB = 0;
		  print_GO = display_game_over;
		  break;
		default:
		  print_GO = display_game_over;
		  break;
	}
};

// --------END User defined FSMs-----------------------------------------------

void replay(task* x){
	const unsigned short numTasks = sizeof(x)/sizeof(task*);
	num_shots = 15;
	shield_health = 7;
	for (unsigned char i = 0; i < numTasks; i++ ) {
		x[i].state = -1;
	}
}

// Implement scheduler code from PES.
int main(){
  DDRA = 0xFF; PORTA = 0x00; // LCD data lines
  DDRD = 0xFF; PORTD = 0x00; // LCD control lines
  DDRC = 0xFE; PORTC = 0x01;
  DDRB = 0xFF; PORTB = 0x00;

  // Period for the tasks
  unsigned long int player_tick_calc = 25;
  unsigned long int player_horiz_calc = 25;
  unsigned long int enemy_tick_calc = 50;
  unsigned long int bullet_tick_calc = 25;
  unsigned long int ctrl_tick_calc = 1;
  unsigned long int detection_tick_calc = 25;
  unsigned long int score_tick_calc = 25;
  unsigned long int shield_tick_calc = 100;
  //Calculating GCD
  unsigned long int tmpGCD = 1;
  tmpGCD = findGCD(player_tick_calc, enemy_tick_calc);
  tmpGCD = findGCD(tmpGCD, player_horiz_calc);
  tmpGCD = findGCD(tmpGCD, ctrl_tick_calc);
  tmpGCD = findGCD(tmpGCD, bullet_tick_calc);
  tmpGCD = findGCD(tmpGCD, detection_tick_calc);
  tmpGCD = findGCD(tmpGCD, score_tick_calc);

  //Greatest common divisor for all tasks or smallest time unit for tasks.
  unsigned long int GCD = tmpGCD;

  //Recalculate GCD periods for scheduler
  unsigned long int player_period = player_tick_calc/GCD;
  unsigned long int player_horiz_period = player_tick_calc/GCD;
  unsigned long int enemy_period = enemy_tick_calc/GCD;
  unsigned long int bullet_period = bullet_tick_calc/GCD;
  unsigned long int ctrl_period = ctrl_tick_calc/GCD;
  unsigned long int detection_period = detection_tick_calc/GCD;
  unsigned long int score_period = score_tick_calc/GCD;  
  unsigned long int shield_period = shield_tick_calc/GCD;
  unsigned long int game_over_period = 1000;
  //Declare an array of tasks 
  static task task1, task2, task3, task4, task5, task6, task7, task8, task9;
  task *tasks[] = {&task1, &task2, &task3, &task4, &task5, &task6, &task7, &task8, &task9};
  const unsigned short numTasks = sizeof(tasks)/sizeof(task*);
  
  // Task 1
  task1.state = -1;//Task initial state.
  task1.period = player_period;//Task Period.
  task1.elapsedTime = player_period;//Task current elapsed time.
  task1.TickFct = &player_movement;//Function pointer for the tick.

  // Task 2
  task2.state = -1;//Task initial state.
  task2.period = player_horiz_period;//Task Period.
  task2.elapsedTime = player_horiz_period;//Task current elapsed time.
  task2.TickFct = &player_hoizontal;//Function pointer for the tick.

  // Task 3
  task3.state = -1;//Task initial state.
  task3.period = bullet_period;//Task Period.
  task3.elapsedTime = bullet_period; // Task current elasped time.
  task3.TickFct = &bullet_logic; // Function pointer for the tick.

  // Task 4
  task4.state = -1;//Task initial state.
  task4.period = enemy_period;//Task Period.
  task4.elapsedTime = enemy_period; // Task current elasped time.
  task4.TickFct = &enemy_ai; // Function pointer for the tick.
  
  // Task 5
  task5.state = -1;//Task initial state.
  task5.period = ctrl_period;//Task Period.
  task5.elapsedTime = ctrl_period; // Task current elasped time.
  task5.TickFct = &ctrl_logic; // Function pointer for the tick.
  
  // Task 6
  task6.state = -1;//Task initial state.
  task6.period = detection_period;//Task Period.
  task6.elapsedTime = detection_period; // Task current elasped time.
  task6.TickFct = &death_calc; // Function pointer for the tick.
  
  // Task 7
  task7.state = -1;//Task initial state.
  task7.period = score_period;//Task Period.
  task7.elapsedTime = score_period; // Task current elasped time.
  task7.TickFct = &score_calc; // Function pointer for the tick.
  
  // Task 8
  task8.state = -1;//Task initial state.
  task8.period = shield_period;//Task Period.
  task8.elapsedTime = shield_period; // Task current elasped time.
  task8.TickFct = &deploy_shield; // Function pointer for the tick.
  
  // Task 9
  task9.state = -1;//Task initial state.
  task9.period = game_over_period;//Task Period.
  task9.elapsedTime = game_over_period; // Task current elasped time.
  task9.TickFct = &print_game_over; // Function pointer for the tick.
  
  // Set the timer and turn it on
  game_over = 0;
  LCD_init();
  LCD_ClearScreen();
  TimerOn();
  TimerSet(GCD);
  num_shots = 15;
  shield_health = 7;
  unsigned short i; // Scheduler for-loop iterator
  while(1) {
    // Scheduler codes
	
    for ( i = 0; i < numTasks; i++ ) {
        // Task is ready to tick
        if ( tasks[i]->elapsedTime == tasks[i]->period ) {
            // Setting next state for task
			if(num_shots == 0 || shield_health == 0){
				if(i == 4 || i == 8){
					tasks[i]->state = tasks[i]->TickFct(tasks[i]->state);
					// Reset the elapsed time for next tick.
					tasks[i]->elapsedTime = 0;
					if(GetBit(tmp_val1, 3)){
						replay(tasks);
					}
				}
			}
			else{
			  if(i < 8)
              tasks[i]->state = tasks[i]->TickFct(tasks[i]->state);
              // Reset the elapsed time for next tick.
              tasks[i]->elapsedTime = 0;
			}
        }
        tasks[i]->elapsedTime += 1;
    }
    while(!TimerFlag);
    TimerFlag = 0;
  }

  // Error: Program should not exit!
  return 0;
}
