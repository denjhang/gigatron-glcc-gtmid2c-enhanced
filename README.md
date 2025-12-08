# gigatron-glcc-gtmid2c-enhanced
An enhanced GLCC music engine, based on GTMIDI and ported from GLCC, can convert MIDI to C arrays and allows for the creation of complex custom instrument macros, such as volume modulation, waveform transformation, pitch bend, and vibrato.  

Thanks to lb3361, he taught me how to modify sound.s to add more macro commands, and I succeeded.

Actually, glcc 2.6.37 already has a port of gtmid built into glcc, and most importantly, it uses vIRQ to take over the music playback program, which makes music playback less resource-intensive, essentially implementing multithreading. This feature is essential. I also studied the sound.s source code, and I found understanding vIRQ and the music interrupt program very difficult; it's not an easy task.

With the help of Claude 4.5 Sonnet, I at least understood the explanation of the D, X, N, M macros in sound.s, and after a few modifications, I successfully added the W command, which is:

`#define W(c,n,v,w) 175+(c),(n),(v),(w) /* channel c on, note=n, wavA=v ,wavX=w*/; `

This adds setting wavX (0xfb register) to modify the waveform.
The following are modifications to sound.s to be compatible with waveform modification commands. This is not difficult, so you can continue to add pitch bend or even more complex ADSR calculations.

L 205 def code_midi_tick()::
`
        
            SUBI(0x24) # Adjusted to include W(c,n,v,w) 

`
L 178:
`
 
            # set note
            ADDI(0x10);STW(vLR)
            LDW('_midi.p');PEEK();INC('_midi.p');STW('_midi.cmd')

            # set volume
            LDW(vLR);_BLT('.freq')
            LDI(0xfa);ST('_midi.tmp')
            LDW('_midi.p');PEEK();INC('_midi.p');POKE('_midi.tmp')
            
            # set wave 
            LDW(vLR);_BLT('.freq')
            LDI(0xfb);ST('_midi.tmp') 
            LDW('_midi.p');PEEK();INC('_midi.p');POKE('_midi.tmp')

            # Read 'w' and write to 0xfb
            #-------note---end-------------------------------------------
            label('.freq')
            LDI(0xfc);ST('_midi.tmp')
            LDWI(v('notesTable')-22);ADDW('_midi.cmd');ADDW('_midi.cmd');STW('_midi.cmd')
            LUP(0);ST(vLR);LDW('_midi.cmd');LUP(1);ST(vLR+1)
            LDW(vLR);DOKE('_midi.tmp');_CALLJ('.getcmd')

`

Then I also modified the makefile.
`
CFLAGS=-map=64k,./music.ovl --no-runtime-bss sound.s clock.s
`

As for where this music data came from, it certainly wasn't handwritten by me. Instead, it uses a tool I previously wrote called `mid2gbas` to obtain text-formatted MID music data. The biggest difference between this and the original gbas is that it includes more complex volume modulation, waveform switching, pitch bend adjustment, and supports reading additional instrument configuration files to achieve different macro configurations for multiple instruments.  

Then I wrote a tool to convert the gbas text into C arrays, thus integrating the results of my previous research.  
`
./midi_converter.exe bwv883f.mid bwv883f.gbas -d -time 108.5 -config midi_config.ini
`
  
`
python gbas_to_c.py bwv883f.gbas
`
