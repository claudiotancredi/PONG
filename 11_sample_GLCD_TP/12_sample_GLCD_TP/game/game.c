		#include "../GLCD/GLCD.h" 
		#include "../timer/timer.h"
		#include "../adc/adc.h"
		#include <stdio.h>

		extern int game_status;
		extern int record;

		extern int ball_position[];
		extern int flag_end_game;
		int paddle_x = 100;

		int current_score = 0;
		
		
		void resetDisplayAndShowInitialMessage(void){
			LCD_Clear(Black);
			GUI_Text(104, 147, (uint8_t *) "PONG", White, Black);
			GUI_Text(44, 165, (uint8_t *) "Press KEY1 to start", White, Black);
			return;
		}
			
		void drawGameBordersAndBall(void){
			LCD_DrawGameHBorder(0,0,239, 5, Red); //Top border
			LCD_DrawGameVBorders(0, 5,278,235,5,Red); //Side borders, includes drawing the ball
			return;
		}
		
		void drawPaddle(void){
			LCD_DrawGameHBorder(paddle_x,278,paddle_x+39,10,Green); //paddle
			return;
		}
		
		void writeRecord(void){
			char r[]="";
			sprintf(r,"%d", record);
			GUI_Text(233-9*strlen(r),8,(uint8_t*) r, White, Black); //write record
			return;
		}
		
		void writeCurrentScore(void){
			char r[]="";
			sprintf(r,"%d", current_score);
			GUI_Text(8,155,(uint8_t*) r, White, Black); //write current score
			return;
		}
		
		void stopPaddleAndBallAndSound(void){
			disable_timer(2);
			disable_timer(0);
			reset_timer(0);
			return;
		}
		
		void writePausedText(void){
			if ((ball_position[0]+4>=95 && ball_position[0]<=154) && (ball_position[1]+4>=155 && ball_position[1]<=166)){
					LCD_DrawGameHBorder(ball_position[0],ball_position[1],ball_position[0]+4,5,Black);
				}
				GUI_Text(95,155, (uint8_t*) "Paused", White, Black);
				if ((ball_position[0]+4>=95 && ball_position[0]<=154) && (ball_position[1]+4>=155 && ball_position[1]<=166)){
					LCD_DrawGameHBorder(ball_position[0],ball_position[1],ball_position[0]+4,5,Green);
				}
				return;
		}
		
		void clearPausedText(void){
			LCD_DrawGameHBorder(95, 155, 150, 12, Black);
				if ((ball_position[0]+4>=95 && ball_position[0]<=154) && (ball_position[1]+4>=155 && ball_position[1]<=166)){
					LCD_DrawGameHBorder(ball_position[0],ball_position[1],ball_position[0]+4,5,Green);
				}
				return;
		}
		
		void restartPaddleAndBall(void){
			enable_timer(0);
		}

		void reset_game(void){
			flag_end_game=0;
			resetDisplayAndShowInitialMessage();
			game_status=0;
			return;
		}

		void start_game(void){
			ADC_init();	
			LCD_DrawGameHBorder(44,147,230,30,Black); //Delete text
			drawGameBordersAndBall();
			drawPaddle();
			writeRecord();
			writeCurrentScore();
			init_timer(1, 0x17D7840); /*1 s * 25 MHz = 25*10^6=0x17D7840       timer for countdown*/
			enable_timer(1);
			return;
		}

		void pause_game(void){
			if (game_status==1){
				stopPaddleAndBallAndSound();
				writePausedText();
				game_status=2;
			}
			else{
				clearPausedText();
				restartPaddleAndBall();
				game_status=1;
			}
			return;
		}
