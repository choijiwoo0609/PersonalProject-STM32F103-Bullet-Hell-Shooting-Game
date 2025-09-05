#include "device_driver.h"

#define LCDW				(320)
#define LCDH				(240)		
#define X_MIN	 			(0)			
#define X_MAX	 			(LCDW - 1)	
#define Y_MIN	 			(0)			
#define Y_MAX	 			(LCDH - 1)		

#define TIMER_PERIOD		(10)	

#define RIGHT           	(1)	
#define LEFT				(-1)

#define PLAYER_STEP			(10)
#define PLAYER_SIZE_X		(15)
#define PLAYER_SIZE_Y		(15)
#define ENEMY_STEP			(10)
#define ENEMY_SIZE_X		(15)
#define ENEMY_SIZE_Y		(15)			

#define BORDER_WIDTH		(5)			

#define BACK_COLOR			(5)			
#define ENEMY_COLOR			(0)				
#define PLAYER_COLOR		(1)			
#define BORDER_COLOR		(4)		

#define Max_heart			3	

#define GAME_OVER			(1)			

#define ENEMY_MAX_BULLETS		20			
#define ENEMY_BULLET_SPEED		5			
#define ENEMY_BULLET_WIDTH		5			
#define ENEMY_BULLET_HEIGHT		5				
#define ENEMY_BULLET_COLOR 		(0)			

#define PLAYER_MAX_BULLETS		10		
#define PLAYER_BULLET_SPEED		5				
#define PLAYER_BULLET_WIDTH		5		
#define PLAYER_BULLET_HEIGHT	5		
#define PLAYER_BULLET_COLOR 	4	

#define ITEM_SPAWN_CHANCE		1		
#define ITEM_LIFETIME			100		
#define ITEM_WIDTH				5	
#define ITEM_HEIGHT				5	

#define BASE  (500)
#define DISPLAY_MODE		3	

typedef struct {
    int x, y;
	int w, h;
    int life;
	int ci;
}Plane;

typedef struct {
    int x, y;
	int w, h;
	int active;			
	int ci;
}Bullet;

typedef enum {
	GAME_STATE_START,
	GAME_STATE_READY1,
	GAME_STATE_PLAY1,
	GAME_STATE_READY2,
	GAME_STATE_PLAY2,
	GAME_STATE_READY3,
	GAME_STATE_PLAY3,
	GAME_STATE_CLEAR,
	GAME_STATE_GAMEOVER
}GameState;

typedef struct {
	int x, y;
	int w, h;
	int active;
	int lifetime;
	int ci;
} Item;

static Plane Player;
static Plane Enemy;
static Bullet Enemy_Bullet[ENEMY_MAX_BULLETS];
static Bullet Player_Bullet[PLAYER_MAX_BULLETS];
static Item item;
static GameState gameState;

static int score;
static unsigned short color[] = {RED, YELLOW, GREEN, BLUE, WHITE, BLACK};	

extern volatile int TIM4_expired;
extern volatile int USART1_rx_ready;
extern volatile int USART1_rx_data;
extern volatile int Jog_key_in;
extern volatile int Jog_key;
extern volatile int plane_bullet_timer;
extern volatile int plane_bullet_ready;

static void Draw_Object(Plane * obj)
{
	Lcd_Draw_Box(obj->x, obj->y, obj->w, obj->h, color[obj->ci]);
}

static void Draw_Border(void)
{
	Lcd_Draw_Box(0, 0, LCDW, BORDER_WIDTH, color[BORDER_COLOR]);

	Lcd_Draw_Box(0, LCDH - BORDER_WIDTH, LCDW, BORDER_WIDTH, color[BORDER_COLOR]);

	Lcd_Draw_Box(0, 0, BORDER_WIDTH, LCDH, color[BORDER_COLOR]);

    Lcd_Draw_Box(LCDW - BORDER_WIDTH, 0, BORDER_WIDTH, LCDH, color[BORDER_COLOR]);
}

void drawStartScreen() 
{
	Lcd_Clr_Screen();
	Lcd_Printf(20, 80, WHITE, BLACK, 2, 2, "PLAY!");
	Lcd_Printf(20, 110, WHITE, BLACK, 1, 1, "Press any key to start");
	Uart_Printf("Press any key to start\n");
}

void drawReady1Screen() 
{
	Lcd_Clr_Screen();
	Lcd_Printf(20, 20, WHITE, BLACK, 2, 2, "Stage 1");
	Lcd_Printf(20, 50, WHITE, BLACK, 1, 1, "Press any key to start");
	Uart_Printf("Stage 1 : Press any key to start\n");
}

void drawReady2Screen() 
{
	Lcd_Clr_Screen();
	Lcd_Printf(20, 80, WHITE, BLACK, 2, 2, "Stage 2");
	Lcd_Printf(20, 110, WHITE, BLACK, 1, 1, "Press any key to start");
	Uart_Printf("Stage 2 : Press any key to start\n");
}

void drawReady3Screen() 
{
	Lcd_Clr_Screen();
	Lcd_Printf(20, 80, WHITE, BLACK, 2, 2, "Stage 3");
	Lcd_Printf(20, 110, WHITE, BLACK, 1, 1, "Press any key to start");
	Uart_Printf("Stage 3 : Press any key to start\n");
}

void drawGameClearScreen(){
	Lcd_Clr_Screen();
	Lcd_Printf(20, 80, GREEN, BLACK, 2, 2, "GAME Clear!");
	Lcd_Printf(20, 110, WHITE, BLACK, 1, 1, "Congratulation!");
	Uart_Printf("Game Clear, Congratulation!\n");
}

void drawGameOverScreen()
{
	Lcd_Clr_Screen();
	Lcd_Printf(20, 80, RED, BLACK, 2, 2, "GAME OVER!");
	Lcd_Printf(20, 110, WHITE, BLACK, 1, 1, "Press any key to restart");
	Uart_Printf("Game Over, Please press any key to continue.\n");
}

#include <stdlib.h>
#include <stdint.h>

const uint8_t heart_bitmap[9] = {
    0b01100110, //   **  **
    0b11111111, // ********
    0b11111111, // ********
    0b11111111, // ********
    0b01111110, //  ******
    0b00111100, //   ****
    0b00011000, //    **
    0b00000000, // 
    0b00000000  //
};

void Draw_Heart(int x, int y, uint16_t color)
{
    int row, col;

    for (row = -1; row <= 9; row++) {
        for (col = -1; col <= 8; col++) {
            if (x + col >= 0 && x + col < LCDW && y + row >= 0 && y + row < LCDH) {
                Lcd_Put_Pixel(x + col, y + row, GREEN);
            }
        }
    }

    for (row = 0; row < 9; row++) {
        uint8_t line = heart_bitmap[row];
        for (col = 0; col < 8; col++) {
            if (line & (0x80 >> col)) {
                Lcd_Put_Pixel(x + col, y + row, color);
            }
        }
    }
}

void render_hearts(int life, int start_x, int start_y, uint16_t color)
{
    int heart_width = 8;
    int heart_gap = 4;

    int i;

    for (i = 0; i < life; i++) {
        int x_pos = start_x + i * (heart_width + heart_gap);
        Draw_Heart(x_pos, start_y, color);
    }

    for (i = life; i < Max_heart; i++) {
        int x_pos = start_x + i * (heart_width + heart_gap);
        Draw_Heart(x_pos, start_y, GREEN);
    }
}

static void Buzzer_Beep(unsigned char tone, int duration)
{
	const static unsigned short tone_value[] = {261,277,293,311,329,349,369,391,415,440,466,493,523,554,587,622,659,698,739,783,830,880,932,987};

	TIM3_Out_Freq_Generation(tone_value[tone]);
	TIM2_Delay(duration);
	TIM3_Out_Stop();
}

static void Hit_Enemy_Sound(){
	TIM3_Out_Freq_Generation(1000);
	TIM2_Delay(30);
	TIM3_Out_Stop();
}

static void Hit_Player_Sound(){
	TIM3_Out_Freq_Generation(300);
	TIM2_Delay(40);
	TIM3_Out_Stop();
}

static void Eat_Player_HP(){
    TIM3_Out_Freq_Generation(500);
    TIM2_Delay(20);
    TIM3_Out_Stop();
    TIM2_Delay(20);
    TIM3_Out_Freq_Generation(500);
    TIM2_Delay(20);
    TIM3_Out_Stop();
}

static int Check_Plane_Collision(void)
{
	int col = 0;

	if((Enemy.x >= Player.x) && ((Player.x + PLAYER_STEP) >= Enemy.x)) col |= 1<<0;	
	else if((Enemy.x < Player.x) && ((Enemy.x + ENEMY_STEP) >= Player.x)) col |= 1<<0;
	
	if((Enemy.y >= Player.y) && ((Player.y + PLAYER_STEP) >= Enemy.y)) col |= 1<<1;	
	else if((Enemy.y < Player.y) && ((Enemy.y + ENEMY_STEP) >= Player.y)) col |= 1<<1;

	if (col == 3){
		Player.life -= 1;
	}

	if(Player.life == 0)
	{
		return GAME_OVER;
	}
	return 0;
}

static int Check_Bullet_Collision(void)
{
	int i;
	int col = 0;

	for(i = 0; i < ENEMY_MAX_BULLETS; i++){
		if(!Enemy_Bullet[i].active) continue;

		if( (Enemy_Bullet[i].x >= Player.x) && ((Player.x + PLAYER_STEP) >= Enemy_Bullet[i].x)) col |= 1<<0;
		else if ( (Enemy_Bullet[i].x < Player.x) && ((Enemy_Bullet[i].x + ENEMY_BULLET_SPEED) >= Player.x)) col |= 1<<0;

		if( (Enemy_Bullet[i].y >= Player.y) && ((Player.y + PLAYER_STEP) >= Enemy_Bullet[i].y)) col |= 1<<1;
		else if ( (Enemy_Bullet[i].y < Player.y) & ((Enemy_Bullet[i].y + ENEMY_BULLET_SPEED) >= Player.y) )col |= 1<<1;

		if (col != 3) col = 0;
		if (col == 3) break;
	}

	if(col == 3)
	{
		Hit_Player_Sound();
		Enemy_Bullet[i].ci = BLACK;
		Lcd_Draw_Box(Enemy_Bullet[i].x, Enemy_Bullet[i].y, Enemy_Bullet[i].w, Enemy_Bullet[i].h, color[Enemy_Bullet[i].ci]);
		Enemy_Bullet[i].ci = RED;
		Lcd_Draw_Box(Enemy_Bullet[i].x, Enemy_Bullet[i].y, Enemy_Bullet[i].w, Enemy_Bullet[i].h, color[Enemy_Bullet[i].ci]);
		Player.life -= 1;
		Uart_Printf("You lose your life! Present Life : %d\n", Player.life);
		Enemy_Bullet[i].active = 0;
	}

	if(Player.life == 0)
	{
		return GAME_OVER;
	}

	return 0;
}

static void Score(void)
{
	int target = 0;
	int i;

	for(i = 0; i < PLAYER_MAX_BULLETS; i++){
		target = 0;
		if(!Player_Bullet[i].active) continue;

		if ( (Player_Bullet[i].x >= Enemy.x) && ((Enemy.x + ENEMY_STEP) >= Player_Bullet[i].x) ) target |= 1<<0;
		else if ( (Player_Bullet[i].x < Enemy.x) && ((Player_Bullet[i].x + PLAYER_BULLET_SPEED) >= Enemy.x) ) target |= 1<<0;

		if ( (Player_Bullet[i].y >= Enemy.y) && ((Enemy.y + ENEMY_STEP) >= Player_Bullet[i].y) ) target |= 1<<1;
		else if ( (Player_Bullet[i].y < Enemy.y) && ((Player_Bullet[i].y + PLAYER_BULLET_SPEED) >= Enemy.y) ) target |= 1<<1;

		if (target == 3) {
			Hit_Enemy_Sound();
			Player_Bullet[i].ci = BLACK;
			Lcd_Draw_Box(Player_Bullet[i].x, Player_Bullet[i].y, Player_Bullet[i].w, Player_Bullet[i].h, color[Player_Bullet[i].ci]);
			Player_Bullet[i].ci = RED;
			Lcd_Draw_Box(Player_Bullet[i].x, Player_Bullet[i].y, Player_Bullet[i].w, Player_Bullet[i].h, color[Player_Bullet[i].ci]);
			score += 1;
			Uart_Printf("Get_Score : %d\n", score);
			Lcd_Printf(BORDER_WIDTH, BORDER_WIDTH, BLUE, YELLOW, 2, 2, "%d", score);
			Player_Bullet[i].active = 0;
			break;
		}
	}
}


static void k0(void)
{
	if(Player.y > Y_MIN) {Player.y -= PLAYER_STEP;}
}
static void k1(void)
{
	if(Player.y + Player.h < Y_MAX) {Player.y += PLAYER_STEP;}
}
static void k2(void)
{
	if(Player.x > X_MIN) {Player.x -= PLAYER_STEP;}
}
static void k3(void)
{
	if(Player.x + Player.w < LCDW - BORDER_WIDTH) {
		Player.x += PLAYER_STEP;
	}
}

static int Player_Move(int k)
{
	static void (*key_func[])(void) = {k0, k1, k2, k3};

	if(k <= 3) key_func[k]();

	if(Player.x <= BORDER_WIDTH) {Player.x += PLAYER_STEP;}
	if(Player.x + Player.w >= LCDW - BORDER_WIDTH) {
		Player.x -= PLAYER_STEP;
	}

	if(Player.y <= BORDER_WIDTH) {Player.y += PLAYER_STEP;}
	if(Player.y + Player.h >= LCDH - BORDER_WIDTH * 2){
		Player.y -= PLAYER_STEP;
	}

	return Check_Plane_Collision();
}

static int Enemy_Move() 
{
	int i = rand();
	if (i%2 == 0) {Enemy.x += ENEMY_STEP * RIGHT;}
	else {Enemy.x += ENEMY_STEP * LEFT;}

	if(Enemy.x <= BORDER_WIDTH * 7) {Enemy.x += ENEMY_STEP;}
	if(Enemy.x + Enemy.w >= LCDW - BORDER_WIDTH) {Enemy.x -= ENEMY_STEP;}

	try_spawn_item();

	return Check_Plane_Collision();
}

void try_spawn_item() {
	if ((item.active) || (Player.life >= Max_heart)) { return;}

	int r = rand() % 100;
	if(r < ITEM_SPAWN_CHANCE) {
		item.x = BORDER_WIDTH * 2 + rand() % (LCDW - 2 * BORDER_WIDTH - item.w);
		item.y = BORDER_WIDTH * 2 + rand() % (LCDH - 3 * BORDER_WIDTH - item.h);
		item.w = ITEM_WIDTH;
		item.h = ITEM_HEIGHT;
		item.active = 1;
		item.ci = 2;		
		item.lifetime = ITEM_LIFETIME;
	}
}

static void update_item() {
	if (!item.active) return;

    int col = 0;
    if ((item.x >= Player.x) && ((Player.x + PLAYER_SIZE_X) >= item.x)) col |= 1<<0;
    else if ((item.x < Player.x) && ((item.x + item.w) >= Player.x)) col |= 1<<0;

    if ((item.y >= Player.y) && ((Player.y + PLAYER_SIZE_Y) >= item.y)) col |= 1<<1;
    else if ((item.y < Player.y) && ((item.y + item.h) >= Player.y)) col |= 1<<1;

    if (col == 3) {
		Eat_Player_HP();

        if (Player.life < Max_heart) {
            Player.life += 1;
			Uart_Printf("You get a life! Present Life : %d\n", Player.life);
        }
		Lcd_Draw_Box(item.x, item.y, item.w, item.h, color[1]);
        item.active = 0;
        item.lifetime = 0;
    }
}

void draw_item() {
	if (!item.active) return;
	Lcd_Draw_Box(item.x, item.y, item.w, item.h, color[item.ci]);
}

void item_func() {
	try_spawn_item();
	update_item();
	draw_item();

	if(item.active){
		item.lifetime--;
		if(item.lifetime <= 0) {
			item.active = 0;
			Lcd_Draw_Box(item.x, item.y, item.w, item.h, color[5]);
		}

	}
}

static void Player_Bullet_Init(void)
{
	int i;

	for (i = 0; i < PLAYER_MAX_BULLETS; i++){
		Player_Bullet[i].active = 0;
	}
}

static void Player_Shoot(void)
{
	int i;

	for(i = 0; i < PLAYER_MAX_BULLETS; i++){
		if(Player_Bullet[i].active == 0){
			Player_Bullet[i].x = Player.x + Player.w/2 - PLAYER_BULLET_WIDTH/2;
			Player_Bullet[i].y = Player.y - 10;
			Player_Bullet[i].w = PLAYER_BULLET_WIDTH;
			Player_Bullet[i].h = PLAYER_BULLET_HEIGHT;
			Player_Bullet[i].ci = PLAYER_BULLET_COLOR;
			Player_Bullet[i].active = 1;
			break;
		}
	}
}

static void Player_Bullet_Move(void)
{
	int i;

	for(i = 0; i < PLAYER_MAX_BULLETS; i++){
		if(Player_Bullet[i].active == 1){
			Lcd_Draw_Box(Player_Bullet[i].x, Player_Bullet[i].y, Player_Bullet[i].w, Player_Bullet[i].h, color[BACK_COLOR]);

			Player_Bullet[i].y -= PLAYER_BULLET_SPEED;

			if(Player_Bullet[i].y + Player_Bullet[i].h <= BORDER_WIDTH){
				Player_Bullet[i].active = 0;
			}
			else {
				Lcd_Draw_Box(Player_Bullet[i].x, Player_Bullet[i].y, Player_Bullet[i].w, Player_Bullet[i].h, color[Player_Bullet[i].ci]);
			}
		}
	}
}

static void Enemy_Bullet_Init(void)
{
	int i;

	for (i = 0; i < ENEMY_MAX_BULLETS; i++){
		Enemy_Bullet[i].active = 0;
	}
}

static void Enemy_Shoot(void)
{
	int i;

	for (i = 0; i < ENEMY_MAX_BULLETS; i++){
		if(Enemy_Bullet[i].active == 0){
			Enemy_Bullet[i].x = Enemy.x + Enemy.w/2 - ENEMY_BULLET_WIDTH/2;
			Enemy_Bullet[i].y = Enemy.y + Enemy.h;
			Enemy_Bullet[i].w = ENEMY_BULLET_WIDTH;
			Enemy_Bullet[i].h = ENEMY_BULLET_HEIGHT;
			Enemy_Bullet[i].ci = ENEMY_BULLET_COLOR;
			Enemy_Bullet[i].active = 1;
			break;
		}
	}
}

static int Enemy_Bullet_Move(void)
{
	int i;

	for(i = 0; i < ENEMY_MAX_BULLETS; i++){
		if (Enemy_Bullet[i].active){
			Lcd_Draw_Box(Enemy_Bullet[i].x, Enemy_Bullet[i].y, Enemy_Bullet[i].w, Enemy_Bullet[i].h, color[BACK_COLOR]);

			Enemy_Bullet[i].y += ENEMY_BULLET_SPEED;

			if(Enemy_Bullet[i].y >= LCDH - BORDER_WIDTH) {
				Enemy_Bullet[i].active = 0;
			}
			else {
				Lcd_Draw_Box(Enemy_Bullet[i].x, Enemy_Bullet[i].y, Enemy_Bullet[i].w, Enemy_Bullet[i].h, color[Enemy_Bullet[i].ci]);
			}
		}
	}

	return Check_Bullet_Collision();
}

void System_Init(void)
{
	Clock_Init();
	LED_Init();
	Key_Poll_Init();
	Uart1_Init(115200);

	SCB->VTOR = 0x08003000;
	SCB->SHCSR = 7<<16;
}

static void Game_Init(void)
{
	Lcd_Clr_Screen();

	Player.x = LCDW / 2;
	Player.y = LCDH - 30;
	Player.w = PLAYER_SIZE_X;
	Player.h = PLAYER_SIZE_Y;
	Player.ci = PLAYER_COLOR;

	Enemy.x = LCDW / 2;
	Enemy.y = 30;
	Enemy.w = ENEMY_SIZE_X;
	Enemy.h = ENEMY_SIZE_Y;
	Enemy.ci = ENEMY_COLOR;

	Lcd_Draw_Box(Player.x, Player.y, Player.w, Player.h, color[Player.ci]);
	Lcd_Draw_Box(Enemy.x, Enemy.y, Enemy.w, Enemy.h, color[Enemy.ci]);
}

void Ready_State(GameState nextState, void (*drawFunc)(void)) {
    drawFunc();
    Jog_Wait_Key_Pressed();
    Jog_Wait_Key_Released();
    Lcd_Clr_Screen();
    Game_Init();
    Enemy_Bullet_Init();
    Player_Bullet_Init();
    Lcd_Printf(BORDER_WIDTH, BORDER_WIDTH, BLUE, YELLOW, 2, 2, "%d", score);
    render_hearts(Player.life, BORDER_WIDTH, LCDH - BORDER_WIDTH * 3, RED);
	Draw_Border();
}

void Play_Level(GameState nextState, int scoreTarget, int timerDelay) {
    int inter_lock = 0;
    int prev_hp = Player.life;
    TIM4_Repeat_Interrupt_Enable(1, timerDelay);

    while (1) {
        int game_over_1 = 0, game_over_2 = 0;

        if ((Jog_key_in) && (inter_lock == 0)) {
            inter_lock = 1;
            Player.ci = BACK_COLOR;
            Draw_Object(&Player);
            game_over_1 = Player_Move(Jog_key);
            Player.ci = PLAYER_COLOR;
            Draw_Object(&Player);

        if (Jog_key == 5) Player_Shoot();
            Jog_key_in = 0;
        } else if (Jog_key_in == 0 && inter_lock == 1) {
            inter_lock = 0;
        }

        if (TIM4_expired) {
            Enemy.ci = BACK_COLOR;
            Draw_Object(&Enemy);
            game_over_1 = Enemy_Move();
            Enemy.ci = ENEMY_COLOR;
            Draw_Object(&Enemy);

            game_over_2 = Enemy_Bullet_Move();
            Player_Bullet_Move();

			item_func();

            if (prev_hp != Player.life) {
                render_hearts(Player.life, BORDER_WIDTH, LCDH - BORDER_WIDTH * 3, RED);
                prev_hp = Player.life;
            }

            if (rand() % 5 == 0) Enemy_Shoot();
            TIM4_expired = 0;
        }

        Score();

        if (game_over_1 || game_over_2) {
            score = 0;
            TIM4_Repeat_Interrupt_Enable(0, 0);
            Player.life = 0;
            return;
        }

        if (score >= scoreTarget) {
            return;
        }
    }
}

void Main(void)
{
    System_Init();
    Uart_Printf("프로젝트_1 : 비행 탄막 게임\n");

    Lcd_Init(DISPLAY_MODE);
    Jog_Poll_Init();
    Jog_ISR_Enable(1);
    Uart1_RX_Interrupt_Enable(1);

    TIM3_Out_Init();

	gameState = GAME_STATE_START;

	enum key{C1, C1_, D1, D1_, E1, F1, F1_, G1, G1_, A1, A1_, B1, C2, C2_, D2, D2_, E2, F2, F2_, G2, G2_, A2, A2_, B2};
	enum note{N16=BASE/4, N8=BASE/2, N4=BASE, N2=BASE*2, N1=BASE*4};
	const int shoot_theme[][2] = {{C2, N8}, {D2, N8}, {G1, N8}, {C2, N8}, {F1, N8}, {G1, N8}, {C2, N8}, {D2, N8}, {A1, N8}, {G1, N8}, {E1, N8}, {F1, N8}, {G1, N4}, {C2, N4}};
	const int clear_theme[][2] = {{C2, N8}, {E2, N8}, {G2, N8}, {E2, N8}, {F2, N8}, {A2, N8}, {G2, N8}, {E2, N8}, {D2, N8}, {F2, N8}, {E2, N8}, {C2, N8}, {G1, N4}, {C2, N4}};

	const int gameover_theme[][2] = {{E2, N8}, {D2, N8}, {C2, N8}, {A1, N8}, {C2, N8}, {B1, N8}, {G1, N4}, {C2, N4}};

    while (1)
    {
        switch (gameState)
        {
	        case GAME_STATE_START:
                drawStartScreen();
                Player.life = 3;
                Jog_key_in = 0;

				int i;

				for(;;){
					for (i = 0; i < sizeof(shoot_theme) / sizeof(shoot_theme[0]); i++) {
						Buzzer_Beep(shoot_theme[i][0], shoot_theme[i][1]);
						if (Jog_key_in) {
							TIM3_Out_Stop();
							Jog_key_in = 0;
							goto XXXX;
						}
					}
				}

				XXXX:;
				Uart_Printf("Welcome!\n");
                gameState = GAME_STATE_READY1;
                break;

            case GAME_STATE_READY1:
                Ready_State(GAME_STATE_PLAY1, drawReady1Screen);
                gameState = GAME_STATE_PLAY1;
                break;

			case GAME_STATE_PLAY1:
				Play_Level(GAME_STATE_READY2, 10, TIMER_PERIOD * 20);
				Uart_Printf("Round 1!\n");
				gameState = (Player.life <= 0) ? GAME_STATE_GAMEOVER : GAME_STATE_READY2;
				break;

            case GAME_STATE_READY2:
                Ready_State(GAME_STATE_PLAY2, drawReady2Screen);
                gameState = GAME_STATE_PLAY2;
                break;

			case GAME_STATE_PLAY2:
				Play_Level(GAME_STATE_READY3, 20, TIMER_PERIOD * 15);
				Uart_Printf("Round 2!\n");
				gameState = (Player.life <= 0) ? GAME_STATE_GAMEOVER : GAME_STATE_READY3;
				break;

            case GAME_STATE_READY3:
                Ready_State(GAME_STATE_PLAY3, drawReady3Screen);
                gameState = GAME_STATE_PLAY3;
                break;

			case GAME_STATE_PLAY3:
				Play_Level(GAME_STATE_CLEAR, 30, TIMER_PERIOD * 5);
				Uart_Printf("Round 3!\n");
				gameState = (Player.life <= 0) ? GAME_STATE_GAMEOVER : GAME_STATE_CLEAR;
				break;

            case GAME_STATE_CLEAR:
                drawGameClearScreen();

				int j;
				for (j = 0; j < sizeof(clear_theme) / sizeof(clear_theme[0]); j++) {
                    Buzzer_Beep(clear_theme[j][0], clear_theme[j][1]);
                    if (Jog_key_in) {
                        TIM3_Out_Stop();
                        Jog_key_in = 0;
                        break;
                    }
                }

                Jog_Wait_Key_Pressed();
                Jog_Wait_Key_Released();
				score = 0;
                Uart_Printf("GAME CLEAR!\n");
                gameState = GAME_STATE_START;
                break;

            case GAME_STATE_GAMEOVER:
                drawGameOverScreen();

				int k;
                for (k = 0; k < sizeof(gameover_theme) / sizeof(gameover_theme[0]); k++) {
                    Buzzer_Beep(gameover_theme[k][0], gameover_theme[k][1]);
                    if (Jog_key_in) {
                        TIM3_Out_Stop();
                        Jog_key_in = 0;
                        break;
                    }
                }

                Jog_Wait_Key_Pressed();
                Jog_Wait_Key_Released();
                Uart_Printf("Game OVER!\n");
                gameState = GAME_STATE_START;
                break;
        }
    }
}