/*
 * dshot.c
 *
 *  Created on: Apr. 22, 2020
 *      Author: Alka
 */

#include "dshot.h"
//#include "serial_telemetry.h"

int dpulse[16] = {0} ;

const char gcr_encode_table[16] = { 0b11001,
		0b11011,
		0b10010,
		0b10011,
		0b11101,
		0b10101,
		0b10110,
		0b10111,
		0b11010,
		0b01001,
		0b01010,
		0b01011,
		0b11110,
		0b01101,
		0b01110,
		0b01111
};

int shift_amount = 0;
uint32_t gcrnumber;
extern int e_com_time;
extern char send_telemetry;
      // delay after getting dshot signal before changing over to output
int dshot_full_number;



void computeDshotDMA(){


int j = 0;
dshot_frametime = dma_buffer[31]- dma_buffer[0];
	         if((dshot_frametime < 1690)&&(dshot_frametime > 1600)){


				for (int i = 0; i < 16; i++){
					//if(dpulse)
					dpulse[i] = ((dma_buffer[j + (i<<1) +1] - dma_buffer[j + (i<<1)])>>6) ;
//					if(dpulse[i] > 1){
//						dshot_badcounts++;
//						return;
//					}
				}

				uint8_t calcCRC = ((dpulse[0]^dpulse[4]^dpulse[8])<<3
						|(dpulse[1]^dpulse[5]^dpulse[9])<<2
						|(dpulse[2]^dpulse[6]^dpulse[10])<<1
						|(dpulse[3]^dpulse[7]^dpulse[11])
				);
				uint8_t checkCRC = (dpulse[12]<<3 | dpulse[13]<<2 | dpulse[14]<<1 | dpulse[15]);
				//



				if(!armed){
					if (dshot_telemetry == 0){
						 if(calcCRC == ~checkCRC+16){
							 dshot_telemetry = 1;
						//	 is_output = 1;        // so that setupinput() is called on next timer16 timeout

						//	 transferComplete();
						 }
					}
				}
				if(dshot_telemetry){
					checkCRC= ~checkCRC+16;
				}


				int tocheck = (
						dpulse[0]<<10 | dpulse[1]<<9 | dpulse[2]<<8 | dpulse[3]<<7
						| dpulse[4]<<6 | dpulse[5]<<5 | dpulse[6]<<4 | dpulse[7]<<3
						| dpulse[8]<<2 | dpulse[9]<<1 | dpulse[10]);

				if(calcCRC == checkCRC){
					signaltimeout = 0;
					dshot_goodcounts++;
					if(dpulse[11]==1){
                    send_telemetry=1;
					}else{
					send_telemetry=0;
					}
					if (tocheck > 47){


						newinput = tocheck;
	                    dshotcommand = 0;
	                    return;
					}

				if ((tocheck <= 47)&& (tocheck > 0)){
					newinput = 0;
					dshotcommand = tocheck;    //  todo
				}
				if (tocheck == 0){
					newinput = 0;
					dshotcommand = 0;
				}



			//	break;

				if ((dshotcommand > 0) && (running == 0) && armed) {
					switch (dshotcommand){                   // todo


					case 1:
					//	playInputTune();
					break;

					case 2:
					//	playInputTune2();
				    break;

					case 7:
						dir_reversed = 0;
				    break;

				    case 8:
				    	dir_reversed = 1;
				    break;

					case 9:
						bi_direction = 0;
						armed = 0;
						zero_input_count = 0;
				    break;

					case 10:
						bi_direction = 1;
						zero_input_count = 0;
						armed = 0;
				    break;

					case 12:
						//	storeEEpromConfig();
					while (1) {   // resets esc as iwdg times out

					}
				    break;

					case 20:
						forward = 1 - dir_reversed;
					break;

					case 21:
						forward = dir_reversed;
					break;

					}


					last_dshot_command = dshotcommand;
					dshotcommand = 0;
				}
				}else{
					dshot_badcounts++;
				}

		}
}


void make_dshot_package(){
//	TIM8->CNT = 0;

 // e_com_time = (commutation_interval * 6 ) >>1;
  if (!running){
	  e_com_time = 65535;
  }

//	calculate shift amount for data in format eee mmm mmm mmm, first 1 found in first seven bits of data determines shift amount
// this allows for a range of up to 65408 microseconds which would be shifted 0b111 (eee) or 7 times.
for (int i = 15; i >= 9 ; i--){
	if(e_com_time >> i == 1){
		shift_amount = i+1 - 9;
		break;
	}else{
		shift_amount = 0;
	}
}


// shift the commutation time to allow for expanded range and put shift amount in first three bits
	dshot_full_number = ((shift_amount << 9) | (e_com_time >> shift_amount));
//calculate checksum
	uint16_t  csum = 0;
	uint16_t csum_data = dshot_full_number;
		  for (int i = 0; i < 3; i++) {
		      csum ^=  csum_data;   // xor data by nibbles
		      csum_data >>= 4;
		  }
		  csum = ~csum;       // invert it
		  csum &= 0xf;

		  dshot_full_number = (dshot_full_number << 4)  | csum; // put checksum at the end of 12 bit dshot number

// GCR RLL encode 16 to 20 bit

		  gcrnumber = gcr_encode_table[(dshot_full_number >> 12)] << 15  // first set of four digits
		  | gcr_encode_table[(((1 << 4) - 1) & (dshot_full_number >> 8))] << 10  // 2nd set of 4 digits
		  | gcr_encode_table[(((1 << 4) - 1) & (dshot_full_number >> 4))] << 5  //3rd set of four digits
		  | gcr_encode_table[(((1 << 4) - 1) & (dshot_full_number >> 0))];  //last four digits
//GCR RLL encode 20 to 21bit output

//	  gcrnumber = 0b1000000000000000001;
//		  gcr[0] = 0;
//		  lastnumber = 0;
//		  for( int i= 21; i >= 0; i--){
//						if((gcrnumber & ( 1 << (i-1)))){   // if the bit is a 1
//							lastnumber = !lastnumber; // invert the bit
//							gcr[22-i] = (!lastnumber * high_bit_length); // since the output is inverted electrical a 0 becomes period + 1
//						}else{ // if the bit is zero
//						//	gcr[20-i] = (max_duty)- lastnumber*(max_duty); // output same as last one
//							gcr[22-i] = (!lastnumber * high_bit_length);
//						}
//		  }
//		  gcr[1] = high_bit_length;        //  since pwm is inverted
//          gcr[22] = 0;



		  gcr[1+7] = 94;
		  for( int i= 19; i >= 0; i--){              // each digit in gcrnumber
			  gcr[7+20-i+1] = ((((gcrnumber &  1 << i )) >> i) ^ (gcr[7+20-i]>>6)) *94;        // exclusive ored with number before it multiplied by 64 to match output timer.
		  }
          gcr[7] = 0;



//		  gcr[1] = 94;
//		  for( int i= 19; i >= 0; i--){              // each digit in gcrnumber
//			  gcr[20-i+1] = ((((gcrnumber &  1 << i )) >> i) ^ (gcr[20-i]>>6)) * 94;        // exclusive ored with number before it multiplied by 64 to match output timer.
//		  }
//          gcr[0] = 0;


}



