import re
import os
import sys

C_MACROS = """
#define D(x) x                     /* wait x frames */
#define X(c) 127+(c)               /* channel c off */
#define N(c,n) 143+(c),(n)         /* channel c on, note=n */
#define M(c,n,v) 159+(c),(n),(v)   /* channel c on, note=n, wavA=v */
#define W(c,n,v,w) 175+(c),(n),(v),(w)   /* channel c on, note=n, wavA=v ,wavX=w*/
#define byte unsigned char
#define nohop __attribute__((nohop))
"""

MAX_ARRAY_SIZE = 250 # Max bytes per array, including the terminating 0

def get_command_byte_size(command_str):
    if command_str.startswith("D("):
        return 1
    elif command_str.startswith("X("):
        return 1
    elif command_str.startswith("N("):
        return 2
    elif command_str.startswith("M("):
        return 3
    elif command_str.startswith("W("):
        return 4
    return 0 # Should not happen for valid commands

def parse_gbas(gbas_content, base_filename, original_input_filename):
    all_c_arrays = []
    array_names = []
    
    music_data_started = False
    last_tick_sum = 0
    
    current_array_index = 0
    all_array_definitions = []
    array_names_for_pointer = []
    total_mem_size = 0
    
    music_data_started = False
    last_tick_sum = 0
    
    current_array_lines = [] # Stores lines for the current static const byte array
    current_array_byte_size = 0 # Tracks byte size of commands in current_array_lines
    current_line_commands = [] # Stores commands for the current output line
    current_line_byte_size = 0 # Tracks byte size of commands in current_line_commands

    MAX_COMMANDS_PER_LINE = 10 # User requested 10 commands per line
    
    # Regular expressions for parsing
    eat_sound_timer_re = re.compile(r"^\s*call eatSound_Timer,(\d+)\s*$")
    beep_re = re.compile(r"^\s*call beep,(\d+),(\d+),(\d+),(\d+),(\d+)\s*$")
    
    for line in gbas_content.splitlines():
        if "proc music_data '先定时，再演奏，一次性演奏4个通道" in line:
            music_data_started = True
            array_name = f"{base_filename}{current_array_index:03d}"
            array_names_for_pointer.append(array_name)
            current_array_lines.append(f"nohop static const byte {array_name}[] = {{")
            continue
        
        if not music_data_started:
            continue
            
        # Stop parsing at 'endproc' for music_data
        if "endproc" in line:
            break

        command_str = None
        command_byte_size = 0

        # Parse eatSound_Timer
        match_timer = eat_sound_timer_re.match(line)
        if match_timer:
            current_tick_sum = int(match_timer.group(1))
            delay = current_tick_sum - last_tick_sum
            if delay > 0:
                command_str = f"D({delay})"
                command_byte_size = get_command_byte_size(command_str)
            last_tick_sum = current_tick_sum

        # Parse beep
        match_beep = beep_re.match(line)
        if match_beep:
            ch = int(match_beep.group(1))
            note = int(match_beep.group(2))
            vol_gbas = int(match_beep.group(3))
            wave = int(match_beep.group(4))
            pitch_bend = int(match_beep.group(5)) # Ignored as per instructions

            # Volume conversion: C volume = 127 - GBAS volume, range 64-127
            # GBAS volume is 0-63.
            # If GBAS volume is 0, it means note off.
            if vol_gbas == 0:
                command_str = f"X({ch})"
                command_byte_size = get_command_byte_size(command_str)
            else:
                vol_c = 127 - vol_gbas
                # Ensure volume is within 64-127 range
                vol_c = max(64, min(127, vol_c))
                command_str = f"W({ch},{note},{vol_c},{wave})"
                command_byte_size = get_command_byte_size(command_str)
        
        if command_str:
            # Check if adding this command to the current line or array would exceed limits
            # +1 for the terminating 0
            if (current_array_byte_size + current_line_byte_size + command_byte_size + 1 > MAX_ARRAY_SIZE) or \
               (len(current_line_commands) >= MAX_COMMANDS_PER_LINE):
                
                # Flush current line buffer to current_array_lines
                if current_line_commands:
                    current_array_lines.append("  " + ",".join(current_line_commands) + ",")
                    current_array_byte_size += current_line_byte_size
                    current_line_commands = []
                    current_line_byte_size = 0

                # If array still too big, close current array and start new one
                if current_array_byte_size + command_byte_size + 1 > MAX_ARRAY_SIZE:
                    current_array_lines.append("  0") # Terminate current array
                    current_array_lines.append("};")
                    all_array_definitions.append("\n".join(current_array_lines))
                    
                    current_array_index += 1
                    array_name = f"{base_filename}{current_array_index:03d}"
                    array_names_for_pointer.append(array_name)
                    current_array_lines = [f"nohop static const byte {array_name}[] = {{"]
                    current_array_byte_size = 0
            
            current_line_commands.append(command_str)
            current_line_byte_size += command_byte_size
            
    # Flush any remaining commands in the current line buffer
    if current_line_commands:
        current_array_lines.append("  " + ",".join(current_line_commands) + ",")
        current_array_byte_size += current_line_byte_size

    # Add the last array if it has content
    if current_array_lines:
        current_array_lines.append("  0") # Terminate last array
        current_array_byte_size += 1 # Account for the terminating 0
        current_array_lines.append("};")
        all_array_definitions.append("\n".join(current_array_lines))
        total_mem_size += current_array_byte_size

    # Generate the main pointer array
    main_pointer_array = [f"nohop const byte *{base_filename}[] = {{"]
    for name in array_names_for_pointer:
        main_pointer_array.append(f"  {name},")
    main_pointer_array.append("  0")
    main_pointer_array.append("};")

    # Generate the header comment
    num_segments = len(array_names_for_pointer)
    header_comment = f"""
/* extern const byte* {base_filename}[];
 * -- generated by gbas_to_c.py from file {original_input_filename}
 *    memsize {total_mem_size} in {num_segments} segments
 */
"""

    return header_comment + C_MACROS + "\n" + "\n\n".join(all_array_definitions) + "\n\n" + "\n".join(main_pointer_array) + "\n"

def main():
    # Check if input file is provided
    if len(sys.argv) != 2:
        print("Usage: python gbas_to_c.py <input_file.gbas>")
        sys.exit(1)

    input_filename = sys.argv[1]
    
    # Check if input file exists
    if not os.path.exists(input_filename):
        print(f"Error: Input file '{input_filename}' not found.")
        sys.exit(1)

    # Read the gbas file
    try:
        with open(input_filename, "r", encoding="utf-8") as f:
            gbas_content = f.read()
    except Exception as e:
        print(f"Error reading file '{input_filename}': {e}")
        sys.exit(1)

    # Extract base filename for C array naming
    base_filename = os.path.splitext(os.path.basename(input_filename))[0]

    # Convert to C format
    try:
        c_code = parse_gbas(gbas_content, base_filename, input_filename)
    except Exception as e:
        print(f"Error during conversion: {e}")
        sys.exit(1)

    # Write to a new C file
    output_filename = f"{base_filename}.gbas.c"
    try:
        with open(output_filename, "w") as f:
            f.write(c_code)
    except Exception as e:
        print(f"Error writing to file '{output_filename}': {e}")
        sys.exit(1)

    print(f"Conversion complete. Output written to {output_filename}")

if __name__ == "__main__":
    main()
