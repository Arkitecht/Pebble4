#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"
#include "Pebble4.h"


#define MY_UUID { 0xCA, 0x42, 0x3B, 0x65, 0x5D, 0xA6, 0x49, 0x0A, 0xB3, 0x29, 0xC6, 0x87, 0x8E, 0xC8, 0xCE, 0x62 }
PBL_APP_INFO(MY_UUID,
             "Pebble4", 
			 "Aaron Rubin (aaron@arkitech.net)",
             1, 0, /* App version */
             RESOURCE_ID_IMAGE_MENU_ICON,
             APP_INFO_STANDARD_APP);

// Can be used to distinguish between multiple timers in your app
#define CPU_TIMER 1
#define COLS 7
#define ROWS 6
#define STEP_SIZE 16.17
#define LINE_SIZE 3
#define GUTTER 4.5
#define START_Y 40
#define PIECE_SIZE 5
#define PIECE_SPACE 7

typedef struct {
	int layout[ROWS][COLS];
} Board;


typedef struct {
 //holds a score, column pair. 
 int score;
 int col;
} MoveValue;

Window window;
Layer boardLayer, chooseLayer, dropdownLayer;
TextLayer turnLayer;
int whosTurn = 1;
int chosenCol = 0;
int rowToFill = -1;
bool waiting = 0;
bool dropping = 0;
bool dropRow = 0;
bool choosing = 0;
bool gameOver = 0;
AppContextRef appContext;
int turnCount = 0;

PropertyAnimation prop_animation;

// Can be used to cancel timer via `app_timer_cancel_event()`
AppTimerHandle timer_handle;
Board gameBoard;



void updateMoveValue(MoveValue *v,int s,int c) {
	v->score = s;
	v->col = c;
}

MoveValue createMoveValue(int startVal) {
	MoveValue v;
	updateMoveValue(&v,startVal,0);
	return v;
}


bool have_a_winner(int v,int h, int d,int *whoWon) {
	if (v != -1) {
		*whoWon = v;
		return 1;
	} else if	(h != -1 ) {
		*whoWon = h;
		return 1;
	} else if	(d != -1 ) {
		*whoWon = d;
		return 1;
	} else {
		*whoWon = -1;
		return 0;
	}
}


int check_for_winner_v(int row,int col) {
	int startPlayer = gameBoard.layout[row][col];
	
	if ( startPlayer == -1 ) return -1;
	
	int lastRow = row + 3;
	if ( lastRow >= ROWS ) return -1;
	for (int i=0; i<4; i++ ) {
		if ( gameBoard.layout[row+i][col] != startPlayer ) return -1;
	}
	return startPlayer;
}

int check_for_winner_h(int row,int col) {
	int startPlayer = gameBoard.layout[row][col];
	
	if ( startPlayer == -1 ) return -1;
	
	int lastCol = col + 3;
	if ( lastCol >= COLS ) return -1;
	for (int i=0; i<4; i++ ) {
		if ( gameBoard.layout[row][col+i] != startPlayer ) return -1;
	}
	return startPlayer;
}

int check_for_winner_d(int row,int col) {
	int startPlayer = gameBoard.layout[row][col];
	int won = 1;
	
	if ( row > 2 ) return -1;
	
	if ( startPlayer == -1 ) return -1;
		
	int lastRow = row + 3;
	if ( lastRow > ROWS ) return -1;
	//FORWARDS
	int lastCol = col + 3;
	if ( lastCol < COLS ) {
		for (int i=0; i<4; i++ ) {
			if ( gameBoard.layout[row+i][col+i] != startPlayer ) won = 0;		
		}
	
		if ( won ) return startPlayer;
	}
	
	won = 1;
	lastCol = col - 3;
	//BACKWARDS	
	if ( lastCol >= 0 ) {
		for (int i=0; i<4; i++ ) {
			if ( gameBoard.layout[row+i][col-i] != startPlayer ) won = 0;		
		}
		if ( won ) return startPlayer;
	}
	return -1;
}

int calculateCurrentValue() {
	//HORIZONTAL
	for(int i=5;i>=0;i--) {
  		for(int j=0;j<4;j++) {
   			if (gameBoard.layout[i][j] != -1 &&
				gameBoard.layout[i][j] == gameBoard.layout[i][j+1] &&
				gameBoard.layout[i][j] == gameBoard.layout[i][j+2] &&
				gameBoard.layout[i][j] == gameBoard.layout[i][j+3] ) {		
				
				if( gameBoard.layout[i][j] == 1 )
     				return 100;
				else return -100;
			}
		}
	}

 	//VERTICAL
	for(int i=5;i>2;i--) {
		for(int j=0;j<COLS;j++) {
   			if( gameBoard.layout[i][j] != -1 &&
				gameBoard.layout[i][j] == gameBoard.layout[i-1][j] &&
				gameBoard.layout[i][j] == gameBoard.layout[i-2][j] && 
				gameBoard.layout[i][j] == gameBoard.layout[i-3][j] ) {
    				if(gameBoard.layout[i][j]==1)
     					return 100;
    				else return -100;
			}
		}
	}

	//check for 4 in a diagonal up ->
	for(int i=5;i>2;i--) {
		for(int j=0;j<4;j++) {
			if( gameBoard.layout[i][j] != -1 &&
				gameBoard.layout[i][j] == gameBoard.layout[i-1][j+1] &&
				gameBoard.layout[i][j] == gameBoard.layout[i-2][j+2] &&
				gameBoard.layout[i][j] == gameBoard.layout[i-3][j+3]) {
    				if(gameBoard.layout[i][j]==1)
     					return 100;
    				else return -100;
    		}
		}
	}

 	
	//check for 4 in the reverse diagonal
	for(int i=5;i>2;i--) {
		for(int j=6;j>2;j--) {
			if(	gameBoard.layout[i][j] != -1 &&  
				gameBoard.layout[i][j] == gameBoard.layout[i-1][j-1] &&
				gameBoard.layout[i][j] == gameBoard.layout[i-2][j-2] &&
				gameBoard.layout[i][j] == gameBoard.layout[i-3][j-3] ) {
					if(gameBoard.layout[i][j]==1) 
     					return 100;
    				else return -100;
			}
		}
	}

	//HORIZONTAL - 3 in a row
	for(int i=5;i>=0;i--) {
  		for(int j=0;j<4;j++) {
   			if (gameBoard.layout[i][j] != -1 &&
				gameBoard.layout[i][j] == gameBoard.layout[i][j+1] &&
				gameBoard.layout[i][j] == gameBoard.layout[i][j+2] ) {		
				
				if( gameBoard.layout[i][j] == 1 )
     				return 50;
				else return -50;
			}
		}
	}
	
	//VERTICAL - 3
	for(int i=5;i>2;i--) {
		for(int j=0;j<COLS;j++) {
   			if( gameBoard.layout[i][j] != -1 &&
				gameBoard.layout[i][j] == gameBoard.layout[i-1][j] &&
				gameBoard.layout[i][j] == gameBoard.layout[i-2][j] ) {
    				if(gameBoard.layout[i][j]==1)
     					return 50;
    				else return -50;
			}
		}
	}
	
	//HORIZONTAL - 2 in a row
	for(int i=5;i>=0;i--) {
  		for(int j=0;j<4;j++) {
   			if (gameBoard.layout[i][j] != -1 &&
				gameBoard.layout[i][j] == gameBoard.layout[i][j+1] ) {		
				
				if( gameBoard.layout[i][j] == 1 )
     				return 25;
				else return -25;
			}
		}
	}
	
	//VERTICAL - 2
	for(int i=5;i>2;i--) {
		for(int j=0;j<COLS;j++) {
   			if( gameBoard.layout[i][j] != -1 &&
				gameBoard.layout[i][j] == gameBoard.layout[i-1][j] ) {
    				if(gameBoard.layout[i][j]==1)
     					return 25;
    				else return -25;
			}
		}
	}

	if( gameBoard.layout[5][3] != -1 ) return (gameBoard.layout[5][3])?10:-10;
	
	return 0;
}

MoveValue findBestMove(int whichPlayer,int depth,int maxDepth)
 {//Finsd the best move for the side. Returns column and score pair 
  //depth specifies current level of depth (should be passed as 0)
  //maxDepth specifies the maximum depth to search
  
   //holds the best score
	MoveValue best_score = createMoveValue(-100000);
    if (depth%2==1) best_score.score *= -1;

	MoveValue mv = createMoveValue(0);
   	for (int i=0;i<7;i++) { 
		//try putting a piece at each of 7 columns.
  		mv.col=i;
  		if(gameBoard.layout[0][i]!=-1)
   			continue; //if column is full ignore it.
    
  		//find the row to put this piece. x is the row
  		int x=0;
  		while( x < 8 && gameBoard.layout[x][i] == -1 ) x++;
  		gameBoard.layout[x-1][i] = whichPlayer;
 
  		//find if it is a game over, don't search further if it is
  		//a score of 100 or -100 means a game over
  		MoveValue cur = createMoveValue(0);
  		cur.score= calculateCurrentValue();

      	if( cur.score==100 || cur.score == -100 || depth==maxDepth )
       		mv.score = cur.score;
  		else {
			//search further (this one is the recursive call)
    		mv.score = findBestMove(((whichPlayer)?0:1),depth+1,maxDepth).score;
  			//3-sideToMove, gives you the number of the opponent. If sideToMove is 1
  			//3-sideToMove will be 2. If sideToMove is 2, 3-sideToMove is 1
		}	

  		//if this move is better, use it instead
  		if( mv.score > best_score.score && depth % 2 == 0 ) 
   			best_score = mv;
  		else if ( mv.score < best_score.score && depth % 2 == 1 )
   			best_score = mv;

  		//undo the move. Remove the piece you just put
  		x=0;
  		while( gameBoard.layout[x][i] == -1 )x++;
  			gameBoard.layout[x][i] = -1;
   } 
   return best_score;
 }

void initBoard(Board *b) {
	for (int r=0;r < ROWS; r++) {
		for (int c=0;c < COLS; c++) {
			b->layout[r][c] = -1;
		}
	}
}

float getPieceX(int col,int player) {
	return GUTTER + LINE_SIZE + col * STEP_SIZE + LINE_SIZE * col + PIECE_SPACE;
}

float getPieceY(int row,int player) {
	int yspace = 0;
	if ( row == 0 ) yspace = 1;
	if ( row == 5 ) yspace = -1;	
	return START_Y + LINE_SIZE + row * STEP_SIZE + LINE_SIZE * row + PIECE_SPACE + yspace;
}

void drawPiece(GContext* ctx,int col,int row,int player) {	
	float x = getPieceX(col,player);
	float y = getPieceY(row,player);
	graphics_context_set_stroke_color(ctx, GColorBlack);
	graphics_context_set_fill_color(ctx, GColorBlack);
	
	if ( player ) {
		y += STEP_SIZE;
		graphics_draw_circle(ctx, GPoint(x, y), PIECE_SIZE);		
	} else {
		graphics_fill_circle(ctx, GPoint(x, y), PIECE_SIZE);
	}
	
}

void setTurnLabel() {
	if ( whosTurn == 1 ) {
	  text_layer_set_text(&turnLayer,"My Turn");	
	} else {
	  text_layer_set_text(&turnLayer,"Your Turn");		
	}
}

void cpuTurn() {
	//
//	drop();
	/*
	if ( turnCount == 0 ) {
		gameBoard.layout[5][5] = 1;
		gameBoard.layout[5][4] = 1;
		gameBoard.layout[5][3] = 1;
		gameBoard.layout[4][4] = 1;
		gameBoard.layout[4][3] = 1;
		gameBoard.layout[3][3] = 1;
	} 
	*/
	MoveValue myMove = findBestMove(1,0,4);
	chosenCol = myMove.col;
	int row = findEmptyRow(chosenCol);
	if ( row != -1 ) drop(row);
}

void nextTurn() {
	if ( gameOver ) return;
	if ( whosTurn == 1 ) {
		whosTurn = 0;
		choosing = 1;
	} else {
		whosTurn = 1;
	}
	setTurnLabel();
	chosenCol = 0;
	layer_set_hidden(&chooseLayer, false);
	layer_mark_dirty(&chooseLayer);	
	layer_set_frame(&dropdownLayer, GRect(0,25,window.layer.frame.size.w,20));
	layer_set_hidden(&dropdownLayer, true);
	if ( whosTurn && !gameOver ) {
		timer_handle = app_timer_send_event(appContext, 1500 /* milliseconds */, CPU_TIMER);
	}
}

void handle_timer(AppContextRef ctx, AppTimerHandle handle, uint32_t cookie) {
  (void)ctx;
  (void)handle;

  if (cookie == CPU_TIMER) {
	cpuTurn();
  }
}


void drawDropdown(Layer *layer, GContext* ctx) {
	float x = getPieceX(chosenCol,whosTurn);
	float y = 5;
	graphics_context_set_stroke_color(ctx, GColorBlack);
	graphics_context_set_fill_color(ctx, GColorBlack);
	if ( !whosTurn ) {
		graphics_fill_circle(ctx, GPoint(x, y), PIECE_SIZE);
	} else {
		y += 45;
		graphics_draw_circle(ctx, GPoint(x, y), PIECE_SIZE);
	}
}

void drawChooser(Layer *layer, GContext* ctx) {
	float x = getPieceX(chosenCol,whosTurn);
	float y = 5;
	graphics_context_set_stroke_color(ctx, GColorBlack);
	graphics_context_set_fill_color(ctx, GColorBlack);
	if ( !whosTurn ) {
		graphics_fill_circle(ctx, GPoint(x, y), PIECE_SIZE);
	} else {
		y += 45;
		graphics_draw_circle(ctx, GPoint(x, y), PIECE_SIZE);
	}
}

void drawBoard(Layer *layer, GContext* ctx) {
	float x = 0;
	float stepSize = STEP_SIZE;
	graphics_context_set_stroke_color(ctx, GColorBlack);
	graphics_context_set_fill_color(ctx, GColorBlack);
	
	for(int i=0;i<(COLS+1);i++) {
		x = GUTTER + i * stepSize + i * LINE_SIZE;
		graphics_fill_rect(ctx,GRect(x,START_Y,LINE_SIZE,layer->frame.size.h-START_Y),0,GCornersAll);
	}
	
	//DRAW TOP LINE

	graphics_draw_line(ctx,GPoint(GUTTER,START_Y),GPoint(layer->frame.size.w-GUTTER-LINE_SIZE,START_Y));
	
	graphics_draw_line(ctx,GPoint(GUTTER,START_Y+LINE_SIZE),GPoint(layer->frame.size.w-GUTTER-LINE_SIZE,START_Y+LINE_SIZE));
	float y = 0;
	for(int i=1;i<ROWS;i++) {
		y = START_Y + i * stepSize + i * LINE_SIZE;
		graphics_fill_rect(ctx,GRect(GUTTER+LINE_SIZE,y,layer->frame.size.w-GUTTER*2-LINE_SIZE,LINE_SIZE),0,GCornersAll);
	}

	for(int row=0;row<ROWS;row++) {
		for (int col=0;col<COLS;col++) {
			if ( gameBoard.layout[row][col] == 1 || gameBoard.layout[row][col] == 0 )
				drawPiece(ctx,col,row,gameBoard.layout[row][col]);
				//drawPiece(ctx,col,row,(row+col)%2);
		}	
	}
}

void up_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
  (void)recognizer;
  (void)window;  
  //Not your turn, so buttons dont do anything
  if (whosTurn || chosenCol == 0 || !choosing || gameOver) return;  
  chosenCol--;
  layer_mark_dirty(&chooseLayer);
}

int findEmptyRow(int col) {
	for(int row=(ROWS-1);row>-1;row--) {
		if ( gameBoard.layout[row][col] == -1 ) return row;
	}
	return -1;
}

void down_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
  (void)recognizer;
  (void)window;
  //Not your turn, so buttons dont do anything
  if (whosTurn || chosenCol >= (COLS-1) || !choosing || gameOver) return;
  chosenCol++;
  layer_mark_dirty(&chooseLayer);
}

void awardWinner(int winner) {
	gameOver = 1;
	if ( winner ) text_layer_set_text(&turnLayer,"LOSER!!");
	else text_layer_set_text(&turnLayer,"WINNER!!");
}

void check_for_winner() {
	int whoWon = -1;
	for(int row=0;row<ROWS;row++) {
		for (int col=0;col<COLS;col++) {
			if ( have_a_winner(	check_for_winner_v(row,col),
								check_for_winner_h(row,col),
								check_for_winner_d(row,col),&whoWon) )
				return awardWinner(whoWon);
		}	
	}
}

void placePiece() {
	gameBoard.layout[rowToFill][chosenCol] = whosTurn;
	layer_set_hidden(&dropdownLayer, true);
	layer_mark_dirty(&boardLayer);
	check_for_winner();	
	return nextTurn();
}

void animation_stopped(Animation *animation, bool finished, void *data) {
	if (finished) return placePiece();	
}

void drop(int row) {
  choosing = 0;

  float stop = getPieceY(row,whosTurn);

  GRect to_rect = GRect(0, stop, 144, 30);

  rowToFill = row;	

  layer_set_hidden(&dropdownLayer, false);
  layer_mark_dirty(&dropdownLayer);	
  layer_set_hidden(&chooseLayer, true);
  property_animation_init_layer_frame(&prop_animation, 
	&dropdownLayer, 
	NULL,
	&to_rect
  );
  animation_set_handlers(&prop_animation.animation, (AnimationHandlers) {
        .stopped = (AnimationStoppedHandler) animation_stopped
      }, NULL );
  animation_schedule(&prop_animation.animation);
  
}

void select_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
  (void)recognizer;
  (void)window;
  //Not your turn or Game over, so buttons dont do anything
  if (whosTurn || gameOver) return;
  
  //check that colum is not full
  int row = findEmptyRow(chosenCol);
  if ( row == -1 ) return;


  //DROP
  drop(row);
}

void animation_started(Animation *animation, void *data) {

}


void reset() {
  	initBoard(&gameBoard);
	whosTurn = 1;
	chosenCol = 0;
	rowToFill = -1;
	waiting = 0;
	dropping = 0;
	dropRow = 0;
	choosing = 0;
	gameOver = 0;
	turnCount = 0;
	layer_mark_dirty(&boardLayer);
	nextTurn();
}

void select_long_click_handler(ClickRecognizerRef recognizer, Window *window) {
  (void)recognizer;
  (void)window;
  //Not your turn, so buttons dont do anything
  reset();

}


// This usually won't need to be modified

void handleClicks(ClickConfig **config, Window *window) {
  (void)window;

  config[BUTTON_ID_SELECT]->click.handler = (ClickHandler) select_single_click_handler;

  config[BUTTON_ID_SELECT]->long_click.handler = (ClickHandler) select_long_click_handler;

  config[BUTTON_ID_UP]->click.handler = (ClickHandler) up_single_click_handler;
  config[BUTTON_ID_UP]->click.repeat_interval_ms = 100;

  config[BUTTON_ID_DOWN]->click.handler = (ClickHandler) down_single_click_handler;
  config[BUTTON_ID_DOWN]->click.repeat_interval_ms = 100;
}

void handle_init(AppContextRef ctx) {
  (void)ctx;

  appContext = ctx;

  window_init(&window, "Window Name");
  window_stack_push(&window, true /* Animated */);
  resource_init_current_app(&APP_RESOURCES);

  GFont fontSmall = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SANSATION_LIGHT_21));

  text_layer_init(&turnLayer, GRect(0, 0, window.layer.frame.size.w, 27));
  text_layer_set_background_color(&turnLayer, GColorClear);
  text_layer_set_text_color(&turnLayer, GColorBlack);
  text_layer_set_font(&turnLayer, fontSmall);
  text_layer_set_text_alignment(&turnLayer, GTextAlignmentCenter);
  layer_add_child(&window.layer,&turnLayer.layer);

  layer_init(&chooseLayer, GRect(0,25,window.layer.frame.size.w,20));
  chooseLayer.update_proc = &drawChooser;
  layer_add_child(&window.layer, &chooseLayer);

  layer_init(&dropdownLayer, GRect(0,30,window.layer.frame.size.w,20));
  dropdownLayer.update_proc = &drawDropdown;
  layer_add_child(&window.layer, &dropdownLayer);


  layer_init(&boardLayer, GRect(0, 0, window.layer.frame.size.w, window.layer.frame.size.h));
  boardLayer.update_proc = &drawBoard;
  layer_add_child(&window.layer, &boardLayer);
  
  reset();

  window_set_click_config_provider(&window, (ClickConfigProvider) handleClicks);  
}

void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,
	.timer_handler = &handle_timer
  };
  app_event_loop(params, &handlers);
}
