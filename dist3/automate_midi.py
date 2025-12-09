import argparse
import os
import subprocess
import sys
import shutil

def find_midi_file(filename):
    """
    Searches for the MIDI file in the current directory and 'music_midi' folder.
    Returns the full path if found, otherwise returns None.
    """
    # Check current directory
    if os.path.exists(filename):
        return filename
    
    # Check 'music_midi' folder
    music_midi_path = os.path.join("music_midi", filename)
    if os.path.exists(music_midi_path):
        return music_midi_path
        
    return None

def automate_midi(filename_mid_input, time_t):
    found_midi_path = find_midi_file(filename_mid_input)
    if not found_midi_path:
        print(f"Error: MIDI file '{filename_mid_input}' not found in current directory or 'music_midi' folder.")
        sys.exit(1)

    filename_mid = found_midi_path
    filename_base = os.path.splitext(os.path.basename(filename_mid))[0]
    filename_gbas_c = f"{filename_base}.gbas.c"

    print("Checking for required files...")
    required_files = [
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

        # Modify line 9
        if len(lines) >= 9:
            lines[8] = f"PGMS=music.gt1\n"
        else:
            print(f"Error: {makefile_path} has less than 9 lines.")
            sys.exit(1)

        with open(makefile_path, 'w') as f:
            f.writelines(lines)
        print(f"Modified {makefile_path} line 11 to: MIDIS={filename_base}.gbas.c")
        print(f"Modified {makefile_path} line 9 to: PGMS=music.gt1")
        print(f"{makefile_path} modification complete.")

        print("Starting MIDI conversion and compilation process...")
        # 3. Execute the following commands in order
        # Command 1: midi_converter.exe
        time_param = f"-time {time_t}" if time_t else ""
        # Assuming midi_converter.exe and midi_config.ini are in the current directory
        cmd1 = f"midi_converter.exe {filename_mid} {filename_base}.gbas -d {time_param} -config midi_config.ini"
        print(f"Executing: {cmd1}")
        try:
            # Suppress DEBUG output by redirecting to NUL (Windows) or /dev/null (Unix-like)
            if os.name == 'nt':
                subprocess.run(cmd1, shell=True, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            else:
                subprocess.run(cmd1, shell=True, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
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

        # Command 4: Rename music.gt1 to music-{filename_base}.gt1 using Python function
        old_name = "music.gt1"
        new_name = f"music-{filename_base}.gt1"
        print(f"Renaming {old_name} to {new_name}...")
        
        # Add a loop to wait for music.gt1 to be generated
        import time
        max_wait_time = 30  # seconds
        wait_interval = 1   # seconds
        time_elapsed = 0
        while not os.path.exists(old_name) and time_elapsed < max_wait_time:
            print(f"Waiting for {old_name} to be generated... ({time_elapsed}/{max_wait_time}s)")
            time.sleep(wait_interval)
            time_elapsed += wait_interval

        if not os.path.exists(old_name):
            print(f"Error: {old_name} was not generated within {max_wait_time} seconds. Cannot rename.")
            sys.exit(1)

        try:
            if os.path.exists(new_name):
                os.remove(new_name)
            os.rename(old_name, new_name)
            print(f"Successfully renamed {old_name} to {new_name}")
        except OSError as e:
            print(f"Error renaming file: {e}")
            sys.exit(1)

        # Move files to their respective directories
        print("Moving files to their respective directories...")
        try:
            # Define target directories
            dir_midi = "music_midi"
            dir_gt1 = "music_gt1"
            dir_c = "music_data_c"
            dir_gbas = "music_data_gbas"

            # Create directories if they don't exist
            os.makedirs(dir_midi, exist_ok=True)
            os.makedirs(dir_gt1, exist_ok=True)
            os.makedirs(dir_c, exist_ok=True)
            os.makedirs(dir_gbas, exist_ok=True)

            # Move MIDI file (only if it was originally in the current directory)
            if filename_mid == filename_mid_input: # Check if the MIDI file was found in the current directory
                midi_src = filename_mid
                midi_dst = os.path.join(dir_midi, filename_mid_input) # Use original input name for destination
                if os.path.exists(midi_src):
                    if os.path.exists(midi_dst):
                        os.remove(midi_dst)
                    shutil.move(midi_src, midi_dst)
                    print(f"Moved {midi_src} to {midi_dst}")
            else:
                print(f"MIDI file '{filename_mid_input}' was already in 'music_midi' or a subdirectory, no move needed.")

            # Move GT1 file
            gt1_src = new_name
            gt1_dst = os.path.join(dir_gt1, new_name)
            if os.path.exists(gt1_src):
                if os.path.exists(gt1_dst):
                    os.remove(gt1_dst)
                shutil.move(gt1_src, gt1_dst)
                print(f"Moved {gt1_src} to {gt1_dst}")

            # Move C file
            c_src = filename_gbas_c
            c_dst = os.path.join(dir_c, filename_gbas_c)
            if os.path.exists(c_src):
                if os.path.exists(c_dst):
                    os.remove(c_dst)
                shutil.move(c_src, c_dst)
                print(f"Moved {c_src} to {c_dst}")

            # Move GBAS file
            gbas_src = f"{filename_base}.gbas"
            gbas_dst = os.path.join(dir_gbas, gbas_src)
            if os.path.exists(gbas_src):
                if os.path.exists(gbas_dst):
                    os.remove(gbas_dst)
                shutil.move(gbas_src, gbas_dst)
                print(f"Moved {gbas_src} to {gbas_dst}")

            print("File moving complete.")
        except OSError as e:
            print(f"Error moving files: {e}")
            sys.exit(1)
    finally:
        pass

    print("Automation script executed successfully.")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Automate MIDI conversion and compilation.")
    parser.add_argument("-mid", dest="filename_mid_input", required=True, help="MIDI file name (e.g., filename.mid). Will search in current directory and 'music_midi' folder.")
    parser.add_argument("-time", dest="time_t", required=False, help="Time parameter for midi_converter.exe (optional)")
    args = parser.parse_args()

    automate_midi(args.filename_mid_input, args.time_t)