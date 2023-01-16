#include <Arduino.h>

// Definitions
#define CCU Serial1 // communication port between CCU generator and the pCard
#define PC  Serial  // Virtual serial

IntervalTimer LEDblink; // for HV LED!

// pre-setup commands
// Pin namings
const byte EnBpin = 2;
const byte Latchpin = 3;
const byte CLKpin = 4;
const byte Data1pin = 5;
const byte Data2pin = 6;
const byte Data3pin = 7;
const byte Data4pin = 8;
const byte Data5pin = 9;
const byte Data6pin = 10;
const byte Data7pin = 11;
const byte Data8pin = 12;
const byte RSTpin = 13;
const byte Triggerpin = 16;
const byte Connectionpin = 17;
const byte LEDerrorpin = 18;
const byte LEDVPPpin = 19;
const byte LEDVNNpin = 20;
const byte VoltageOKpin = 21;
const int VPPreadPin = A1;
const int VNNreadPin = A0;

// Variable bank!
volatile byte VNN = 0;
volatile byte VPP = 0;

byte VPPread = 0;
byte VNNread = 0; // contains read voltage
volatile bool HVlevelError = false;

volatile bool STOP = true;       // at the beginning stop signal generation
volatile bool VoltageOK = false; // if negative voltage is appled!
volatile word SeqCount = 0;
byte income[32];
word Sequence;
word Maxseq;
byte bitstream[256][16];

byte CCUSettings[32];
volatile bool Bitpending = false;
unsigned long lasrInterupt = 0;
unsigned longcurrenttime;
byte HVarray[2];
bool VPPLEDstate;
bool VNNLEDstate;
bool VoltageOKstate;
void
setup()
{
	// Serial Port
	// conection----------------------------------------------------------------------------------------------------------------------Serial
	// port config
	CCU.begin(9600);

	PC.begin(9600);
	// Pin
	// Mode!-----------------------------------------------------------------------------------------------------------------------------------
	// pin mode config
	pinMode(EnBpin, OUTPUT);
	pinMode(Latchpin, OUTPUT);
	pinMode(CLKpin, OUTPUT);
	pinMode(Data1pin, OUTPUT);
	pinMode(Data2pin, OUTPUT);
	pinMode(Data3pin, OUTPUT);
	pinMode(Data4pin, OUTPUT);
	pinMode(Data5pin, OUTPUT);
	pinMode(Data6pin, OUTPUT);
	pinMode(Data7pin, OUTPUT);
	pinMode(Data8pin, OUTPUT);
	pinMode(RSTpin, OUTPUT);
	pinMode(Triggerpin, INPUT_PULLUP);
	pinMode(Connectionpin, OUTPUT);
	digitalWrite(Connectionpin, HIGH);
	pinMode(LEDerrorpin, OUTPUT);
	pinMode(LEDVPPpin, OUTPUT);
	pinMode(LEDVNNpin, OUTPUT);
	pinMode(VoltageOKpin, INPUT_PULLUP);
	// Logicwrite on
	// pins!---------------------------------------------------------------------------------------------------------------------------
	// pin logic
	digitalWrite(EnBpin, LOW);
	digitalWrite(Latchpin, LOW);
	digitalWrite(CLKpin, LOW);
	digitalWrite(Data1pin, LOW);
	digitalWrite(Data2pin, LOW);
	digitalWrite(Data3pin, LOW);
	digitalWrite(Data4pin, LOW);
	digitalWrite(Data5pin, LOW);
	digitalWrite(Data6pin, LOW);
	digitalWrite(Data7pin, LOW);
	digitalWrite(Data8pin, LOW);
	digitalWrite(RSTpin, HIGH);

	digitalWrite(LEDerrorpin, HIGH);
	digitalWrite(LEDVPPpin, LOW);
	digitalWrite(LEDVNNpin, LOW);
	delay(250);
	digitalWrite(LEDerrorpin, LOW);
	digitalWrite(LEDVPPpin, HIGH);
	digitalWrite(LEDVNNpin, LOW);
	delay(250);
	digitalWrite(LEDerrorpin, LOW);
	digitalWrite(LEDVPPpin, LOW);
	digitalWrite(LEDVNNpin, HIGH);
	delay(250);
	digitalWrite(LEDerrorpin, LOW);
	digitalWrite(LEDVPPpin, LOW);
	digitalWrite(LEDVNNpin, LOW);
	digitalWrite(Connectionpin, LOW);

	// interupts--------------------------------------------------------------------------------------------------------------------------------------
	// Interrups
	attachInterrupt(
	    digitalPinToInterrupt(Triggerpin), TriggerInterrup,
	    FALLING); // main trigger, if comes in and CCU confirms, a delayed
	              // trigger out will be created and counted
	LEDblink.begin(blinkLED, 100000);

	for (int i = 0; i < 256; i++) {
		for (int j = 0; j < 16; j++) {
			bitstream[i][j] = 255;
			bitstream[i][j] = 0;
		}
	}

	programBits();
}

void
loop()
{
	VoltageOKstate = digitalRead(VoltageOKpin);
	readHV();

	// digitalWrite(RSTpin,digitalRead(VoltageOKpin); // if negative 5 is
	// gone! it will keep shift registes in LOW mode.

	// put your main code here, to run repeatedly:
	if (Serial1.available()) {
		Serial1.readBytes(income, 32);
		if (income[0] == 66 && income[1] == 105 && income[2] == 116
		    && income[3] == 128) { // it is a sequence
			Sequence = income[4] << 8;
			Sequence = Sequence + income[5] - 1;
			Maxseq = income[7] << 8;
			Maxseq = Maxseq + income[8];

			for (int i = 0; i < 16; i++) {
				bitstream[Sequence][i] = income[i + 10];
			}
			SeqCount = 0;
			programBits();
		}
	}

	if (Bitpending) {
		Bitpending = false;
		digitalWrite(Latchpin, HIGH);
		delayMicroseconds(5);
		digitalWrite(Latchpin, LOW);
		programBits(); // programs the next sequence
	}
}

void
TriggerInterrup(void)
{ // sequence will be added, main loop will perform the programming.

	SeqCount++;
	if (Maxseq > 0) {
		if (SeqCount > Maxseq - 1) {
			SeqCount = 0;
		}
	} else {
		SeqCount = 0;
	}
	Bitpending = true;
}

void
readHV(void)
{
	int readinput;
	readinput = analogRead(VPPreadPin);
	VPPread = readinput * 0.477;
	readinput = analogRead(VNNreadPin);
	VNNread = readinput * 0.806451612903;
	VPPLEDstate = false;
	VNNLEDstate = false;

	if (VNNread > 10) {
		VNNLEDstate = true;
	}
	if (VPPread > 10) {
		VPPLEDstate = true;
	}
	if (!VoltageOKpin) {
		VPPread = 0;
		VNNread = 0;
	}
	HVarray[0] = VPPread;
	HVarray[1] = VNNread;
}

void
programBits(void)
{
	for (int i = 0; i < 8; i++) {

		digitalWrite(Data1pin,
		             bitRead(bitstream[SeqCount][1],
		                     i)); // responcible for channel 5to8
		digitalWrite(Data2pin,
		             bitRead(bitstream[SeqCount][3],
		                     i)); // resposible for chanels 13 16
		digitalWrite(Data3pin,
		             bitRead(bitstream[SeqCount][5], i)); // 21 24
		digitalWrite(Data4pin,
		             bitRead(bitstream[SeqCount][7], i)); // 29 32
		digitalWrite(Data5pin,
		             bitRead(bitstream[SeqCount][9], i)); // 39 40
		digitalWrite(Data6pin,
		             bitRead(bitstream[SeqCount][11], i)); // 45 48
		digitalWrite(Data7pin,
		             bitRead(bitstream[SeqCount][13], i)); // 53 56
		digitalWrite(Data8pin,
		             bitRead(bitstream[SeqCount][15], i)); // 61 64
		digitalWrite(CLKpin, HIGH);
		// delayMicroseconds(5);
		digitalWrite(CLKpin, LOW);
		// delayMicroseconds(5);
	}
	for (int i = 0; i < 8; i++) {
		digitalWrite(Data1pin, bitRead(bitstream[SeqCount][0],
		                               i)); // resposible for 1 to 4
		digitalWrite(Data2pin,
		             bitRead(bitstream[SeqCount][2], i)); // 9 to 12
		digitalWrite(Data3pin,
		             bitRead(bitstream[SeqCount][4], i)); // 17  20
		digitalWrite(Data4pin,
		             bitRead(bitstream[SeqCount][6], i)); // 25  28
		digitalWrite(Data5pin,
		             bitRead(bitstream[SeqCount][8], i)); // 33 36
		digitalWrite(Data6pin,
		             bitRead(bitstream[SeqCount][10], i)); // 41 44
		digitalWrite(Data7pin,
		             bitRead(bitstream[SeqCount][12], i)); // 49 52
		digitalWrite(Data8pin,
		             bitRead(bitstream[SeqCount][14], i)); // 57 60
		digitalWrite(CLKpin, HIGH);
		//  delayMicroseconds(5);
		digitalWrite(CLKpin, LOW);
		// delayMicroseconds(5);
	}
}
/*
void printInfo(void){
byte Ch;
byte Value;
    for (word j=0;j<16;j++){
    Ch=0;
    for (byte i=0;i<4;i++){
    Ch++;
    Value=0;
    Serial.print(" ");
    if(Ch==1){
    bitWrite(Value,0,bitRead(bitstream[SeqCount][j],6));
    bitWrite(Value,1,bitRead(bitstream[SeqCount][j],7));
    }
    if(Ch==2){
    bitWrite(Value,0,bitRead(bitstream[SeqCount][j],4));
    bitWrite(Value,1,bitRead(bitstream[SeqCount][j],5));
    }
    if(Ch==3){
    bitWrite(Value,0,bitRead(bitstream[SeqCount][j],2));
    bitWrite(Value,1,bitRead(bitstream[SeqCount][j],3));
    }
    if(Ch==4){
    bitWrite(Value,0,bitRead(bitstream[SeqCount][j],0));
    bitWrite(Value,1,bitRead(bitstream[SeqCount][j],1));
    }

switch (Value){
case 0:
Serial.print("RR");
break;
case 1:
Serial.print("+1");
break;
case 2:
Serial.print("-1");
break;
case 3:
Serial.print(" 0");
break;
}



    }
  }



  Serial.println( " ");
}

*/

void
blinkLED(void)
{
	CCU.write(HVarray, 2);
	if (VPPLEDstate) {
		digitalWrite(LEDVPPpin, !digitalRead(LEDVPPpin));
	} else {
		digitalWrite(LEDVPPpin, LOW);
	}
	if (VNNLEDstate) {
		digitalWrite(LEDVNNpin, !digitalRead(LEDVNNpin));
	} else {
		digitalWrite(LEDVNNpin, LOW);
	}
	if (!VoltageOKstate) {
		digitalWrite(LEDerrorpin, !digitalRead(LEDerrorpin));
	} else {
		digitalWrite(LEDerrorpin, LOW);
	}
}
