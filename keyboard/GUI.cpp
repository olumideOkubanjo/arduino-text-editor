
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include <SD.h>
#include <TouchScreen.h>
#include <time.h>

#define SD_CS 10

// physical dimensions of the tft display (# of pixels)
#define DISPLAY_WIDTH  480
#define DISPLAY_HEIGHT 320

// touch screen pins, obtained from the documentaion
#define YP A3 // must be an analog pin, use "An" notation!
#define XM A2 // must be an analog pin, use "An" notation!
#define YM 9  // can be a digital pin
#define XP 8  // can be a digital pin

// dimensions of the part allocated to the map display
#define MAP_DISP_WIDTH (DISPLAY_WIDTH - 60)
#define MAP_DISP_HEIGHT DISPLAY_HEIGHT

// calibration data for the touch screen, obtained from documentation
// the minimum/maximum possible readings from the touch point
#define TS_MINX 100
#define TS_MINY 120
#define TS_MAXX 940
#define TS_MAXY 920

// thresholds to determine if there was a touch
#define MINPRESSURE   10
#define MAXPRESSURE 1000

//These are the colours in Hexadecimal
#define BLACK 0x0000
#define WHITE 0xFFFF
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0

MCUFRIEND_kbv tft;

// a multimeter reading says there are 300 ohms of resistance across the plate,
// so initialize with this to get more accurate readings
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

// different than SD
Sd2Card card;

enum Modes {
	start_screen=0, keyboard_view1=1, keyboard_view2=2 ,display=3, quit=4
};
int current_state= start_screen;


char keys[26]= {'q','w','e','r','t','y','u','i',
	'o','p','a','s','d','f','g','h',
	'j','k','l','z','x','c','v','b',
	'n','m'};

char Caps[5]={"Caps"};
char Spaces[10]={"Space bar"};
char enter_word[4] ="ENT";
char delet[5]={"del"};

char arrow[3]={"->"};

char arrow_backward[3]={"<-"};

char Select[7]={"Select"};
char Cut[4]={"Cut"};
char Copy[5]={"Copy"};
char Paste[6]={"Paste"};
char Find[5]= {"Find"};
char Replace[8]={"Replace"};
char Undo[5]={"Undo"};
char Redo[5]= {"Redo"};

bool caps_lock=false;

int old_cursor_x=0;
int old_cursor_y=0;

int cursor_x=0;
int cursor_y=20;

bool text_selected=0;

bool arrows_appear=false;

int text_location_x=160+47;
int text_location_y= 20/2;


void setup() {
	int r1=255;
	int g1=255;
	int b1= 255;

	init();
	Serial.begin(9600);
	uint16_t ID= tft.readID();
	tft.begin(ID);

	//the main menu
	tft.setRotation(1);
	tft.setCursor(0,320/3);
	tft.setTextSize(2);
	tft.fillScreen(BLACK);
	tft.println("Select Mode:");
	tft.setTextSize(1);
	
	while (current_state==start_screen) {

		TSPoint p= ts.getPoint();

		pinMode(YP,OUTPUT);
  		pinMode(XM,OUTPUT);
		
		int background= tft.color565(r1-=2,g1-=2,b1-=2);

		tft.setCursor(212,112);
		tft.setTextColor(background);
		tft.print("keyboard");

		tft.setCursor(212+112,112);
		tft.print("Display");

		tft.drawFastHLine(195,(320/3)-15,80,background);
		tft.drawFastHLine(195,(320/3)+30,80,background);
		tft.drawFastVLine(195,(320/3)-15,45,background);
		tft.drawFastVLine(195+80,(320/3)-15,45,background);

		tft.drawFastHLine(195+110,(320/3)-15,80,background);
		tft.drawFastHLine(195+110,(320/3)+30,80,background);
		tft.drawFastVLine(195+110,(320/3)-15,45,background);
		tft.drawFastVLine(195+80+110,(320/3)-15,45,background);

		if(p.z > MINPRESSURE) {
			int pixel_x = map(p.y,TS_MINX,TS_MAXX,480,0);
      		int pixel_y = map(p.x, TS_MAXY, TS_MINY, 0, 320);

      		if(pixel_x>195 && pixel_x<275 && pixel_y >(320/3)-15 && pixel_y < (320/3)+30) {

      			current_state=keyboard_view1;
      		}

      		if(pixel_x>(195+110) && pixel_x< (195+110+80) && pixel_y > (320/3)-15 && pixel_y < (320/3)+30) {

      			current_state = display;
      		}
		}

		delay(5);

	}
}

void draw_keys() {

	tft.setTextColor(WHITE);
	tft.fillScreen(BLACK);
	int spacing=80;
	for (int i=0; i<4;i++) {
		tft.drawFastHLine(0,spacing,480,WHITE);
		spacing+=60;
	}

	spacing=60;

	for (int i=0; i<8; i++) {
		tft.drawFastVLine(spacing,80,240,WHITE);
		spacing+=60;
	}

		//making the spacebar

	tft.fillRect(182,261,80,60,BLACK);

		//writing the keys.
	int x_position=-17;
	int y_position=90;

	tft.setCursor(x_position,y_position);
	tft.setTextSize(2);


	for(int i=0; i<26;i++) {
		if(i==8 || i==16 || i==24) {
				//going to a new line
			y_position+=60;
			x_position=-17;
		}

		x_position+=60;
		tft.setCursor(x_position,y_position);
		tft.print(keys[i]);
	}

	x_position+=25;
	tft.setCursor(x_position,y_position);
	tft.print(Caps);

	x_position+=60;
	tft.setCursor(x_position,y_position);
	tft.print(Spaces);

	x_position+=125;
	tft.setCursor(x_position,y_position);
	tft.print(enter_word);

	x_position+=60;
	tft.setCursor(x_position,y_position);
	tft.print(delet);

	x_position+=70;
	tft.setCursor(x_position,y_position);
	tft.print(arrow);

	cursor_x=0;
	cursor_y=20;
};

void key_pressed() {

	//NOTE: just send the index of the letters along with the state of capslock to 
	//know which character has been pressed. So send index and caps_lock

	TSPoint p= ts.getPoint();
	pinMode(YP,OUTPUT);
  	pinMode(XM,OUTPUT);

	bool pressed=0;

	TSPoint p2=ts.getPoint();
	pinMode(YP,OUTPUT);
	pinMode(XM,OUTPUT);

	while(p2.z>100) {
		p2=ts.getPoint();

		pinMode(YP,OUTPUT);
  		pinMode(XM,OUTPUT);

		pressed=1;
	}

	if (pressed) {
		int pixel_x = map(p.y,TS_MINX,TS_MAXX,480,0);

		pixel_x=pixel_x/60;

    	int pixel_y = map(p.x, TS_MAXY, TS_MINY, 0, 320);
    	pixel_y-=80;
    	pixel_y=pixel_y/60;

    	int offset_y= 8*int(pixel_y);

    	int index= pixel_x +offset_y;

    	if (index<=25) {

    		if (!caps_lock) {

    			tft.setCursor(cursor_x,cursor_y);
    			tft.print(keys[index]);

    			cursor_x+=12;

    			if(cursor_x==480) {
    				cursor_x=0;
    				cursor_y+=18;
    			} 
    		}

    		else {

    			tft.setCursor(cursor_x,cursor_y);
    			tft.print(char(keys[index]-32));
    			cursor_x+=12;

    			if(cursor_x==480) {
    				cursor_x=0;
    				cursor_y+=18;
    			} 
 
    		}
    	}

    	else if (index==26) {
    		if (!caps_lock) {

    			caps_lock=1;
    			tft.fillRect(120,260,60,60,WHITE);
    			tft.setTextColor(BLACK);
    			tft.setCursor(128,270);
    			tft.print(Caps);
    			tft.setCursor(cursor_x,cursor_y);
    			tft.setTextColor(WHITE);
    		}

    		else {
    			tft.fillRect(121,261,59,59,BLACK);
    			tft.setTextColor(WHITE);
    			tft.setCursor(128,270);
    			tft.print(Caps);
    			tft.setCursor(cursor_x,cursor_y);
    			caps_lock=0;
    		}
    	}

    	else if(index==27 || index==28) {

    		cursor_x+=12;
    		
    		if (cursor_x==480) {

    			cursor_x=0;
    			cursor_y+=18;
    		}
    	}

    	else if(index==30) {

    		if(cursor_x==0) {

    			if(old_cursor_x==0 && old_cursor_y==0) {

    				cursor_x=480;
    				cursor_y-=18;
    			}
    			else {
    				cursor_x=old_cursor_x;
    				cursor_y= old_cursor_y;
    				old_cursor_y=0;
    				old_cursor_x=0;
    			}
    		}

    		else {

    			//this is backspace so just send the index no need to send an ascii
    			cursor_x-=12;
    			tft.fillRect(cursor_x,cursor_y,12,18,BLACK);
    		}
    	}

    	else if(index==29) {

    		//this is the return carrier, so just send the index 29 no need for ascii

    		old_cursor_x=cursor_x;
    		old_cursor_y=cursor_y;

    		cursor_x=0;
    		cursor_y+=18;

    	}

    	else {
    		current_state=keyboard_view2;
    	}
    	
	}

};

void tool_bar() {

	int box_location_x= 160;
	int box_location_y=0;

	//here you might need to send an ascii cause the young god is running low on fuel to 
	//devise a smart ass solution like key pressed.

	tft.fillScreen(BLACK);

	int spacing=0;

	for(int i=0;i<9;i++) {
		tft.drawFastHLine(160,spacing,160,WHITE);
		spacing+=40;
	}

	 spacing=160;
	 for (int i=0;i<2;i++) {
		tft.drawFastVLine(spacing,0,320,WHITE);
		spacing+=160;
	 }

	tft.setCursor(text_location_x,text_location_y);
	tft.setTextSize(2);
	tft.setTextColor(WHITE);
	tft.print(Select);

	tft.setCursor(text_location_x,text_location_y+40);
	tft.print(Copy);

	tft.setCursor(text_location_x,text_location_y+80);
	tft.print(Paste);

	tft.setCursor(text_location_x,text_location_y+120);
	tft.print(Cut);

	tft.setCursor(text_location_x,text_location_y+160);
	tft.print(Find);

	tft.setCursor(text_location_x,text_location_y+200);
	tft.print(Replace);

	tft.setCursor(text_location_x,text_location_y+240);
	tft.print(Undo);

	tft.setCursor(text_location_x,text_location_y+280);
	tft.print(Redo);

	tft.drawFastHLine(0,0,40,WHITE);
	tft.drawFastHLine(0,40,40,WHITE);

	tft.drawFastVLine(40,0,40,WHITE);
	tft.drawFastVLine(0,0,40,WHITE);

	tft.setCursor(40/3,40/3);
	tft.print(arrow_backward);


};

void tools_pressed() {

	TSPoint p= ts.getPoint();
	pinMode(YP,OUTPUT);
  	pinMode(XM,OUTPUT);

	bool pressed=0;

	TSPoint p2=ts.getPoint();
	pinMode(YP,OUTPUT);
	pinMode(XM,OUTPUT);

	while(p2.z>100) {
		p2=ts.getPoint();

		pinMode(YP,OUTPUT);
  		pinMode(XM,OUTPUT);

		pressed=1;
	}

	if (pressed) {
		int pixel_x = map(p.y,TS_MINX,TS_MAXX,480,0);
    	int pixel_y = map(p.x, TS_MAXY, TS_MINY, 0, 320);

    	pixel_y=pixel_y/40;


    	if (pixel_y==0 && pixel_x>160 && pixel_x < 320) {

    		if(!text_selected) {

    			tft.fillRect(160,0,160,40,WHITE);
    			tft.setTextColor(BLACK);
    			tft.setCursor(text_location_x,text_location_y);
    			tft.print(Select);

    			tft.setTextColor(WHITE);

    			tft.drawFastVLine(160-40,0,40,WHITE);
    			tft.drawFastHLine(160-40,0,40,WHITE);
    			tft.drawFastHLine(160-40,40,40,WHITE);

    			tft.setCursor(text_location_x-87,text_location_y);

    			tft.print(arrow_backward);

    			tft.drawFastVLine(320+40,0,40,WHITE);
    			tft.drawFastHLine(320,0,40,WHITE);
    			tft.drawFastHLine(320,40,40,WHITE);
    			tft.setCursor(320+40/3,text_location_y);

    			tft.print(arrow);

    			arrows_appear=true;

    			text_selected=true;


    		}

    		else {
    			tft.fillRect(161,1,159,39,BLACK);
    			tft.setTextColor(WHITE);
    			tft.setCursor(text_location_x,text_location_y);
    			tft.print(Select);

    			tft.fillRect(160-41,0,40,41,BLACK);
    			tft.fillRect(321,0,40,41,BLACK);

    			text_selected=0;
    			arrows_appear=0;

    		}

    	}

    	if(arrows_appear) {

    		//write what you want to do with the arrows for selecting text. if you don't 

    		//get what I mean by that just call me.
    	}

    	else if(pixel_y==1 && pixel_x>160 && pixel_x < 320) {

    		//how you gonna copy cuz idk.
    	}

    	else if (pixel_y==2 && pixel_x>160 && pixel_x < 320) {

    		//how you gonna paste
    	}

    	else if (pixel_y==3 && pixel_x>160 && pixel_x < 320) {
    		//how you gonna cut
    	}

    	else if (pixel_y==4 && pixel_x>160 && pixel_x < 320) {
    		//how you gonna find
    	}

    	else if (pixel_y==5 && pixel_x>160 && pixel_x < 320) {
    		//how you gonna replace.
    	}

    	else if (pixel_y==6 && pixel_x>160 && pixel_x < 320) {

    		//how you gonna undo.
    	}

    	else if (pixel_y==0 && pixel_x>0 && pixel_x < 40) {

    		current_state=keyboard_view1;
    	}

    	else {
    		//what you want to do with redo
    	}
	}
}

int main() {

	setup();

	while (current_state!=quit) {

		if(current_state==keyboard_view1) {

			draw_keys();

			while(current_state==keyboard_view1) {

				key_pressed();

				delay(20);
			}
		}

		if (current_state== keyboard_view2) {

			tool_bar();

			while(current_state==keyboard_view2) {
				tools_pressed();
			}
		}

	}
	return 0;
}