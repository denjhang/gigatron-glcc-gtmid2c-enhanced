#include <stdio.h>
#include <gigatron/sys.h>
#include <gigatron/console.h>
#include <gigatron/libc.h>


struct Note
{
    int ch,noteoffset,volume,waveform,relative_pitch,start_step;	
};

struct Note notes[] = 
{//channel, pitch, volume, waveform,relative_pitch, start_step
{1, 62, 64, 2, 0, 0},
{1, 62, 64, 2, 50, 10},
{1, 62, 64, 2, 100, 20},
{1, 62, 64, 2, 150, 30},
{1, 62, 64, 2, 200, 40},
{1, 62, 64, 2, 230, 45},
{1, 62, 64, 2, 250, 50},
{1, 62, 64, 2, 300, 60},
{1, 62, 64, 2, 350, 70},
{1, 62, 64, 2, 400, 80},
{1, 62, 64, 2, 450, 90},
{1, 0, 0, 2, 0, 100},
{0, 0, 0, 2, 0, 101},
};

struct Note *play_note(struct Note *note, int stepCount){
        while (stepCount == note->start_step) {
          channel_t *chan = &channel(note->ch);
          int off = note->noteoffset * 2;
		  int keyL = SYS_Lup(notesTable + off - 2);
		  int keyH = SYS_Lup(notesTable + off - 1);
//		  int key_sum,keyL_new,keyH_new,key_sum_new;
//		  int keyL_new,keyH_new,key_sum_new;
		  
          chan->wavA = 128 - note->volume;
          chan->wavX = note->waveform;
		  
		  if(note->relative_pitch==0){
          chan->keyL = SYS_Lup(notesTable + off - 2);
          chan->keyH = SYS_Lup(notesTable + off - 1);			  			  
		  }else{
			  //如果keyL+弯音值 小于0
			  if(keyL+note->relative_pitch<0){
				  chan->keyL=127+(keyL+note->relative_pitch)%127;
				  chan->keyH=keyH+(keyL+note->relative_pitch)/127-1;
			  }
			  //如果keyL+弯音值 大于127
			  else if(keyL+note->relative_pitch>127){
				  chan->keyL=(keyL+note->relative_pitch)%127;
				  chan->keyH=keyH+(keyL+note->relative_pitch)/127;
				 				  
			  }else{
				  chan->keyH=keyH;
				  chan->keyL=keyL+note->relative_pitch;
			  }
			
//			chan->keyL = keyL_new;
//		    chan->keyH = keyH_new;		

		  }
//		printf("keyH=%i,keyL=%i,relative_pitch=%i\n",chan->keyH,chan->keyL,note->relative_pitch);		 		  
        note += 1;
        }
        return note;
}

void init_channel(){
	channel1.keyL=0;
	channel2.keyL=0;
	channel3.keyL=0;
	channel4.keyL=0;
	
	channel1.keyH=0;
	channel2.keyH=0;
	channel3.keyH=0;
	channel4.keyH=0;	
	
}

void print_all_notes(){
	register struct Note *note = notes;
	while(note->ch!=0){
		printf("ch:%i,n_offset:%i,vol:%i\n wave:%i,r_pitch:%i,\n step:%i\n",note->ch,note->noteoffset,note->volume,note->waveform,note->relative_pitch,note->start_step);
		note += 1;
	}
	
}


int main()
{
  register struct Note *note = notes;
  register int tmpfc = frameCount;
  register int stepCount = 0;
  init_channel();
  printf("Gigatron music engine\n");
  print_all_notes();
  
  while(note->ch!=0){ // using ch==0 to mark the end
    while (frameCount == tmpfc){ /* wait */ }     
    tmpfc = frameCount;
    note = play_note(note, stepCount);
    soundTimer = 1;
    stepCount += 1;
  }
  printf("Completion of the song\n");
  return 0;
}
