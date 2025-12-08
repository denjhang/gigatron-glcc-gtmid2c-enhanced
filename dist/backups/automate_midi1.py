import argparse
import os
import subprocess
import sys

def automate_midi(filename_mid, time_t):
    filename_base = os.path.splitext(filename_mid)[0]
    filename_gbas_c = f"{filename_base}.gbas.c"

    print("Checking for required files...")
    required_files = [
        filename_mid,
        "midi_converter.exe",
        "gbas_to_c.py",
        "Makefile",
        "music.c"
    ]

    for f in required_files:
        if not os.path.exists(f):
            print(f"Error: Missing file {f}. Please ensure all necessary files are in the current directory.")
            sys.exit(1)
    print("All required files found.")

    try:
        # 1. Modify music.c
        music_c_path = r"music.c"
        print(f"Modifying {music_c_path}...")
        with open(music_c_path, 'r') as f:
            lines = f.readlines()
        
        # Modify line 18
        if len(lines) >= 18:
            lines[17] = f"extern const byte* {filename_base}[];\n"
        else:
            print(f"Error: {music_c_path} has less than 18 lines.")
            sys.exit(1)
        
        # Modify line 36
        if len(lines) >= 36:
            lines[35] = f"  {{ {filename_base}, \"{filename_base}\" }},\n"
        else:
            print(f"Error: {music_c_path} has less than 36 lines.")
            sys.exit(1)

        with open(music_c_path, 'w') as f:
            f.writelines(lines)
        print(f"Modified {music_c_path} line 18 to: extern const byte* {filename_base}[];")
        print(f"Modified {music_c_path} line 36 to:   {{ {filename_base}, \"{filename_base}\" }},")
        print(f"{music_c_path} modification complete.")

        # 2. Modify Makefile
        makefile_path = r"Makefile"
        print(f"Modifying {makefile_path}...")
        with open(makefile_path, 'r') as f:
            lines = f.readlines()
        
        # Modify line 11
        if len(lines) >= 11:
            lines[10] = f"MIDIS={filename_base}.gbas.c\n"
        else:
            print(f"Error: {makefile_path} has less than 11 lines.")
            sys.exit(1)

        with open(makefile_path, 'w') as f:
            f.writelines(lines)
        print(f"Modified {makefile_path} line 11 to: MIDIS={filename_base}.gbas.c")
        print(f"{makefile_path} modification complete.")

        print("Starting MIDI conversion and compilation process...")
        # 3. Execute the following commands in order
        # Command 1: midi_converter.exe
        time_param = f"-time {time_t}" if time_t else ""
        # Assuming midi_converter.exe and midi_config.ini are in the current directory
        cmd1 = f"midi_converter.exe {filename_mid} {filename_base}.gbas -d {time_param} -config midi_config.ini"
        print(f"Executing: {cmd1}")
        try:
            subprocess.run(cmd1, shell=True, check=True)
            print("MIDI conversion complete.")
        except subprocess.CalledProcessError as e:
            print(f"Error executing command 1: {e}")
            sys.exit(1)

        # Command 2: python gbas_to_c.py filename.gbas
        cmd2 = f"python gbas_to_c.py {filename_base}.gbas"
        print(f"Executing: {cmd2}")
        try:
            subprocess.run(cmd2, shell=True, check=True)
            print("GBAS to C conversion complete.")
        except subprocess.CalledProcessError as e:
            print(f"Error executing command 2: {e}")
            sys.exit(1)

        # Command 3: make clean && make
        cmd3 = "make clean && make"
        print(f"Executing: {cmd3}")
        try:
            subprocess.run(cmd3, shell=True, check=True)
            print("Compilation complete.")
        except subprocess.CalledProcessError as e:
            print(f"Error executing command 3: {e}")
            sys.exit(1)
    finally:
        # Since the script is already in the target directory, no need to change back
        # os.chdir(original_cwd)
        # print(f"Returned to original working directory: {os.getcwd()}")
        pass

    print("Automation script executed successfully.")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Automate MIDI conversion and compilation.")
    parser.add_argument("-mid", dest="filename_mid", required=True, help="Path to the MIDI file (e.g., filename.mid)")
    parser.add_argument("-time", dest="time_t", required=False, help="Time parameter for midi_converter.exe (optional)")
    args = parser.parse_args()

    automate_midi(args.filename_mid, args.time_t)