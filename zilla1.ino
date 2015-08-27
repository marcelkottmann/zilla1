#include <EEPROM.h>
#include <SPI.h>
#include <FreqMeasure.h>

const int slaveSelectPinFreq = 10;
const int slaveSelectPinVol = 9;

const int pitchPin0 = 7;
const int pitchPin1 = 6;
const int pitchPin2 = 5;

const int potPin1 = A1;
const int potPin2 = A2;

const int frequencyCalibrationChannel = 1;

const int PAUSE=0;
const int C=1;
const int Cis=2;
const int D=3;
const int Dis=4;
const int E=5;
const int F=6;
const int Fis=7;
const int G=8;
const int Gis=9;
const int A=10;
const int Ais=11;
const int H=12;

float tunes[] = {261.6, 277.2, 293.7, 311.1, 329.6, 349.2, 370, 392, 415.3, 440, 466.2, 493.9};

const int numOctaves=5;

const int MAX_TONES=12*numOctaves;
byte vals[MAX_TONES];
byte ozzoctav[MAX_TONES];
byte calibri[MAX_TONES];

int noteOnManualValReference[]={0,0};
int manualpitch=0;
int manualpitch2=0;

int rechteck=0;
int loggingCounter=0;
volatile int duplicateBeatRequest=0;
volatile int capturedBeatRequest=0;
long lastMidiCheck=millis();


//int melody[] = {
//C, 1, 1, E, 1, 1 , G, 1, 1, H, 1, 1  ,C, 2, 1,E, 2, 1, G, 2, 1, H, 2, 1,C, 3, 1, H, 2, 1,G, 2, 1,E, 2, 1, H, 1, 1,G, 1, 1,E, 1, 1,
//};
int melody[] = {
C, 2, 2, C, 2, 2, G, 2, 2, C, 2, 2, PAUSE, 0,3, H, 1, 1, PAUSE, 0,1, H, 2, 1,PAUSE, 0,1, H, 3, 1,
C, 3, 2, C, 3, 2, G, 3, 2, C, 3, 2, PAUSE, 0,3, H, 3, 1, PAUSE, 0,1, H, 2, 1,PAUSE, 0,1, H, 3, 1,
C, 1, 2, C, 1, 2, G, 1, 2, C, 1, 2, PAUSE, 0,3, H, 2, 1, PAUSE, 0,1, H, 1, 1,PAUSE, 0,1, H, 0, 1,
};
//int melody[] = {C, 1,32};

const int numSteps=sizeof(melody)/sizeof(melody[0]);

long lastbeat=0;
int currentStep=0;
long beatcounter=0;
long keypressed=0;
long keyreleased=0;


//some predefined adsr functions
long off[]={0,0,0,0};
long clikk[]={10,300,0,0};
long adsr2[]={100,250,240,700};
long adsr[]={100,200,240,700};
long long_attack[]={500,200,240,300};
long mountain[]={2000,2000,240,2000};
long full[]={200,200,240,300};

long squar_ch0[]={5000,1000,240,2000};
long s01_ch0[]={10,500,255,2000};
long s02_ch0[]={10,500,255,2000};
long fp_ch0[]={0,300,0,9999999}; // 0 -> low freq, 255 -> high freq
long fl_ch0[]={0,0,200,9999999};  //255 -> low, 0 -> high
long pitchin_ch0[]={0,0,255,9999999};
long pwm_ch0[]={0,0,255,9999999}; // 0,0,255,9999999 => 50/50 duty cycle, 0,0,20,9999999 ~> 10/90 duty cycle

long squar_ch1[]={5,100,240,20};
long s01_ch1[]={10,500,240,20};
long s02_ch1[]={10,500,240,20};
long fp_ch1[]={0,0,200,9999999}; // 0 -> low freq, 255 -> high freq
long fl_ch1[]={0,0,0,9999999};  //255 -> low, 0 -> high
long pitchin_ch1[]={0,0,255,9999999};
long pwm_ch1[]={0,0,200,9999999}; // 0,0,255,9999999 => 50/50 duty cycle, 0,0,20,9999999 ~> 10/90 duty cycle

long squar_ch2[]={5,50,0,0};
long s01_ch2[]={5,50,0,20};
long s02_ch2[]={5,50,0,20};
long fp_ch2[]={0,0,100,9999999}; // 0 -> low freq, 255 -> high freq
long fl_ch2[]={0,0,0,9999999};  //255 -> low, 0 -> high
long pitchin_ch2[]={0,0,255,9999999};
long pwm_ch2[]={0,0,200,9999999}; // 0,0,255,9999999 => 50/50 duty cycle, 0,0,20,9999999 ~> 10/90 duty cycle
  
  
//funcs:
//index 0: adsr of square wave
//index 1: adsr of sine wave
//index 2: adsr of triangle wave
//index 3: adsr of filter param (should be equal to index 4)
//index 4: adsr of filter param (should be equal to index 3)
//index 5: adsr of filter level
//index 6: adsr of pitch in
//index 7: adsr of pwm
int funcs_size=8;
long* funcs_ch0[]={squar_ch0,s01_ch0,s02_ch0,fp_ch0,fp_ch0,fl_ch0, pitchin_ch0, pwm_ch0};
long* funcs_ch1[]={squar_ch1,s01_ch1,s02_ch1,fp_ch1,fp_ch1,fl_ch1, pitchin_ch1, pwm_ch1};
long* funcs_ch2[]={squar_ch2,s01_ch2,s02_ch2,fp_ch2,fp_ch2,fl_ch2, pitchin_ch2, pwm_ch2};
long** funcs[]={funcs_ch0, funcs_ch1, funcs_ch2};

float mix_ch0[]={0.2, 1, 1, 1, 1, 1, 1, 1 };
float mix_ch1[]={0.8, 0.5, 0.5, 1, 1, 1, 1, 1 };
float mix_ch2[]={1, 1, 1, 1, 1, 1, 1, 1 };
float* mix[]={mix_ch0, mix_ch1, mix_ch2 };



int channel[]={0,2,4,1,3,5};
long release_reference_current_values[]={0,0,0,0,0,0,0,0};
byte current_values[]={0,0,0,0,0,0,0,0};
float tremolo1[]={0.1,0.0021,   0.1,0.0021,        0.1,0.0021,   0.1,0.0015,   0.1,0.0015,   0,0,      0,0.0005,    0.1,0.0001};
//float tremolo1[]={0,0,   0,0,        0,0,   0,0,   0,0,   0,0,            0,0, 0,0};
float rund=0.2;

volatile boolean is_beat=false;
volatile boolean request_beat=false;
long last=0;

int beat_length=120;
volatile int external_beat=false;  
long external_beat_length=beat_length;
int divider_counter=0;
long last_external_beat=0;
int noteOn = 144;
int noteOff = 128;
int midi_channel=1;
byte currentNote=0;
byte currentPitch=0;
byte currentVelocity=127;
boolean midi_enabled=false;
long pitchBend=0;
long pitchBendTarget=0;

byte selectedParam=0;
int frequencyCalibration=10;
int dutyCycleCalibration=190;

void setup() {  
  Serial.begin(115200);
  Serial.println("Startup...");   

  pinMode (slaveSelectPinFreq, OUTPUT);
  pinMode (slaveSelectPinVol, OUTPUT);

  pinMode (pitchPin0, OUTPUT);
  pinMode (pitchPin1, OUTPUT);
  pinMode (pitchPin2, OUTPUT);
  digitalWrite(pitchPin0,LOW);
  digitalWrite(pitchPin1,LOW);
  digitalWrite(pitchPin2,LOW);
    
  //input for external sync/beat
  pinMode (3, INPUT);

  SPI.begin(); 

  FreqMeasure.begin();

  //staticVoltageTest();


  //frequency calibration
  ozz(slaveSelectPinFreq, frequencyCalibrationChannel, frequencyCalibration);

  // duty cycle
  ozz(slaveSelectPinFreq, 2, dutyCycleCalibration);
  ozz(slaveSelectPinFreq, 3, dutyCycleCalibration+manualpitch);


  int tune_version=2;
  
  if (tune_version==0 || EEPROM.read(0)!=tune_version) {  
    Serial.println("Start tuning oszillator 0...");   
  
   //turn off other oszillator
   
    //ozz(slaveSelectPinFreq, 1, 0);
    //digitalPotWrite(slaveSelectPinVol, 1, 255);
    tune3(0);
  
//    Serial.println("Start tuning oszillator 1...");   
    
    //turn off other oszillator
  //  ozz(slaveSelectPinFreq, 0, 0);
  //  digitalPotWrite(slaveSelectPinVol, 0, 255);
  //  tune(1);
  
    Serial.println("Tuning finished."); 
    EEPROM.write(0,tune_version);
  }
  else
  {
    int m=1;
    for (int pitch=0; pitch<numOctaves;pitch++)
    {
      for (int tit=0; tit<12; tit++)
      {
        ozzoctav[tit+12*pitch]=EEPROM.read(3*m);
        vals[tit+12*pitch]=EEPROM.read(3*m+1);
        calibri[tit+12*pitch]=EEPROM.read(3*m+2);
        m++;
      }
    }
  }

  Serial.println("attach sync interrupt..."); 
  attachInterrupt(1, requestBeat, RISING);
  Serial.println("finished."); 
}

void requestBeat()
{
  capturedBeatRequest++; 
  if (request_beat)
  {
     duplicateBeatRequest++; 
  }
  request_beat=true;
  external_beat=true;
}

void checkMidi(long now){
  long now1=millis();
  
  byte commandByte;
  byte noteByte;
  byte velocityByte;

  if (Serial.available()){
    if (!midi_enabled)
    {
      external_beat=true;
      keypressed=0;
      keyreleased=now;
      midi_enabled=true;
    }
    
    commandByte = Serial.read();//read first byte
    
    if (commandByte>=noteOn && commandByte<(noteOn+16))
    {
      midi_channel=commandByte-noteOn;
      commandByte=noteOn;
    } 
    else if (commandByte>=noteOff && commandByte<(noteOff+16))
    {
      midi_channel=commandByte-noteOff;
      commandByte=noteOff;
    }
    
    if (commandByte==noteOn || commandByte==noteOff || commandByte==224 || commandByte==176)
    {
        do 
        {
        }
        while (Serial.available()<2);
        
        noteByte =  Serial.read();
        velocityByte =  Serial.read();
      
      if (commandByte==noteOn && velocityByte>0)
      {
          velocityByte=min(velocityByte*4,127);
      }
    }
    
    if (commandByte==noteOn && velocityByte>0)
    {
       currentNote=(noteByte-48)%12+1;
       currentPitch=1+(noteByte-48)/12;
       currentVelocity=velocityByte;
       keyreleased=0;
       keypressed=now; 

       noteOnManualValReference[0]=analogRead(potPin1);
       noteOnManualValReference[1]=analogRead(potPin2);
  
  Serial.println(millis()-now1);

  
  }
    else if (currentNote==(noteByte-48)%12+1 && currentPitch==1+(noteByte-48)/12 && (commandByte==noteOff || (commandByte==noteOn && velocityByte==0)))
    {
       keyreleased=now;
       keypressed=0; 
    } 
    else if (commandByte==224)
    {
      long pb=velocityByte;
      pb = pb << 7;
      pb = pb | noteByte;
      pitchBendTarget=((pb-8192)*20)/8192;
    }
    else if (commandByte==176)
    {
      byte starto=0;
      byte endo=1;
      byte startf=0;
      byte endf=2;

      switch(selectedParam)
      {
         case 0:
          break; 
         case 1:
          endo=0;
          break;
         case 2:
          starto=1;
          break;
         default:
         starto = endo = (velocityByte-2)/3;
         startf = endf = (velocityByte-2)%3;
         break;
      }
      
      if (noteByte==74) //select oszillator and waveform
      {
        selectedParam=velocityByte;
      }
      
      for (int osz=starto; osz<=endo; osz++)
      {
        for (int func=startf; func<=endf; func++)
        {
          if (noteByte==71)//mix volume
          {
            mix[midi_channel][osz*3+func]=velocityByte/127.;          
          }
          else if (noteByte==81)//tremolo depth
          {
            tremolo1[osz*6+func*2]=velocityByte/127.;          
          }
          else if (noteByte==91)//tremolo rate
          {
            tremolo1[osz*6+func*2+1]=velocityByte/500.;          
          }
          else if (noteByte==16)//attack (A)
          {
            funcs[midi_channel][osz*3+func][0]=velocityByte*10;          
          }
          else if (noteByte==80)//decay (D)
          {
            funcs[midi_channel][osz*3+func][1]=velocityByte*10.;          
          }
          else if (noteByte==19)//sustain (S)
          {
            funcs[midi_channel][osz*3+func][2]=velocityByte*2;          
          }
          else if (noteByte==2)//release (R)
          {
            funcs[midi_channel][osz*3+func][3]=velocityByte*100;          
          }
        }
      }
    }
  }
}

void setOzzOctav(byte ozzoctav)
{
  digitalWrite(pitchPin0, ozzoctav & B00000001);
  digitalWrite(pitchPin1, (ozzoctav>>1) & B00000001);
  digitalWrite(pitchPin2, (ozzoctav>>2) & B00000001);
}

float measureFrequency(int precision)
{
  double sum = 0;
  int count = 0;
  while(count < precision)
  {
    if (FreqMeasure.available()) {
      // average several reading together
      sum = sum + FreqMeasure.read();
      count++;
    } 
  }
  return FreqMeasure.countToFrequency(sum / count);
}


void tune3(int oszillator)
{
  Serial.print("Finding lowest possible frequency:");
  ozz(slaveSelectPinFreq, oszillator, 0);
  setOzzOctav(0);
  ozz(slaveSelectPinFreq, frequencyCalibrationChannel, 255);
  float frequency = measureFrequency(50);
  Serial.println(frequency);

  Serial.print("Finding highest possible frequency:");
  ozz(slaveSelectPinFreq, oszillator, 255);
  setOzzOctav(7);
  ozz(slaveSelectPinFreq, frequencyCalibrationChannel, 0);
  frequency = measureFrequency(50);
  Serial.println(frequency);
        

  float resval = 0;
  byte calibrati = 255;
  byte ozzoctave=0;


  ozz(slaveSelectPinFreq, oszillator, resval);
  ozz(slaveSelectPinFreq, frequencyCalibrationChannel, calibrati);
  setOzzOctav(ozzoctave);

  //turn up volume
  digitalPotWrite(slaveSelectPinVol, oszillator, 0);

  int tp=0;

  int m=1;  
  int lowPrecision=5;
  int highPrecision=50;
  for (int pitch=0; pitch<numOctaves;pitch++)
  {  
    int tones_it=0; 
    for (int tones_it=0; tones_it<12;tones_it++)
    {  
      boolean tuned=false; 
      int precision=lowPrecision;
      while (!tuned)
      {
        setOzzOctav(ozzoctave);
        float frequency = measureFrequency(precision);
        float targetfreq=tunes[tones_it]/((16.)/(pow(2.,pitch)));
            
        float diff = frequency - targetfreq;
          //if (abs(diff)<targetfreq*tolerance)
        if (abs(diff)<=0.6 && precision==lowPrecision)
        {
          precision=highPrecision;
          Serial.println("Switching to high precision measurement.");
        } 
        else if (abs(diff)<=0.5 && precision==highPrecision)
        {
          vals[tones_it+12*pitch]=round(resval);
          ozzoctav[tones_it+12*pitch]=ozzoctave;
          calibri[tones_it+12*pitch]=calibrati;
          Serial.println("tuned:");   
          Serial.println("pitch:");
          Serial.println(pitch);
          Serial.println("tone:");
          Serial.println(tones_it);
          Serial.println("calibri:");
          Serial.println(calibri[tones_it+12*pitch]);
          Serial.println("ozzoctav:");
          Serial.println(ozzoctav[tones_it+12*pitch]);
          Serial.println("tune:");
          Serial.println(vals[tones_it+12*pitch]);

          //persist
          EEPROM.write(3*m,ozzoctave);
          EEPROM.write(3*m+1,round(resval));
          EEPROM.write(3*m+2,calibrati);
          m++;

          //fasten tuning
          resval=resval*pow(2.,1./12.);

          tuned=true;
        }
        else
        {          
//              resval=max(0,min(255,resval-diff/2.));
          float oldresval=resval;
          resval=resval*0.8 + resval*0.2*targetfreq/frequency;
          if (round(resval)==round(oldresval))
          {
              if (resval<oldresval)
              {
                resval-=1.0;
              }  
              else 
              {
                resval+=1.0;
              }
          }
          resval=max(0,min(255,resval));
          
          ozz(slaveSelectPinFreq, oszillator, round(resval));
          ozz(slaveSelectPinFreq, frequencyCalibrationChannel, calibrati);
          
          if (round(resval)==0 && ozzoctave>0)
          {
            if (calibrati<255)
            {
              calibrati++;  
            }
            else
            {
              ozzoctave--;
              calibrati=0;
              resval=230;
            }
          }
          else if (round(resval)==255 && ozzoctave<7)
          {
            if (calibrati>0)
            {
              calibrati--;  
            }
            else
            {
              ozzoctave++;
              calibrati=255;
              resval=20;
            }
          }

          Serial.print("--- TUNING tone ");
          Serial.print((tones_it+1)+pitch*12);
          Serial.print("/");
          Serial.print(numOctaves*12);
          Serial.println(" ---");
          Serial.println("precision:");
          Serial.println(precision);  
          Serial.println("measured:");
          Serial.println(frequency);  
          Serial.println("diff:");          
          Serial.println(diff);            
          Serial.println("ozzoctave:");
          Serial.println(ozzoctave);          
          Serial.println("calibrati:");
          Serial.println(calibrati);          
          Serial.println("tune:");
          Serial.println(resval);                                       
        }
      }    
    }
  }
}


void volumeTest()
{
  byte val=0;
  while(true)
  {
    ozz(slaveSelectPinFreq, 0, 128);    
    digitalPotWrite(slaveSelectPinVol, 0, val);
    val++;
    delay(2);
  }
}

void staticVoltageTest()
{
  while(true)
  {
    ozz(slaveSelectPinFreq, 0, 0);    
    digitalPotWrite(slaveSelectPinVol, 0, 0);
    ozz(slaveSelectPinFreq, 1, 38);    
    digitalPotWrite(slaveSelectPinVol, 1, 0);
  }
}


void loop() {
  //volumeTest();
  
/*  if (loggingCounter++==10000)
  {
    Serial.println("captured beat requests:");
    Serial.println(capturedBeatRequest); 
    Serial.println("duplicate beat requests:");
    Serial.println(duplicateBeatRequest); 
    loggingCounter=0;
  }
*/

  //duty cycle
  ozz(slaveSelectPinFreq, 2, dutyCycleCalibration);
  ozz(slaveSelectPinFreq, 3, dutyCycleCalibration+manualpitch);

  //manual pitch2
  //ozz(slaveSelectPinFreq, 1, frequencyCalibration+manualpitch2);


  for (int i=0; i<2; i++)
  {
    int manualVal=0;
    byte manualFunc=0;
    
    if (i==0)
    {    
      manualVal=analogRead(potPin1);
      manualFunc=1;
    } else if (i==1)
    {    
      manualVal=analogRead(potPin2);
      manualFunc=16;
    }
    
    if (manualFunc==1) //pwm
    {
      funcs[midi_channel][7][2]=manualVal/4;
    } 
    else if (manualFunc==2) //filter param
    {
      funcs[midi_channel][3][2]=manualVal/4; 
      funcs[midi_channel][4][2]=manualVal/4; 
    }
    else if (manualFunc==3) //filter level
    {
      funcs[midi_channel][5][2]=manualVal/4; 
    }
    else if (manualFunc>=4 && manualFunc<=6) //mix
    {
      mix[midi_channel][manualFunc-4]=manualVal/1023.0; 
    }
    else if (manualFunc==7) //single mix 
    {
      float x=(manualVal/1023.0)*7.;
      float mixx0=x-3.;
  
      float mixx1=x-1.;
      if (x>=3 && x<=5.5)
      {
         mixx1=5-x; 
      }
      else if (x>=5.5)
      {
        mixx1=x-6.;
      }
  
      float mixx2=x;
      if (x>=1.5 && x<=4)
      {
         mixx2=3-x; 
      }
      else if (x>4)
      {
        mixx2=x-5.;
      }
      
      mix[midi_channel][0]=min(1., max (0, mixx0)); 
      mix[midi_channel][1]=min(1., max (0, mixx1)); 
      mix[midi_channel][2]=min(1., max (0, mixx2)); 
      //mix[0]=1.;
      //mix[1]=1.;
      //mix[2]=1.;
    }
    else if (manualFunc==8) //glaetten 
    {
      rund=manualVal/1024.;
    }  
    else if (manualFunc==9) //attack of filter param
    {
      funcs[midi_channel][3][0]=manualVal/4;
    }  
    else if (manualFunc==10) //deacy of filter param
    {
      funcs[midi_channel][3][1]=manualVal/4;
    }  
    else if (manualFunc==11) //sustain of filter param
    {
      funcs[midi_channel][3][2]=manualVal/4;
    }  
    else if (manualFunc==12) //ads of filter param
    {
      funcs[midi_channel][3][0]=manualVal/4;
      funcs[midi_channel][3][1]=funcs[midi_channel][3][0];
      funcs[midi_channel][3][2]=funcs[midi_channel][3][0];
    }  
    else if (manualFunc==13) //ads of square
    {
      funcs[midi_channel][0][0]=manualVal/4;
      funcs[midi_channel][0][1]=funcs[midi_channel][0][0];
      funcs[midi_channel][0][2]=funcs[midi_channel][0][0];
    }  
    else if (manualFunc==14) //attack of pitchin
    {
      funcs[midi_channel][6][0]=manualVal;
    }  
    else if (manualFunc==15) //manualpitch
    {
      manualpitch=manualVal-noteOnManualValReference[i];
    }  
    else if (manualFunc==16) //manualpitch2
    {
      manualpitch2=manualVal-noteOnManualValReference[i];
    }  
  }
  
  long now=millis();
  
  
  if (!external_beat)
  {
    is_beat=(now-lastbeat)>beat_length;
    if (is_beat)
    {
      lastbeat=now-((now-lastbeat)%beat_length);
    }
  } 
  else
  {    
    if (request_beat)
    {
      long elength=now-last_external_beat;
      if (elength>100)
      {
        external_beat_length=elength;
        divider_counter=0;
        is_beat=true;  
        last_external_beat=now;
      }
      request_beat=false;
    }
    
    if (divider_counter==0 && ((now-last_external_beat)>(external_beat_length/2)))
    {
      divider_counter++;
      is_beat=true;
    }
  }
  
  checkMidi(now);

  //handle pitchbend
  long pitchDiff = pitchBendTarget-pitchBend;
  if (pitchDiff!=0)
  {
    pitchBend = pitchBend + pitchDiff/abs(pitchDiff);
  }

  byte volume=0;
  boolean nextNote=false;

  long llength=melody[(currentStep+2)%numSteps];
  
  if (is_beat)
  {
    beatcounter++;
    if (beatcounter>=llength)
    {
      nextNote=true;
      beatcounter=0;
    }
    else
    {
      nextNote=false; 
    }
  }    

  if (nextNote)
  {
    currentStep+=3;
    keypressed=now;
    keyreleased=0;
    currentNote=melody[currentStep%numSteps];
    currentPitch=melody[(currentStep+1)%numSteps];
    
    noteOnManualValReference[0]=analogRead(potPin1);
    noteOnManualValReference[1]=analogRead(potPin2);
  }  

  long rel=50;

  long pressed_duration=now-keypressed;
  long released_duration=now-keyreleased;
  
  for (int func=0; func<funcs_size;func++)
  {
    if (keypressed>0 && pressed_duration<funcs[midi_channel][func][0])
    {
      volume=(pressed_duration*255/funcs[midi_channel][func][0]);
    } 
    else if (keypressed>0 && pressed_duration<(funcs[midi_channel][func][0]+funcs[midi_channel][func][1]))
    {
      volume=255-(pressed_duration-funcs[midi_channel][func][0])*(255-funcs[midi_channel][func][2])/funcs[midi_channel][func][1];
    }
    else if (keypressed>0)
    {
      volume=funcs[midi_channel][func][2];
    } 
    else if (keyreleased>0 && released_duration<funcs[midi_channel][func][3])
    {
      volume=release_reference_current_values[func]-(released_duration*release_reference_current_values[func])/funcs[midi_channel][func][3];
    }
    else
    {
      volume=0;
    }
    
    if (keypressed>0 && func<3) //only the irst three funcs can be changed in velocity
    {
      volume=volume*(1.*currentVelocity)/127.;
    }
    
    if (keypressed>0)
    {
      release_reference_current_values[func]=volume;
    }
    
    if (currentNote==0)
    {
      volume=0;
    }

    volume=(1.-tremolo1[2*func])*volume + abs(tremolo1[2*func]*volume*(1+cos(now*tremolo1[2*func+1]))/2.);
    volume=volume*mix[midi_channel][func];
    

    //glaetten
    volume=volume*rund+current_values[func]*(1.-rund);
    current_values[func]=volume;
    
    //func 0-5 control the digital pot 
    if (func<6)
    {
      digitalPotWrite(slaveSelectPinVol, channel[func], 255-volume);
    } 
    else if (func==7)
    {
      ozz(slaveSelectPinFreq, 4, volume); 
    }
  }
  
  if (currentNote>0)
  {      
    int pitch=currentPitch;
    
    int pitchBendFunc=255-current_values[6];
    
    int val=vals[12*pitch+(currentNote-1)]; 
    byte oct=ozzoctav[12*pitch+(currentNote-1)]; 
    byte cali=calibri[12*pitch+(currentNote-1)];
    
    setOzzOctav(oct);
    ozz(slaveSelectPinFreq, 0, val+pitchBend-pitchBendFunc);
    ozz(slaveSelectPinFreq, frequencyCalibrationChannel, cali+manualpitch2);
  }    
  
  if (!midi_enabled && keypressed>0 && pressed_duration>=(llength*beat_length-rel))
  {
    keypressed=0;
    keyreleased=now;
  }
 
  
  if (is_beat && external_beat)
  {
     beat_length=now-lastbeat;
     lastbeat=now;
     is_beat=false;
  }    

}

void ozz(int slave, int osz, int value) {
  byte val = max(0,min(255, value));
  digitalPotWrite(slave, osz, val);
}

void digitalPotWrite(int slave, int address, int value) {
  // take the SS pin low to select the chip:
  digitalWrite(slave,LOW);
  //  send in the address and value via SPI:
  SPI.transfer(address);
  SPI.transfer(value);
  // take the SS pin high to de-select the chip:
  digitalWrite(slave,HIGH); 
}
