import subprocess
import re
import sys

def run_aes_enc(command):
    result = subprocess.run(command, capture_output=True, text=True)

    if result.returncode != 0:
        print(f"Error executing command: {result.stderr}")
        return None

    match = re.search(r"Time (\d+\.\d+) ms", result.stdout)
    if match:
        return float(match.group(1))
    else:
        print("Time not found in output")
        return None

def main():
    times = []
    if sys.argv[1] == 'serial':
        command = ["build/aes", "test/Tux.bmp"]
    elif sys.argv[1] == 'openmp':
        command = ["build/aes_openmp", "test/Tux.bmp", sys.argv[2]]
    elif sys.argv[1] == 'pthread':
        command = ["build/aes_pthread", "test/Tux.bmp", sys.argv[2]]
    elif sys.argv[1] == 'aes-ni':
        command = ["build/aes-ni", "test/Tux.bmp"]
    elif sys.argv[1] == 'aes-ni-pthread':
        command = ["build/aes-ni-pthread", "test/Tux.bmp", sys.argv[2]]

    runs = 500
    for _ in range(runs):
        elapsed_time = run_aes_enc(command)

        if elapsed_time is not None:
            times.append(elapsed_time)

    print("----------------------------")
    print("Executable version: %s" % sys.argv[1])
    print("Total Runs: %d" % runs)
    print("Average Time: %f ms" % (sum(times) / len(times)))
    print("----------------------------")
    

if __name__ == "__main__":
    main()
