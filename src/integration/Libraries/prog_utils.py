import subprocess
import re
import time
import serial
import Libraries.common as common

import Libraries.progress_bar as progress_bar

def touch_serial_port(port, baudrate):
    print("Forcing reset using %dbps open/close on port %s" % (baudrate, port))
    try:
        s = serial.Serial(port=port, baudrate=baudrate)
        s.setDTR(False)
        s.close()
    except IOError as e:
        print("Wait and retry")
    except TypeError as e:
        print(f"Error while touching the port {port}")
    except:
        pass
    time.sleep(0.4)  # DO NOT REMOVE THAT (required by SAM-BA based boards)

def wait_for_reboot(vid, pid):
    print("Rebooting board in bootloader mode...")
    elapsed=0
    port_found = False
    while elapsed < 15 and port_found == False:
        spin_port = common.find_device(vid, pid)
        if spin_port != None:
            port_found = True
        time.sleep(0.25)
        elapsed += 0.25

    if port_found == False:
        print("Error! Unable to find selected board after reboot.")
        exit(-1)

    time.sleep(1)
    print("Board ready.")

def execute_cmd_prog(cmd, match_pattern_linebline=None, match_callback_linebline=None, match_pattern_end=None, match_callback_end=None, debug=False, timeout=None):
    if debug:
        print("cmd >", cmd)
    process = subprocess.Popen(cmd.split(" "), stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)
    start_time = time.time()
    old_output_line = ""
    whole_output_buffer = ""
    match_callback_ret = []
    
    while True:
        output_line = process.stdout.readline()
        whole_output_buffer += output_line

        if timeout and (time.time() - start_time) > timeout:
            process.kill()
            raise TimeoutError(f"Timeout reached after {timeout} seconds")
        
        if (match_pattern_linebline is not None and match_callback_linebline is not None):
            matchPattern = match_pattern_linebline.findall(output_line)
            if matchPattern:
                match_callback_ret.append(match_callback_linebline(matchPattern))

        if debug:
            if (output_line != "\n"):
                print(output_line)
    
        if not output_line:
            break

        if (old_output_line != output_line):
            start_time = time.time()
            old_output_line = output_line

    process.wait()  # Wait for the process to finish

    if (match_pattern_end is not None and match_callback_end is not None):
        matchPattern = match_pattern_end.findall(whole_output_buffer)
        if matchPattern:
            match_callback_ret.append(match_callback_end(matchPattern))
    return process.returncode, match_callback_ret

def match_bootloader_action(matchPattern):
    speed = "0 B/s"
    if 0 <= 6 < len(matchPattern[0]):
        if (matchPattern[0][5] != "" and matchPattern[0][6] != ""):
            speed = matchPattern[0][5] + " " + matchPattern[0][6]
        progress_bar.progress_bar(float(matchPattern[0][4]), 100, prefix='Program upload:', suffix='Complete ' + speed, length=40)
    return

def match_info(matchPattern):
    return matchPattern[0]

def reset_bootloader(port):
    try:
        ret = execute_cmd_prog("./3rdParties/mcumgr.exe --conntype=serial --connstring=dev=" + port + ",baud=115200,mtu=128 reset", timeout=10)
        if (ret[0] == 0):
            print(f"Reset target")
        else:
            return 1 #ko
    except TimeoutError as e:
        print("\nError:", e)
        return 1 #ko

def check_hash_bootloader(port, desired_hash):
    patternImageInfo = re.compile(r"\s*(image=(\d))\s(slot=(\d))\n\s*(version:\s(\d.\d.\d))\n\s*(bootable:\s(.*))\n\s*(flags:\s(.*))\n\s*(hash:\s([0-9a-f]{64}))")
    try:
        ret = execute_cmd_prog("./3rdParties/mcumgr.exe --conntype=serial --connstring=dev=" + port + ",baud=115200,mtu=128 image list", match_pattern_end=patternImageInfo, match_callback_end=match_info, debug=False, timeout=10)
        if (ret[0] == 0):
            print(f"\nBootloader is here")
        else:
            return 1 #ko
        
        if (ret[1] != []): #First upload
            if (0 <= 11 < len(ret[1][0])):
                if(ret[1][0][11] == desired_hash):
                    print("Hash match! The desired program is already in the flash.")
                    return 0 #ok program match
                else:
                    return 2 #ok but need to flash
        return 1 #ko

    except TimeoutError as e:
        print("\nError:", e)
        return 1 #ko

def flash_prog_bootloader(firm_bin, port):  
    patternBootloader = re.compile(r"(\d+\.?\d*) (\w+B|B) \/ (\d+\.?\d*) (\w+B|B) (?:\[.*?\]|).*?(\d+\.?\d*)%(?: (\d+\.?\d*) ((?:\w+B|B)\/s)|)")
    try:
        ret = execute_cmd_prog("./3rdParties/mcumgr.exe --conntype=serial --connstring=dev=" + port + ",baud=115200,mtu=128 image upload -e " + firm_bin, match_pattern_linebline=patternBootloader, match_callback_linebline=match_bootloader_action, debug=False, timeout=10)
        if (ret[0] == 0):
            print(f"\nSuccessfully flashed {firm_bin}")
            return 0 #yes
        else:
            return 1 #ko
    except TimeoutError as e:
            print("\nError:", e)
            return 1 #ko

def master_reset(port, vid, pid):
    '''
    Does not work
    '''
    touch_serial_port(port, 1200)
    time.sleep(2)

    wait_for_reboot(vid, pid)

    boot_port = common.find_device(vid, pid)

    reset_ret = reset_bootloader(port)
    if (reset_ret == 1):
        return 1, "Failure", False

    new_port = common.find_device(vid, pid)

    return new_port


def flash_prog_procedure(firm_bin, port, hash=None):
    vid, pid = common.get_pid_vid(port)

    touch_serial_port(port, 1200)
    time.sleep(1)

    wait_for_reboot(vid, pid)

    new_port = common.find_device(vid, pid)
    
    if (hash != None):
        hash_ret = check_hash_bootloader(new_port, hash)
        if (hash_ret == 0):
            reset_ret = reset_bootloader(new_port)
            if (reset_ret == 1):
                return 1, "Failure", False
            return 0, "Success programm already in flash", True
        elif (hash_ret == 1):
            return 1, "Failure", False
        #if 2 we continue

    prog_ret = flash_prog_bootloader(firm_bin, new_port)
    if (prog_ret == 1):
        return 1, "Failure", False
    time.sleep(5)
    
    reset_ret = reset_bootloader(new_port)
    if (reset_ret == 1):
        return 1, "Failure", False

    return 0, "Success", True