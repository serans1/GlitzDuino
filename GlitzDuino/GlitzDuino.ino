#include <SoftwareSerial.h>

SoftwareSerial mySerial(2, 3); // RX, TX

char myChar;
int RedPin=11;
int BluePin=10;
int GreenPin=9;
void setup()
{
    Serial.begin(9600);

    //mySerial.begin(38400);
    //mySerial.begin(9600);
    //Set here your BLE module baud rate, default of HC0-6 is 9600, I used 1200. 
    mySerial.begin(1200);
    mySerial.print("AT");//just check OK recieved if we are in CMD mode 
    delay(1000);
    //write full spectrum flicker. 
     analogWrite(9,255);
     analogWrite(10,255);
     analogWrite(11,255);
}


String serial_cmd; 
String cmd; 

//the current wavelength, in nm 
int current_wavelength=0; 
//the wavelength we want to change to
int requested_wavelength=400;
//led parameters. 
//mode is fade in and out in normal mode and then flicker upon like level update. 
//the time to stay on after fade in 
int on_time=1000;
//the time to stay off after fade out 
int off_time=1000;
//the dimming pace
int dim_length=1000;
//the arduino step loop, affects fade resolution (smoothness)
int step_length=20; 
//flicker length - on time 
int flicker_on_time=50; 
//flicker length - off time 
int flicker_off_time=50;
//flicker count upon like level update 
int flicker_count=3;
//just counter on current state time 
int current_state_time=0;
typedef enum {IDLE, START, END} cmd_recieve_state;
cmd_recieve_state current_state = IDLE;
typedef enum {OFF, FADEIN, ON,FADEOUT} led_state;
led_state current_led_state = OFF;
typedef enum {SET_LIKE=5,SET_ON_TIME,SET_OFF_TIME,SET_DIM_TIME,SET_FLICKER_ON_TIME,SET_FLICKER_OFF_TIME,SET_FLICKER_COUNT} virtual_BLE_characteristic;
//process the commands as they are dewlivered through strings, real glitz will have custom BLE profile, now mimic BLE characteristics using virtual address/data
bool process_cmd(uint8_t  address,uint8_t  data)
{
   switch (address)
      {
        
        case SET_ON_TIME:
          on_time=data*100; 
          break;
        case SET_OFF_TIME:
          off_time=data*100; 
          break;
        case SET_DIM_TIME:
          dim_length=data*100; 
          break;  
        case SET_FLICKER_ON_TIME:
          flicker_on_time=data*10; 
          break; 
        case SET_FLICKER_OFF_TIME:
          flicker_off_time=data*10; 
          break;
        case SET_FLICKER_COUNT://like level update
          flicker_count=data; 
          break;
        case SET_LIKE:
          double likefactor=(double)data/100; 
          if (likefactor>1)
            likefactor=1;
          if (likefactor<0)
            likefactor=0;  
         requested_wavelength=380+ (645-380)* likefactor; 
         int new_rgb[3];
          waveLengthToRGB(requested_wavelength,new_rgb);
          for (int i=0;i<flicker_count;++i)
          {
            analogWrite(RedPin,0);
            analogWrite(BluePin,0);
            analogWrite(GreenPin,0);
            delay(flicker_off_time);
            analogWrite(RedPin,new_rgb[0]);
            analogWrite(BluePin,new_rgb[1]);
            analogWrite(GreenPin,new_rgb[2]);
            delay(flicker_on_time);
          }
          break; 
          
      } 
      //turn led off and update state for immidiet fadin
      analogWrite(RedPin,0);
      analogWrite(BluePin,0);
      analogWrite(GreenPin,0);
      current_led_state=FADEIN;
      current_state_time=0;
}
void loop() // run over and over
{
    //just for printing 
    if (current_wavelength!=requested_wavelength)
    {

      current_wavelength=requested_wavelength;
      Serial.print("Setting wavelength:");
      Serial.println(requested_wavelength);
      
    }
    //read anny data coming from the bluetooth-expect string in the form "(address,data)" 
    while ( mySerial.available() )
    { 
        
        myChar = mySerial.read();
        
        if (current_state == IDLE&&myChar=='(') //update command state to commnd is starting  
        {
          current_state = START;
        }
        if (current_state == START)
        {
          cmd+=myChar;
          if (myChar==')') //command has ended 
            current_state = END ;
        }
        //Serial.print(myChar);
         
 
    }
    //process incoing command 
    if (current_state == END)
    {
      char charBuf[50];
      cmd.toCharArray(charBuf, 50);
      uint8_t  address; 
      uint8_t data; 
      Serial.println(cmd);
      sscanf(charBuf, "(%hhu,%hhu)", &address, &data);
      cmd="";
       Serial.print ("Adress:");
       Serial.println(address);
       Serial.print ("Data:");
       Serial.println(data);
       //once uint extracted handle the address/data 
       process_cmd(address,data);
      current_state = IDLE; 
    }
    //debugger, for recieving commands to BLE from Arduino terminal 
    while ( Serial.available() )
    { 
        myChar = Serial.read();
        serial_cmd+=myChar;
        
    }
    if (serial_cmd!="")
    {
      Serial.println("Sending:"+serial_cmd);
      serial_cmd="";
    }
    //handle led state, factor is used to do the fade effect  
    double factor=1; 
    int rgb[3];
    //convert wavelength to rgb values 
    waveLengthToRGB(current_wavelength,rgb);
    //now handle led state according to clock and settings.
    if (current_led_state==OFF)
    {
        if (current_state_time>=off_time)
        {
          current_led_state=FADEIN;
          current_state_time=0;
        }
        factor=0;
    }
    if (current_led_state==FADEIN)
    {
        if (current_state_time>=dim_length)
        {
          current_led_state=ON;
          current_state_time=0;
        }
        else 
          factor=(double)current_state_time/dim_length;
    }
    if (current_led_state==ON)
    {
        if (current_state_time>=on_time)
        {
          current_led_state=FADEOUT;
          current_state_time=0;
        }
        else 
          factor=1;
    }
    if (current_led_state==FADEOUT)
    {
        if (current_state_time>=off_time)
        {
          current_led_state=OFF;
          current_state_time=0;
          factor=0;
        }
        else 
          factor=(double)(dim_length-current_state_time)/dim_length;
    }
    //write the actual pwm values 
    analogWrite(RedPin,factor*rgb[0]);
    analogWrite(BluePin,factor*rgb[1]);
    analogWrite(GreenPin,factor*rgb[2]);
    /*Serial.print("Setting RGB:");
    Serial.print(rgb[0]);
    Serial.print(rgb[1]);
    Serial.println(rgb[2]);
    Serial.print("Cycle time:");
    Serial.println(current_state_time);
    Serial.print("Factor:");
    Serial.println(factor);*/
    //tick tock 
    delay(step_length);
    current_state_time+=step_length;
    
    //mySerial.print("AT");      
}


double Gamma = 0.80;
double IntensityMax = 255;

/** Taken from Earl F. Glynn's web page:
* <a href="http://www.efg2.com/Lab/ScienceAndEngineering/Spectra.htm">Spectra Lab Report</a>
* */
void waveLengthToRGB(double Wavelength,int* rgb){
    double factor;
    double Red,Green,Blue;

    if((Wavelength >= 380) && (Wavelength<440)){
        Red = -(Wavelength - 440) / (440 - 380);
        Green = 0.0;
        Blue = 1.0;
    }else if((Wavelength >= 440) && (Wavelength<490)){
        Red = 0.0;
        Green = (Wavelength - 440) / (490 - 440);
        Blue = 1.0;
    }else if((Wavelength >= 490) && (Wavelength<510)){
        Red = 0.0;
        Green = 1.0;
        Blue = -(Wavelength - 510) / (510 - 490);
    }else if((Wavelength >= 510) && (Wavelength<580)){
        Red = (Wavelength - 510) / (580 - 510);
        Green = 1.0;
        Blue = 0.0;
    }else if((Wavelength >= 580) && (Wavelength<645)){
        Red = 1.0;
        Green = -(Wavelength - 645) / (645 - 580);
        Blue = 0.0;
    }else if((Wavelength >= 645) && (Wavelength<781)){
        Red = 1.0;
        Green = 0.0;
        Blue = 0.0;
    }else{
        Red = 0.0;
        Green = 0.0;
        Blue = 0.0;
    };

    // Let the intensity fall off near the vision limits

    if((Wavelength >= 380) && (Wavelength<420)){
        factor = 0.3 + 0.7*(Wavelength - 380) / (420 - 380);
    }else if((Wavelength >= 420) && (Wavelength<701)){
        factor = 1.0;
    }else if((Wavelength >= 701) && (Wavelength<781)){
        factor = 0.3 + 0.7*(780 - Wavelength) / (780 - 700);
    }else{
        factor = 0.0;
    };




    // Don't want 0^x = 1 for x <> 0
    rgb[0] = Red==0.0 ? 0 : (int) round(IntensityMax * pow(Red * factor, Gamma));
    rgb[1] = Green==0.0 ? 0 : (int) round(IntensityMax * pow(Green * factor, Gamma));
    rgb[2] = Blue==0.0 ? 0 : (int) round(IntensityMax * pow(Blue * factor, Gamma));

}
