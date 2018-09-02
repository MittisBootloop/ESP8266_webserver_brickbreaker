//You'll need Adafruit GFX & Adafruit SSD1331 libraries to run this game
//GFX lib Link:      https://github.com/adafruit/Adafruit-GFX-Library
//SSD1331 lib Link:  https://github.com/adafruit/Adafruit-SSD1331-OLED-Driver-Library-for-Arduino


#include <ESP8266WebServer.h>
// for the Display 
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1331.h>
#include <SPI.h>

// SSD1331 OLED Display Pins
#define sclk 14
#define mosi 13
#define cs   4
#define rst  15
#define dc   5

// Color definitions
#define BLACK           0x0000
#define BLUE            0x001F
#define RED             0xF800
#define GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0  
#define WHITE           0xFFFF

//Game variables_______________________________________

//paddle "speed" - paddle is moved by such many (wow) pixels in gameLoop
float pMove;
//paddles x-position (middle of the paddle)
float paddle;
//paddle length (total)
int pLength;
//x- and y- position of the ball
float ball[2];
//x- and y-speed of the ball, x/y is in-/decrease per gameLoop
float ballSpeed[2];
//ball will not move when false - start of each level
boolean gRun;
//lives/balls
int balls;
//for rainbow color of special brick
int rBr,gBr,bBr;
//for blinking of special bricks
int blinkSp;
//if true ball is faster
bool boost;
//if >0 ball won't change direction when hitting a brick
int superB;
//if true paddle will be greater
bool paddleP;
//count for bricks on Screen, if 0 - next level
int bCount;
//level count
int level;
//game speed - sets delay in gameLoop
int gSpeed;
//??? :D
int extraC;
//bricks[row][col][x,y,state,bonus]
int bricks[12][19][4];
//for debugging the game - run it full auto
//also uncomment parts in gameLoop & moveBall
//int xyz;
//--------------------------------------------------------

//Define name of the Wifi & password for creating an access point
const char* ssid = "abcabc";
//!!!Your password MUST be a minimum of 8 characters...otherwise neither password nor ssid will be accepted -> default or old ssid&pwd will show up!!! 
const char* password = "12341234";

ESP8266WebServer server(80);
Adafruit_SSD1331 tft = Adafruit_SSD1331(cs, dc, rst);

// HTML-Page - as raw String literal_______________________________________________________________________
// why the rawn literal? ...like so it's easier to write the HTML and Javascript code 
char webpage[] = R"=====(
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1">
</head>
<body>
<div style="background:#FFA500; width:800px; margin:10px; height:450px;">
    <!-- //Boxes for labeling -->
    <div style="background:#2222FF; width:50%; height:10%; float:left;">
      <h2 align="center">LEFT</h2>
    </div>
    <div style="background:#FF2222; width:50%; height:10%; float:right;">
      <h2 align="center">RIGHT</h2>
    </div>
    <!-- //Boxes for input -->
    <div class="boxL" id="boxL" style="background:#4444FF; width:50%; height:90%; float:left;"></div>
    <div class="boxR" id="boxR" style="background:#FF4444; width:50%; height:90%; float:right;"></div>
</div>
<div style="background:#000000; width:800px; margin:10px; height:150px;">
    <div style="background:#22FF22; width:100%; height:33%; float:right;">
      <h2 align="center">START</h2>
    </div>
    <div class="boxS" id="boxS" style="background:#44FF44; width:100%; height:100%;"></div>
</div>
<script>
window.addEventListener('load', function(){
  var bR = document.getElementById('boxR')
  var bL = document.getElementById('boxL')
  var bS = document.getElementById('boxS')
  var xhr = new XMLHttpRequest();

  //Device Orientation to control the pannel__________________________________
  //Tobozo has added this control via device orientation. He also has a version where he's
  //controlling the paddle with a MPU9250 attached to another ESP: https://youtu.be/0tYRnR2kAIQ
  //This might not work with all devices and browsers (Tobozo uses Firefox)
  //If you want to try it, just uncomment the following part 
  /*
  if (window.DeviceOrientationEvent) {

    function handleOrientation(event) {
      var x = event.beta;  // In degree in the range [-180,180]
      var y = event.gamma; // In degree in the range [-90,90]
      if(x>10) {
        orientation = 1;
      } else if(x<-10) {
        orientation = -1;
      } else {
        orientation = 0;
      }
      if(lastorientation!=orientation) {
        lastorientation = orientation;
        switch(orientation) {
          case 1:
            xhr.open("GET", "/rinc", true);
            xhr.send();
          break;
          case -1:
            xhr.open("GET", "/linc", true);
            xhr.send();
          break;
          case 0:
            xhr.open("GET", "/stop", true);
            xhr.send();
          break;
          default:
            xhr.open("GET", "/start", true);
            xhr.send();
          break;
        }
      }
    };

    window.addEventListener('deviceorientation', handleOrientation);
    
  }*/
  
  //RIGHT PC___________________________________________
  bR.onmousedown = function(){
    xhr.open("GET", "/rinc", true);
    xhr.send();
  } 
  bR.onmouseup = function(){
    xhr.open("GET", "/stop", true);
    xhr.send();
  }
  //RIGHT Mobile_______________________________________
  bR.addEventListener('touchstart', function(e){
    xhr.open("GET", "/rinc", true);
    xhr.send();
  }, false)
  bR.addEventListener('touchend', function(e){
    xhr.open("GET", "/stop", true);
    xhr.send();
  }, false)
  //LEFT PC____________________________________________
  bL.onmousedown = function(){
    xhr.open("GET", "/linc", true);
    xhr.send();
  } 
  bL.onmouseup = function(){
    xhr.open("GET", "/stop", true);
    xhr.send();
  }
  //LEFT Mobile________________________________________
  bL.addEventListener('touchstart', function(e){
    xhr.open("GET", "/linc", true);
    xhr.send();
  }, false)
  bL.addEventListener('touchend', function(e){
    xhr.open("GET", "/stop", true);
    xhr.send();
  }, false)
  //START PC____________________________________________
  bS.onmouseup = function(){
    xhr.open("GET", "/start", true);
    xhr.send();
  } 
  //START Mobile________________________________________
  bS.addEventListener('touchend', function(e){
    xhr.open("GET", "/start", true);
    xhr.send();
  }, false)
  //Key input LEFT(37)__RIGHT(38)______________________
  onkeydown = function(event) {
    if (event.keyCode == 37) {
      xhr.open("GET", "/linc", true);
      xhr.send();
    }
    if (event.keyCode == 39) {
      xhr.open("GET", "/rinc", true);
      xhr.send();
    }
  }
  onkeyup = function(event) {
    if (event.keyCode == 37||event.keyCode == 39) {
      xhr.open("GET", "/stop", true);
      xhr.send();
    }
    if (event.keyCode == 32) {
      xhr.open("GET", "/start", true);
      xhr.send();
    }
  }
}, false);
</script>
</body>
</html>
)=====";
//------------------------------------------------------------------------------------

//Run the HTML page on the server
void handleRoot() {
  server.send(200, "text/html", webpage);
}

void setup() {
  Serial.begin(9600);
  delay(200);
  //Wifi as access point_________________________________________________________
  WiFi.mode(WIFI_AP);
  IPAddress apLocalIp(2, 2, 2, 2);
  //IPAddress apGatewayIp(1, 1, 1, 1);-----------------
  IPAddress apSubnetMask(255, 255, 255, 0);
  //WiFi.softAPConfig(apLocalIp, apLocalIp, apSubnetMask);
  WiFi.softAPConfig(apLocalIp, apLocalIp, apSubnetMask);
  WiFi.softAP(ssid, password);
 //Server____________________________________________________________
  server.begin();
  server.on("/", handleRoot);
  server.on("/linc", pLeft);
  server.on("/rinc", pRight);
  server.on("/stop", pStop);
  server.on("/start", pStart);
 //Display___________________________________________________________
  pinMode(cs, OUTPUT);
  digitalWrite(cs, HIGH);
  tft.begin();
  tft.fillScreen(BLACK);
  //initializing the game____________________________________________
  //xyz=0;
  gSpeed=5;
  initGame();
  gameMenu();

}

void loop() {
  //to get the user input from the webpage
  server.handleClient();
  //to run/change the game depending on input
  gameLoop();
}

//Game_______________________________________________________________________________
//nothing to say
void gameLoop(){
  yield();
  if(balls>0){//running the game
    if(superB>0)superB-=1;
    movePaddle();
    drawPaddle((int)paddle, 60);
    drawSpBr(); 
    //draw ball black bevor moving it and drawing it again
    //otherwise i had to clear screen and redraw everything 
    //-> this causes flickering!--------------------------
    drawBall(BLACK);
    if(boost==1){ballSpeed[0]*=1.5;ballSpeed[1]*=1.5;}
    if(gRun)moveBall();
    if(superB==0)drawBall(WHITE);
    else drawBall(rainbowCol());
    //----------------------------------------------------
  }
  else if(balls==-5)writeToScreen("GAME OVER", 20,45, 1, CYAN);
  else if(balls==-10)writeToScreen(" YOU WON ", 20,45, 1, rainbowCol());
  else{//menu routine
    for(int i=0;i<93;i++){tft.drawPixel(i,58,  BLACK);}
    drawPaddle((int)paddle, 60);
    yield();
    drawBall(BLACK);
    moveBall();
    drawBall(WHITE);
  }
  /*if(!gRun){//starts level automatically(for debugging)
     xyz+=1;
     if(xyz==500){pStart();xyz=0;}
  }*/
  delay(gSpeed);
  yield();
}

//move & draw ball_________________________________________________________
//draws a single pixel as ball - ball array is read directly
//param cBall   color of the Ball
void drawBall(int cBall){
  tft.drawPixel((int)ball[0],(int)ball[1],  cBall);
}

int randNumber;
//moves the ball depending on the values of the ball speed array
void moveBall(){
  //changes direction, if the ball hits the borders or paddle
  if(ball[0]<=0||ball[0]>=95)ballSpeed[0]*=-1;//left/rigth
  if(ball[1]<=0)ballSpeed[1]*=-1;//top
  //hitting paddle
  if(ball[1]>=59&&(int)ball[0]==(int)paddle){                      //hidding paddle middle
    randNumber = random(-6,6);
    if(ballSpeed[0]>=0){ballSpeed[1]=-1;ballSpeed[0]= 0.1;}
    if(ballSpeed[0]<=0){ballSpeed[1]=-1;ballSpeed[0]=-0.1;}
  }
  else if(ball[1]>=59&&(int)ball[0]>=(int)paddle-pLength/2&&(int)ball[0]<=(int)paddle){       //hidding paddle left side
    randNumber = random(-6,6);
    ballSpeed[1]=-0.8;ballSpeed[0]=-0.6;  
  }
  else if(ball[1]>=59&&(int)ball[0]<=(int)paddle+pLength/2&&(int)ball[0]>=(int)paddle){      //hidding paddle right side
    randNumber = random(-6,6);
    ballSpeed[1]=-0.8;ballSpeed[0]= 0.6;  
  }
  else if(ball[1]>=59&&(int)ball[0]==(int)paddle-pLength/2-1){       //hidding paddle left side
    randNumber = random(-6,6);
    ballSpeed[1]=-0.6;ballSpeed[0]=-0.8;  
  }
  else if(ball[1]>=59&&(int)ball[0]==(int)paddle+pLength/2+1){       //hidding paddle right side
    randNumber = random(-6,6);
    ballSpeed[1]=-0.6;ballSpeed[0]= 0.8;  
  }
  else if(ball[1]>=59&&(int)ball[0]==(int)paddle-pLength/2-2){    //hidding paddle left edge
    randNumber = random(-6,6);
    ballSpeed[1]=-0.2;ballSpeed[0]=-1;
  }
  else if(ball[1]>=59&&(int)ball[0]==(int)paddle+pLength/2+2){    //hidding paddle right edge
    randNumber = random(-6,6);
    ballSpeed[1]=-0.2;ballSpeed[0]= 1;
  }
  else if(ball[1]>=59){//routine for missing the paddle
    ball[0]=paddle;
    ball[1]=58;
    gRun=false;
    balls-=1;
    if(balls==0)balls=-5;
    drawLives(balls);
    boost=0;
    superB=0;
    paddleP=0;
  }
  //set new ball position and check if bricks are hit afterwards
  ball[0]+=ballSpeed[0];
  ball[1]+=ballSpeed[1];
  /*if(gRun){// if uncomment the paddle will follow the ball (debugging)
    paddle=ball[0]+randNumber;
  }*/
  checkBaBr(ball[0],ball[1]);
}
//---------------------------------------------------------------------------------------

//move & draw paddle_____________________________________________________________________
//moves the paddle depending on speed
void movePaddle(){
  paddle=paddle+pMove;
  //if paddle violates borders reset to most left/right position
  if(paddle>=95-pLength/2)paddle=95-pLength/2;
  if(paddle<pLength/2)paddle=pLength/2;
  if(!gRun){//ball follows the paddle - if menu/level start/lost ball
    drawBall(BLACK);
    ball[0]=paddle;
    drawBall(WHITE);
  }
}

//draws paddle depending on it's value
//param x   x-position - inserted paddle variable
//param y   y-position - fixed
void drawPaddle(int x, int y){
  if(paddleP)pLength=11;
  else pLength=9;
  for(int i=0;i<96;i++){//needed this to avoid redrawing the whole screen (causes flickering)
    if(i>=x-pLength/2 && i<=x+pLength/2){
      tft.drawPixel(i,y,  tft.color565(170,170,170));
      tft.drawPixel(i,y+1,tft.color565(170,170,170));
    }
    else{
      tft.drawPixel(i,y,  BLACK);
      tft.drawPixel(i,y+1,BLACK);
    }
  }
}
//-------------------------------------------------------------------------------------------

//routine for cheking if the ball hits a brick
//param xBa,yBa   x- & y-position of the ball
//bricks array is read directly. guess what!
void checkBaBr(int xBa, int yBa){
  int state;
  //two for-loop to run through the array
  for(int r=0;r<12;r++){
    if(yBa>=r*3-1 && yBa<=r*3+2){
      for(int c=0;c<19;c++){
        yield();
        //read state of the actual brick
        state=bricks[r][c][2];
        if(state>0 && xBa>=5*c && xBa<=5*c+5){
          //Change state of the Brick in Array
          state-=1;
          bricks[r][c][2]=state;
          //check if special
          if(bricks[r][c][3]==1)paddleP=1;
          if(bricks[r][c][3]==2)superB=60;
          if(bricks[r][c][3]==3)boost=1;
          if(bricks[r][c][3]==4){balls+=1;drawLives(balls);}
          //Change Color or "Delete"(paint it black, like the red door) the Brick
          if(state==3)     drawSingleBr(bricks[r][c][0], bricks[r][c][1],YELLOW);
          else if(state==2)drawSingleBr(bricks[r][c][0], bricks[r][c][1],GREEN);
          else if(state==1)drawSingleBr(bricks[r][c][0], bricks[r][c][1],BLUE);
          else if(state==0){drawSingleBr(bricks[r][c][0], bricks[r][c][1],BLACK);bCount-=1;}
          if(superB==0){//if not super ball do this
            if( xBa==5*c ||xBa==5*c+5)ballSpeed[0]*=-1;
            if(yBa==r*3-1||yBa==r*3+2)ballSpeed[1]*=-1;
          }
          if(bCount==0){//if all bricks are deleted start new level
            //increase level counter by one
            level+=1;
            //set ball over paddle and stop
            ball[1]=58;
            gRun=0;
            if(level==2)initBricksL2();
            if(level==3)initBricksL3();
            if(level==4)initBricksL4();
            if(level==5)initBricksL5();
            if(level==6)initBricksL6();
            if(level==7)initBricksL7();
            if(level==8)initBricksL8();
            if(level==9)initBricksM();
            if(level==10)balls=-10;
            tft.fillScreen(BLACK);
            //bricks and lives are not drawed every run in loop, so let's do it here
            drawBricks();
            drawLives(balls);
          }
        }
      }
    }
  }
}

//Input to controll the game from webpage___________________________________________________
//start function - is called by start field on webpage/space
void pStart(){
  yield();
  if(gRun)return;
  if(balls==-10){//is called when all levels have been beaten, will start menu
    balls=-4;
    paddle==12.0;
    drawPaddle((int)paddle, 60);
    gameMenu();
    delay(300);
  }
  if(balls==-5){//is called when game over, will start menu
    balls+=1;
    paddle==12.0;
    drawPaddle((int)paddle, 60);
    gameMenu();
    delay(300);
  }
  else if(balls==-4){
    //is called in menu, will start level1 and 
    //set amount of balls depending paddle position
    ballSpeed[0]=0;
    ballSpeed[1]=-1;
    while(ball[1]>33){
    delay(10);
    drawBall(BLACK);
    moveBall();
    drawBall(WHITE);
    }
    if(paddle==12){balls=6;gSpeed=10;}
    else if(paddle==46){balls=4;gSpeed=8;}
    else {balls=2;gSpeed=6;}
    initGame();
    delay(300);
  }
  else{
    //sets ballspeed depending on paddle position
    //starts moving the ball (gRun=true)
    ballSpeed[1]=0.75;
    if(paddle>46)ballSpeed[0]=-0.75;
    else ballSpeed[0]= 0.75;
    gRun=true;
  }
  server.send(200);
  yield();
}

//paddle left - is called by left field on webpage/left arrow
void pLeft(){
  yield();
  if(balls==-4){//in menu, paddle jumps between three dificulties
    if(paddle==46)paddle=12;
    else if(paddle==82)paddle=46;
    ball[0]=paddle;
  }
  else pMove=-1; //in game, paddle speed is
  server.send(200);
  yield();
}

//paddle right - is called by right field on webpage/right arrow
void pRight(){
  yield();
  if(balls==-4){//in menu, paddle jumps between three dificulties
    if(paddle==12)paddle=46;
    else if(paddle==46)paddle=82;
    ball[0]=paddle;
  }
  else pMove=1; //in game, paddle speed is
  server.send(200);
  yield();
}

//paddle stop - is called when öeft/right field on webpage/right arrow are released
void pStop(){
  yield();
  pMove=0;
  server.send(200);
  yield();
}
//--------------------------------------------------------------------------------------

//initialize most of the variables 
void initGame(){
  tft.fillScreen(BLACK);
  initBricksL1();
  level=1;
  rBr=250;
  gBr=0;
  bBr=0;
  ball[0]=47;
  ball[1]=58;
  ballSpeed[0]=0.75;
  paddle = 47;
  pMove=0;
  gRun=false;
  randNumber=0; //for debugging
  drawBricks();
  drawLives(balls);
}

//every time when called in a loop returned color will change slightly
//init one color with 250, others with 0 before first call
//return a rgb565 color as uint16 
uint16_t rainbowCol(){
  if(bBr<250&&rBr==250&&gBr==0)bBr+=10;
  else if(bBr==250 &&rBr>0&&gBr==0)rBr-=10;
  else if(rBr==0&&gBr<250&&bBr==250)gBr+=10;
  else if(gBr==250&&bBr>0&&rBr==0)bBr-=10;
  else if(rBr<250&&bBr==0&&gBr==250)rBr+=10;
  else if(rBr==250&&bBr==0&&gBr>0)gBr-=10;
  return tft.color565(rBr,gBr,bBr);
}

//drawing Bricks (from array and special)_____________________________________________________
//drawing one special brick (except rainbow)
//param x & y   x-,y-postion of the brick
//param col1/2  colors for blinking/splited brick
//param num     for blinking change num between 0&1, for splited num =5
void drawSiSpBr(int x, int y, int col1, int col2, int num){
  if(num==0){
    drawSingleBr(x,y,col1);
  }
  if(num==1){
    drawSingleBr(x,y,col2);
  }
  if(num==5){
    for(int i=0;i<8;i++){
      if(i<4) tft.drawPixel(x+i,  y,  col1);
      if(i>=4)tft.drawPixel(x+i-4,y+1,col2);
    }
  }
}

//this function is called in game loop!
//drawing all special bricks depending on value of level
//level is read direct(ly ?!), because it's global. why no param...because reasons! :D
void drawSpBr(){
  uint16_t col=rainbowCol();
  //variable for blinking bricks
  blinkSp+=1;
  if(blinkSp==10)blinkSp=0;
  if(level==1){
    //if brick is not deleted (!=0) draw it
    if(bricks[10][5][2]!=0)drawSiSpBr(bricks[10][5][0], bricks[10][5][1],WHITE,BLACK,blinkSp/3);
    if(bricks[7][15][2]!=0)drawSiSpBr(bricks[7][15][0], bricks[7][15][1],RED,BLUE,blinkSp/5);
    if(bricks[5][9][2]!=0)drawSiSpBr(bricks[5][9][0], bricks[5][9][1],RED,CYAN,5);
    }
  if(level==2){
    if(bricks[9][2][2]!=0)drawSiSpBr(bricks[9][2][0], bricks[9][2][1],WHITE,BLACK,blinkSp/3);
    if(bricks[11][5][2]!=0)drawSingleBr(bricks[11][5][0], bricks[11][5][1],col);
    if(bricks[11][15][2]!=0)drawSingleBr(bricks[11][15][0], bricks[11][15][1],col);
    if(bricks[7][13][2]!=0)drawSiSpBr(bricks[7][13][0], bricks[7][13][1],RED,CYAN,5);
  }
  if(level==3){
    if(bricks[8][5][2]!=0)drawSiSpBr(bricks[8][5][0], bricks[8][5][1],WHITE,BLACK,blinkSp/3);
    if(bricks[7][15][2]!=0)drawSiSpBr(bricks[7][15][0], bricks[7][15][1],RED,BLUE,blinkSp/5);
    if(bricks[5][4][2]!=0)drawSiSpBr(bricks[5][4][0], bricks[5][4][1],RED,CYAN,5);
    if(bricks[2][11][2]!=0)drawSingleBr(bricks[2][11][0], bricks[2][11][1],col);
  }
  if(level==4){
    if(bricks[3][5][2]!=0)drawSiSpBr(bricks[3][5][0], bricks[3][5][1],WHITE,BLACK,blinkSp/3);
    if(bricks[5][15][2]!=0)drawSiSpBr(bricks[5][15][0], bricks[5][15][1],RED,BLUE,blinkSp/5);
    if(bricks[7][4][2]!=0)drawSiSpBr(bricks[7][4][0], bricks[7][4][1],RED,CYAN,5);
    if(bricks[9][11][2]!=0)drawSingleBr(bricks[9][11][0], bricks[9][11][1],col);
  }
  if(level==5){
    if(bricks[2][5][2]!=0)drawSiSpBr(bricks[2][5][0], bricks[2][5][1],WHITE,BLACK,blinkSp/3);
    if(bricks[6][15][2]!=0)drawSiSpBr(bricks[6][15][0], bricks[6][15][1],RED,BLUE,blinkSp/5);
    if(bricks[7][4][2]!=0)drawSiSpBr(bricks[7][4][0], bricks[7][4][1],RED,CYAN,5);
    if(bricks[8][11][2]!=0)drawSingleBr(bricks[8][11][0], bricks[8][11][1],col);
  }
  if(level==6){
    if(bricks[1][5][2]!=0)drawSiSpBr(bricks[1][5][0], bricks[1][5][1],WHITE,BLACK,blinkSp/3);
    if(bricks[5][15][2]!=0)drawSiSpBr(bricks[5][15][0], bricks[5][15][1],RED,BLUE,blinkSp/5);
    if(bricks[7][3][2]!=0)drawSiSpBr(bricks[7][3][0], bricks[7][3][1],RED,CYAN,5);
    if(bricks[9][11][2]!=0)drawSingleBr(bricks[9][11][0], bricks[9][11][1],col);
  }
  if(level==7){
    if(bricks[2][5][2]!=0)drawSiSpBr(bricks[2][5][0], bricks[2][5][1],WHITE,BLACK,blinkSp/3);
    if(bricks[6][15][2]!=0)drawSiSpBr(bricks[6][15][0], bricks[6][15][1],RED,BLUE,blinkSp/5);
    if(bricks[4][4][2]!=0)drawSiSpBr(bricks[4][4][0], bricks[4][4][1],RED,CYAN,5);
    if(bricks[8][11][2]!=0)drawSingleBr(bricks[8][11][0], bricks[8][11][1],col);
  }
  if(level==8){
    if(bricks[5][4][2]!=0)drawSiSpBr(bricks[5][4][0], bricks[5][4][1],WHITE,BLACK,blinkSp/5);
    if(bricks[6][4][2]!=0)drawSiSpBr(bricks[6][4][0], bricks[6][4][1],WHITE,BLACK,blinkSp/5);
    if(bricks[5][14][2]!=0)drawSingleBr(bricks[5][14][0], bricks[5][14][1],col);
    if(bricks[6][14][2]!=0)drawSingleBr(bricks[6][14][0], bricks[6][14][1],col);  
  }
  if(level==9){
    extraC+=1;
    if(bricks[3][7][2]!=0)drawSiSpBr(bricks[3][7][0], bricks[3][7][1],WHITE,BLACK,blinkSp/3);
    if(bricks[4][7][2]!=0)drawSiSpBr(bricks[4][7][0], bricks[4][7][1],WHITE,BLACK,blinkSp/3);
    if(bricks[3][16][2]!=0)drawSingleBr(bricks[3][16][0], bricks[3][16][1],col);
    if(bricks[4][16][2]!=0)drawSingleBr(bricks[4][16][0], bricks[4][16][1],col);
    //what kind of cruelty is this!?
    if(extraC==400){
      extraC=0;
      return;
      int randBrick = random(228);
      //Serial.println(randBrick);
      for(int r=0;r<12;r++){
        yield();
        for(int c=0;c<19;c++){
          extraC+=1;
          if(extraC-1==randBrick){
            if(bricks[r][c][2]==0)bCount+=1;
            bricks[r][c][2]=1+(randBrick%4);
            drawSingleBr(bricks[r][c][0], bricks[r][c][1],MAGENTA);
            break;
          }
        }
      }    
      extraC=0;
    }  
  }
}

//drawing one single brick
//param x/y     x- & y-postion of brick (top left corner)
//param color   color the brick will be drawed in
void drawSingleBr(int x, int y, int color){
  for(int i=0;i<8;i++){
    if(i<4) tft.drawPixel(x+i,  y,  color);
    if(i>=4)tft.drawPixel(x+i-4,y+1,color);
  }
}

//loop for drawing all bricks in the array
void drawBricks(){
  int x;
  int y;
  int state;
  for(int r=0;r<12;r++){
    yield();
    for(int c=0;c<19;c++){
      x=    bricks[r][c][0];
      y=    bricks[r][c][1];
      state=bricks[r][c][2];
      //color depends on bricks individual state
      if(state==4)     drawSingleBr(x,y,RED);
      else if(state==3)drawSingleBr(x,y,YELLOW);
      else if(state==2)drawSingleBr(x,y,GREEN);
      else if(state==1)drawSingleBr(x,y,BLUE);
      else if(state==0)drawSingleBr(x,y,BLACK);//if state is 0, there is no brick
    }
  }
}
//-------------------------------------------------------------------------------

//draws the amount of balls you've to play 
//minus 1...the ball that's actual on the paddle
void drawLives(int lives){
  if(lives<=0)return;
  for(int i=0;i<lives-1;i++){
    uint16_t color;
    if(i==0)color=RED;
    else if(i==1)color=YELLOW;
    else if(i==2)color=GREEN;
    else if(i>2)color=BLUE;
    else color=BLACK;
    tft.drawPixel(5*i+3,63,color);
    tft.drawPixel(5*i+4,63,color);
    tft.drawPixel(5*i+5,63,color);
  }
  for(int i=0;i<14-lives;i++){
    tft.drawPixel(63-5*i,63,BLACK);
    tft.drawPixel(63-5*i+1,63,BLACK);
    tft.drawPixel(63-5*i+2,63,BLACK);
  }
}


/*
//smaller number - hight of two bricks
const boolean numbers[10][20]={{0,0,1,0, 0,1,1,0, 1,0,1,0, 0,0,1,0, 0,0,1,0},
                                  {0,1,1,0, 1,0,0,1, 0,0,1,0, 0,1,0,0, 1,1,1,1},
                                  {0,1,1,0, 1,0,0,1, 0,0,1,0, 1,0,0,1, 0,1,1,0},
                                  {0,0,1,1, 0,1,0,1, 1,0,0,1, 1,1,1,1, 0,0,0,1},
                                  {1,1,1,1, 1,0,0,0, 1,1,1,0, 0,0,0,1, 1,1,1,0},
                                  {0,1,1,1, 1,0,0,0, 1,1,1,0, 1,0,0,1, 0,1,1,0},
                                  {1,1,1,1, 0,0,0,1, 0,0,1,0, 0,1,0,0, 0,1,0,0},
                                  {0,1,1,0, 1,0,0,1, 0,1,1,0, 1,0,0,1, 0,1,1,0},
                                  {0,1,1,0, 1,0,0,1, 0,1,1,1, 0,0,0,1, 1,1,1,1},
                                  {0,1,1,0, 1,0,0,1, 1,0,0,1, 1,0,0,1, 0,1,1,0}};
//to write the above numbers to the screen
//param x/y     set the "cursor"-position
//param number  number that will be written
void drawNum(int x, int y, int number){
  for(int r=0;r<5;r++){
    for(int c=0;c<4;c++){
      if(numbers[number][r*4+c])tft.drawPixel(x+c,y+r,CYAN);
    }
  }
}*/

//param color   color for the game over "logo(?!)"
//void gameOver(uint16_t color){
  
  /*tft.setCursor(20,45);
  tft.setTextColor(color);  
  tft.setTextSize(1);
  tft.print("GAME OVER");*/
//}

//setCursor-, setTextColor-, setTextSize-, print-Function combined in one function
//param - should be self explaining
void writeToScreen(String s, int cursorX, int cursorY, float txtSize, uint16_t color){
  tft.setCursor(cursorX,cursorY);
  tft.setTextColor(color);  
  tft.setTextSize(txtSize);
  tft.print(s);
}

//draws the game menu
void gameMenu(){
  tft.fillScreen(BLACK);
  for(int i=0;i<96;i++){
    for(int j=0;j<11;j++){
      if((i>25 && i<28) || (i>66 && i<69))tft.drawPixel(i+1,23+j,BLACK);
      else if(i<26)tft.drawPixel(i,23+j,GREEN);
      else if(i<67)tft.drawPixel(i,23+j,YELLOW);
      else tft.drawPixel(i,23+j,RED);  
    }
  }
  writeToScreen("EASY",1,25,1,BLACK);
  writeToScreen("MEDIUM",30,25,1,BLACK);
  writeToScreen("HARD",71,25,1,BLACK);
  //initialize paddle and ball for menu
  pLength=9;
  paddle=12;
  ball[0]=12;
  ball[1]=58;
  ballSpeed[0]=0;
  ballSpeed[1]=0;
  balls=-4;//set to -4 - so game knows it's in menu
}

//Init Array for levels_________________________________________________________________
//in each level the special bricks are set individual
void initBricksL1(){
  initBricksFull();
  bricks[10][5][3]=1;//paddle+
  bricks[7][15][3]=3;//faster
  bricks[5][9][3]=4;//live+
  bricks[10][5][2]=1;
  bricks[7][15][2]=1;
  bricks[5][9][2]=1;
}

void initBricksL2(){
  initBricksFull();
  bricks[9][2][3]=1;
  bricks[11][5][3]=2;//super ball
  bricks[11][15][3]=2;//super ball
  bricks[7][13][3]=4;
  bricks[9][2][2]=1;
  bricks[11][5][2]=1;
  bricks[11][15][2]=1;
  bricks[7][13][2]=1;
  tft.fillScreen(BLACK);
  drawBricks();
}

//initializing the array for level 1&2
void initBricksFull(){
  bCount=0;
  for(int r=0;r<12;r++){
    yield();
    for(int c=0;c<19;c++){
      bricks[r][c][0]=1+c*5;
      bricks[r][c][1]=r*3;
      bricks[r][c][2]=4-(r % 4);
      bricks[r][c][3]=0;
      bCount+=1;
    }
  }
}

void initBricksL3(){
  bCount=0;
  for(int r=0;r<12;r++){
    yield();
    for(int c=0;c<19;c++){
      bricks[r][c][0]=1+c*5;
      bricks[r][c][1]=r*3;
      if(r==0||r==1||c==0||c==1||c==18||c==17||c==8||c==9||c==10){
      bricks[r][c][2]=0;
      bricks[r][c][3]=0;
      }
      else{
      bricks[r][c][2]=((r+c)%4)+1;
      bricks[r][c][3]=0;
      bCount+=1;
      }
    }
  }
  bricks[8][5][3]=1;//paddle+
  bricks[7][15][3]=3;//faster
  bricks[5][4][3]=4;//live+
  bricks[2][11][3]=2;//super ball
  bricks[8][5][2]=1;
  bricks[7][15][2]=1;
  bricks[5][4][2]=1;
  bricks[2][11][2]=1;
}
void initBricksL4(){
  bCount=0;
  for(int r=0;r<12;r++){
    yield();
    for(int c=0;c<19;c++){
      bricks[r][c][0]=1+c*5;
      bricks[r][c][1]=r*3;
      if(r%2==0){
      bricks[r][c][2]=0;
      bricks[r][c][3]=0;
      }
      else{
      bricks[r][c][2]=(c%4)+1;
      bricks[r][c][3]=0;
      bCount+=1;
      }
    }
  }
  bricks[3][5][3]=1;//paddle+
  bricks[5][15][3]=3;//faster
  bricks[7][4][3]=4;//live+
  bricks[9][11][3]=2;//super ball
  bricks[3][5][2]=1;
  bricks[5][15][2]=1;
  bricks[7][4][2]=1;
  bricks[9][11][2]=1;
}

void initBricksL5(){
  bCount=0;
  for(int r=0;r<12;r++){
    yield();
    for(int c=0;c<19;c++){
      bricks[r][c][0]=1+c*5;
      bricks[r][c][1]=r*3;
      if((r+c)%4==0||(r+c)%4==1){
      bricks[r][c][2]=0;
      bricks[r][c][3]=0;
      }
      else{
      bricks[r][c][2]=(r%4)+1;
      bricks[r][c][3]=0;
      bCount+=1;
      }
    }
  }
  bricks[2][5][3]=1;//paddle+
  bricks[6][15][3]=3;//faster
  bricks[7][4][3]=4;//live+
  bricks[8][11][3]=2;//super ball
  bricks[2][5][2]=1;
  bricks[6][15][2]=1;
  bricks[7][4][2]=1;
  bricks[8][11][2]=1;
  bCount+=1;
}

void initBricksL6(){
  bCount=0;
  for(int r=0;r<12;r++){
    yield();
    for(int c=0;c<19;c++){
      bricks[r][c][0]=1+c*5;
      bricks[r][c][1]=r*3;
      if(r%2==0||c%2==0){
      bricks[r][c][2]=0;
      bricks[r][c][3]=0;
      }
      else{
      bricks[r][c][2]=(bCount%4)+1;
      bricks[r][c][3]=0;
      bCount+=1;
      }
    }
  }
  bricks[1][5][3]=1;//paddle+
  bricks[5][15][3]=3;//faster
  bricks[7][3][3]=4;//live+
  bricks[9][11][3]=2;//super ball
  bricks[1][5][2]=1;
  bricks[5][15][2]=1;
  bricks[7][3][2]=1;
  bricks[9][11][2]=1;
}

void initBricksL7(){
  bCount=0;
  for(int r=0;r<12;r++){
    yield();
    for(int c=0;c<19;c++){
      bricks[r][c][0]=1+c*5;
      bricks[r][c][1]=r*3;
      if(r%2==0||c%2==0){
      bricks[r][c][2]=(r%4)+1;
      bricks[r][c][3]=0;
      bCount+=1;
      }
      else{
      bricks[r][c][2]=0;
      bricks[r][c][3]=0;
      }
    }
  }
  bricks[2][5][3]=1;//paddle+
  bricks[6][15][3]=3;//faster
  bricks[4][4][3]=4;//live+
  bricks[8][11][3]=2;//super ball
  bricks[2][5][2]=1;
  bricks[6][15][2]=1;
  bricks[4][4][2]=1;
  bricks[8][11][2]=1;
}

void initBricksL8(){
  bCount=0;
  for(int r=0;r<12;r++){
    yield();
    for(int c=0;c<19;c++){
      bricks[r][c][0]=1+c*5;
      bricks[r][c][1]=r*3;
      bricks[r][c][3]=0;
      if((r==0||r==11) && c>=2 && c<=6){
      bricks[r][c][2]=3;
      bCount+=1;
      }
      else if((c==0||c==8) && r>=2 && r<=9){
      bricks[r][c][2]=3;
      bCount+=1;
      }
      else if((r==0||r==11) && c>=12 && c<=16){
      bricks[r][c][2]=4;
      bCount+=1;
      }
      else if((c==10||c==18) && r>=2 && r<=9){
      bricks[r][c][2]=4;
      bCount+=1;
      }
      else if(r==3||r==4){
        if(c==2||c==3||c==5||c==6){bricks[r][c][2]=3;bCount+=1; }
        if(c==12||c==13||c==15||c==16){bricks[r][c][2]=4;bCount+=1; }
      }
      else if(r==8){
        if(c==3||c==4||c==5){bricks[r][c][2]=3;bCount+=1; }
        if(c==12||c==13||c==14||c==15||c==16){bricks[r][c][2]=4;bCount+=1; }
      }
      else{
      bricks[r][c][2]=0;
      }
    }
  }
  bricks[1][1][2]=3;
  bricks[10][1][2]=3;
  bricks[1][7][2]=3;
  bricks[10][7][2]=3;
  bricks[1][11][2]=4;
  bricks[10][11][2]=4;
  bricks[1][17][2]=4;
  bricks[10][17][2]=4;
  bricks[7][2][2]=3;
  bricks[7][6][2]=3;
  bCount+=14;
  bricks[5][4][3]=1;//
  bricks[6][4][3]=1;//faster
  bricks[5][14][3]=2;//
  bricks[6][14][3]=2;//super ball
  bricks[5][4][2]=1;
  bricks[6][4][2]=1;
  bricks[5][14][2]=1;
  bricks[6][14][2]=1;
}

void initBricksM(){
  bCount=0;
  extraC=0;
  for(int r=0;r<12;r++){
    yield();
    for(int c=0;c<19;c++){
      bricks[r][c][0]=1+c*5;
      bricks[r][c][1]=r*3;
      if((c==1 || c==5 || c==3 || c==10 || c==13) && (r!=11 && r!=0)){
        if(c==1 || c==5 || c==3)bricks[r][c][2]=4;
        if(c==10 || c==13)bricks[r][c][2]=2;
        bricks[r][c][3]=0;
        bCount+=1;
      }
      else if(r==4 && c>=9 && c <=14){
        bricks[r][c][2]=2;
        bricks[r][c][3]=0;
        bCount+=1;
      }
      else if((c==7 || c==16) && r>=6 && r<=10){
        bricks[r][c][2]=3;
        bricks[r][c][3]=0;
        bCount+=1;
      }
      else{
      bricks[r][c][2]=0;
      bricks[r][c][3]=0;
      }
    }
  }
  bricks[1][3][2]=0;
  bricks[2][2][2]=4;
  bricks[2][3][2]=0;
  bricks[2][4][2]=4;
  bricks[10][11][2]=2;
  bricks[10][14][2]=2;
  bricks[3][7][2]=1;
  bricks[4][7][2]=1;
  bricks[3][16][2]=1;
  bricks[4][16][2]=1;
  bricks[3][7][3]=1;
  bricks[4][7][3]=1;
  bricks[3][16][3]=2;
  bricks[4][16][3]=2;
  bCount+=6;
}
//---------------------------------------------------------------------------
