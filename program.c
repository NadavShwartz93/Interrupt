
#include <stdio.h>
#include <dos.h>

//This latch value create 18.2 clock ticks.
#define ORIGINAL_LATCH 65536


//Functions declaration:
void change_int8_interval(void);
void interrupt (*int8Save) (void);
void interrupt (*int9Save) (void);
void draw_arrow();
void setCursorPos();
void initialize_screen();
void interrupt myInt8(void);
void interrupt myInt9(void);


//Global veraible:
volatile int int8_counter = 0;
int latch;
int cursor_place;


//This method change Interrupt 8 interval (PIT) - 
//Port 40h - write the new latch value.
//Port 43h - command register that use channel 0.
void change_int8_interval(void) {
	
	asm{
		CLI
		PUSH	 AX
		PUSH	 BX 
		
		MOV		AL,36h			//36h = 00110110b
		OUT		43h,AL
		MOV		BX, latch		//change latch value
		MOV		AL,BL
		OUT		40h,AL
		MOV		AL,BH
		OUT		40h,AL
		
		POP		BX
		POP		AX
		STI
	 } // asm
	 
	 setvect(8, myInt8);

} //change_int8_interval


//This method is my Interrupt 8 handler.
void interrupt myInt8(void){
	int8_counter++;
	
	draw_arrow();
	
	int8Save();
} //myInt8


//This method is my Interrupt 9 handler.
void interrupt myInt9(void){
	char scanCode;
	char pres;
	
	//call to the older interrupt.
	int9Save();
	
	//get the last pressed keyboard scan code.
	//port 60h - read the keyboard scan code.
	//port 61h - informe the cpu that scan code is called.
	asm{
		PUSH AX

		IN AL,60h
		MOV scanCode,AL
		
		AND AL,80h
		MOV pres,AL
		
		IN AL,61h
		OR AL,80h
		OUT 61h,AL
		AND AL,7Fh
		OUT 61h,AL
		
		POP AX
	} //asm
	
	if(pres == 0){
		if(scanCode == 1){ //esc
			
			//retore clock ticks to the original state.
			latch = ORIGINAL_LATCH;
			change_int8_interval();
			
			//return the screen to previous state - 
			//stop writting to B800h segment.
			asm{
				PUSH AX
				
				MOV AH,0
				MOV AL,2
				INT 10h
				
				POP AX
			} //asm
			
			//restore Interrupts.
			setvect(8, int8Save);
			setvect(9, int9Save);
			
			exit(1);
		}
		
	} //if

} //myInt9


//This method set the cursor size to the maximum size.
//Port 3D4h - set cursor size (and the place).
void set_max_cursor(){
	
	asm{
		CLI
		PUSH AX
		PUSH DX
		
		MOV DX,3D4h
		MOV AX,00Ah
		OUT DX,AX
		MOV AX,0F0Bh
		OUT DX,AX
		
		POP DX
		POP AX
		STI
	} //asm
	
} //set_max_cursor


//This method set the cursor position on the screen.
//Port 3D4h - set cursor place (and the size).
void setCursorPos(){
	int x;
	if(cursor_place == 0)
		x = 0;
	else
		x = (cursor_place / 2) + 1;
	
	asm{
		PUSH AX
		PUSH DX
		PUSH BX
		
		MOV BX,x
		
		MOV DX,3D4h
		MOV AL,14
		MOV AH,BH
		OUT DX,AX
		
		MOV AL,15
		MOV AH,BL
		OUT DX,AX
		
		POP BX
		POP DX
		POP AX
	} //asm
	
	if(cursor_place == (2 * 25 * 40)){ //The screen is 25x40, and every picsel is in size of word (2 bytes).
		cursor_place = 0;
		initialize_screen();
		set_max_cursor();
	}
} //setCursorPos


//This method draw the arrow by writing to B800h memory segment.
void draw_arrow(){
	char c0,c1,c2;
	c0 = c1 = '-';
	c2 = '>';
	
	asm{
		PUSH AX
		PUSH ES
		PUSH DI
		
		MOV AX,0B800h
		MOV ES,AX
		MOV DI,cursor_place
		
		MOV AL,c0
		MOV AH,00010110b
		MOV ES:[DI],AX
		ADD DI,2
		
		MOV AL,c1
		MOV AH,00010110b
		MOV ES:[DI],AX
		ADD DI,2
		
		MOV AL,c2
		MOV AH,00010110b
		MOV ES:[DI],AX
		
		MOV cursor_place, DI
		
		POP DI
		POP ES
		POP AX
	} //asm
	
	setCursorPos();
	
} //draw_arrow


//This method initialize screen segment for the memory mapped IO.
void initialize_screen(){
	
	asm{	//textual mode
		CLI
		PUSH AX
		PUSH ES
		PUSH DI
		PUSH CX
		
		MOV AH,0
		MOV AL,1
		INT 10h
		
		MOV AX, 0B800h
		MOV ES,AX //ES SET TO screen segment for memory mapped IO
		MOV DI,0 //offset on the screen - start from the beginning
		MOV AL, ' '
		MOV AH, 00010011b ;//1 in the 4th bit is background blue		
		MOV CX ,1000
		CLD
		REP STOSW
		
		POP CX
		POP DI
		POP ES
		POP AX
		STI
	} //asm
	
} //initialize_screen


void main(){
	
	initialize_screen();
	
	set_max_cursor();
	
	latch = 0;
	
	cursor_place = 0;
	
	//Save the old interrupts.
	int8Save = getvect(8);
	int9Save = getvect(9);

	//Set the new interrupts.
	setvect(8, myInt8);
	setvect(9, myInt9);
	
	change_int8_interval();
	while(1);

} //main