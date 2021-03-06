#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include <SPI.h>
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
// prints a message to the bottom part of the tft screen

int cursor_x = 0; int cursor_y = 0;
int page_num = 1;
String file_names[20];



void status_message(const char* msg) {
  tft.fillRect(
    0, 0, DISPLAY_WIDTH,
    24, TFT_WHITE
  );

  tft.setTextColor(TFT_BLACK);
  tft.setCursor(5, 4);
  tft.setTextSize(2);

  tft.println(msg);
}



void setup() {

	init();
	Serial.begin(9600);
	Serial.flush();
	uint16_t ID= tft.readID();
	tft.begin(ID);
	
	tft.setRotation(1);
	
	tft.setTextSize(2);
	tft.fillScreen(BLACK);
	status_message("Screen Side: No File Selected");
	cursor_y = 24;
	tft.setCursor(cursor_x,cursor_y);

}

bool fillBuffer(String& l, unsigned long long timeout = 1000){
	//Receive a line from the Serial
  char buffer[20];
  uint8_t bufferLen = 0;

  char in_char;
  auto start = millis();
  while(timeout >= (millis()-start)){
    if(Serial.available()){
      in_char = Serial.read();
      buffer[bufferLen]=in_char;
      bufferLen++;
    }

    if(in_char == '\n' || in_char=='\r'){
      l = String(buffer);
      return true;
    }
  }
  return false;
}

void page_change(bool back = false){
	//change page of text
	tft.setTextWrap(true);
	tft.setCursor(0,0);
	cursor_x = 0;cursor_y = 0;

	String page;
	if(back && page_num != 1){
		//gotta move back
		page_num-=1;
	}
	else{
		//move forward to previously written page
		page_num+=1;
	}
	Serial.print("N ");
	Serial.println(page_num);

	if(fillBuffer(page)){
		int i =0;
		while(true){
			if(page[i] != '\n'){
				tft.print(page[i]);
			}
		}
	}
}

void cursor_forward(int text_size = 2){
//Increment cursor location
	int x_increment = text_size*6; int y_increment = text_size*8;
	cursor_x += x_increment;
	if(cursor_x +x_increment > DISPLAY_WIDTH){
		//if the next char will fall off the screen
		cursor_x = 0; cursor_y += y_increment;
	}
	tft.setCursor(cursor_x,cursor_y);
}

void cursor_back(int text_size = 2){
	//Decrement cursor location. If necessary wraparound back around the line without going into the header.
	int x_increment = text_size*6; int y_increment = text_size*8;
	if(cursor_x == 0){
		if(cursor_y >24){ 
			cursor_y -= y_increment;
		}
		else{
			cursor_x+=x_increment;
			if(page_num == 1){

			}
	}
	} 
	cursor_x = (cursor_x- x_increment)%DISPLAY_WIDTH;
	cursor_x = (cursor_x + DISPLAY_WIDTH)%DISPLAY_WIDTH;
	tft.setCursor(cursor_x,cursor_y);	

}

void receiving_mode(){
	/*This mode is responsible for receiving text and displaying it onto the screen while editing a text file.
	Note: It assumes that it should begin writing where cursor_x and cursor_y indicates.
	
	*/

	int text_size = 2;
	
	tft.setTextColor(TFT_WHITE);
	//tft.setTextWrap(true);
	tft.setTextSize(text_size);
	int x_increment = text_size*6; int y_increment = text_size*8;


	//Continuously read chars from Serial until we need to break outta here.
	bool breakout = false;
	while(!breakout){
		//at this point both cursors (tft and our vars) are at start of next char.
		if(Serial.available()){
			// read the incoming byte:
			int in_ascii = Serial.read();
			char in_char = in_ascii;



			if(in_char == '0'){ //enter
				cursor_x = 0;
				cursor_y += y_increment;
				tft.setCursor(cursor_x, cursor_y);
			}

			else if(in_char == '1'){//backspace.
				tft.fillRect(cursor_x,cursor_y,x_increment,y_increment,TFT_BLACK);
				cursor_back();
				tft.fillRect(cursor_x,cursor_y,x_increment,y_increment,TFT_BLACK);	
			}else if(in_char=='9'){//open or save. goto file manager.
				breakout = true;
				tft.fillScreen(TFT_BLACK);
				Serial.println("O");

				 
			}else{
      			//Received a char. Now print to screen being mindful of cursor location
      			//tft.setCursor(cursor_x,cursor_y);
      			char in_char = in_ascii;
      			tft.fillRect(cursor_x,cursor_y,x_increment,y_increment,TFT_BLACK);
      			tft.print(in_char);

      			cursor_forward();
			}

			Serial.println("S");
		}
		//cursor is updated,but still in loop. Lets draw cursor
		tft.print("|");
		tft.setCursor(cursor_x,cursor_y);
	}
}

bool get_files(int & num_of_files){
	//Get list of files from server using agreed upon protocol.
	String num_line;
	//int num_of_files;

	//get Num from server
	if(fillBuffer(num_line)){
		if(num_line[0] == 'N'){
			String num = num_line.substring(2);
			num_of_files = num.toInt();
			Serial.println("A");
		}
	}
	else{
		return 0;
	}

	//get list of files
	for(int i = 0;i<num_of_files+1;i++){
		String temp;
		if(fillBuffer(temp)){
			if(temp[0] == 'F'){
				file_names[i] = temp.substring(2);
				Serial.println("B");
			}
			
		}
		else{
			return 0;
		}
	}
	return 1;

}

bool select_file(){
	/*Getnames of files, display them and allow user to make selection
	Send selection back to server and load the file.
	*/

	//We'll comm with Server to get number of files
	status_message("Files: ");
	cursor_x = 0; cursor_y = 24;
	int text_size = 4;
	tft.setCursor(cursor_x,cursor_y);
	tft.setTextSize(text_size);
	
	tft.setTextColor(TFT_WHITE);
	int x_increment = text_size*6; int y_increment = text_size*8;

	int selection = 0;
	bool selected = false;
	int num_of_files = 0;

	//Get list of files WITH PROTOCOL 
	while(!get_files(num_of_files));
	

	//Given list and num of files display that shi and all that jazz


	for(int i = 0; i<num_of_files+1;i++){
		tft.print(file_names[i]);
		cursor_y+=y_increment;
	}
	cursor_y -=y_increment;
	tft.setCursor(cursor_x,cursor_y);
	tft.print("New File");
	cursor_y+=y_increment;
	

	while(!selected){
		//Take care of touching the screen
		TSPoint touch = ts.getPoint(); 
		pinMode(YP, OUTPUT);
		pinMode(XM, OUTPUT);
		if (touch.z > MINPRESSURE && touch.z < MAXPRESSURE){
			//int x_touch = map(touch.y, TS_MINX, TS_MAXX, tft.width(),0);
			int y_touch = map(touch.x, TS_MAXY, TS_MINY, 0, tft.height());
			//tft.println(x_touch);
			//tft.println(y_touch);
			//tft.println(y_touch);
			selection = (y_touch -24)/y_increment;
			selected = true;
			// tft.println(selection);

		}
		
	}
	// tft.println(num_of_files);
	
	
	
	//Send the selection back
	bool received = false;

	
	while(!received){
		
		Serial.print("N ");
		Serial.println(selection);
		String tempA;
		
		if(fillBuffer(tempA)){
			if(tempA[0] == 'A'){
				received = true;
			}
		}
		
	}

	
	if(selection == num_of_files){
		//If they decide to open a new txtfile
		tft.fillScreen(BLACK);
		status_message("New File Name? ");
		int text_size = 2;
		tft.setTextSize(text_size);
		tft.setTextColor(TFT_BLACK,TFT_WHITE);
		cursor_x = 15*12; cursor_y = 4;
		tft.setCursor(cursor_x,cursor_y);
		int x_increment = text_size*6; int y_increment = text_size*8;


		//get this name
		bool finished = false;
		while(!finished){
			if(Serial.available()){
				// read the incoming byte:
				int in_ascii = Serial.read();
				char in_char = in_ascii;
				//Received a char. Now print to screen being mindful of cursor location
      			//tft.setCursor(cursor_x,cursor_y);
      			if(in_ascii == 8){//backspace
					tft.fillRect(cursor_x,cursor_y,x_increment,y_increment,TFT_WHITE);
					cursor_back();
					tft.fillRect(cursor_x,cursor_y,x_increment,y_increment,TFT_WHITE);	
				}
				else if(in_char == '0'){
					//Done choosing name.

					finished = true;
					tft.setCursor(0,24);
					cursor_x = 0;
					cursor_y = 24;
				}
      			else{
      				
	      			//tft.fillRect(cursor_x,cursor_y,x_increment,y_increment,TFT_WHITE);
	      			tft.print(in_char);
	      			cursor_forward(2);
	      		}
			}
		}

		//The server should have the name at this point and created the file
		//We can start typing with the name of the file up at the top
		
		return 1;

	}

	else{
		//They chose an existing file
		// Move into receiving mode to load contents and allow editing.
		tft.fillScreen(TFT_BLACK);
		status_message("Editing...");
		cursor_x = 0; cursor_y = 24;
		tft.setCursor(cursor_x,cursor_y);

		return 1;
		
	}



	

}


int main() {

	setup();

	//Start receiving chars from the server
	while(true){
		Serial.flush();

		while(!select_file());

		//receiving_mode();
		receiving_mode();		
	}
	return 0;
}
