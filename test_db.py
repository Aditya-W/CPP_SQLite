import subprocess
import os

def run_script(commands):
    # Cross-platform executable handling
    executable = "db.exe" if os.name == 'nt' else "./db"
    
    input_text = "\n".join(commands) + "\n"
    
    # Run the compiled C++ program
    process = subprocess.run(
        [executable], 
        input=input_text, 
        capture_output=True, 
        text=True
    )
    
    # --- SUBPROCESS DIAGNOSTICS ---
    print("\n--- PROCESS DIAGNOSTICS ---")
    print(f"Target Executable: {executable}")
    print(f"Return Code: {process.returncode}")
    print(f"Standard Error: {repr(process.stderr)}")
    print("---------------------------\n")
    # ------------------------------
    
    return process.stdout.split('\n')

def test_insert_and_select():
    commands = [
        "insert 1 aditya aditya@example.com",
        "select",
        ".exit"
    ]
    
    output = run_script(commands)
    
    print("--- RAW OUTPUT DEBUG ---")
    for i, line in enumerate(output):
        print(f"Line {i}: {repr(line)}")
    print("------------------------")

    # Let's assert against exactly what your C++ code outputs
    assert output[0] == "Inserting: 1 aditya aditya@example.com"
    assert output[1] == "Executed"
    assert output[2] == "( 1, aditya, aditya@example.com )"
    assert output[3] == "Executed"
    assert output[4] == "Exiting database..."
    
    print("\n✅ test_insert_and_select passed!")

def test_long_strings():

    long_username = 'a' * 33
    long_email = 'a' * 256

    commands = [
        f"insert 1 {long_username} {long_email}",
        "select",
        ".exit"
    ]

    output = run_script(commands)

    print("\n--- LONG STRING OUTPUT ---")
    for i, line in enumerate(output):
        print(f"Line: {i}: {repr(line)}")
    print("--------------------------")

    assert output[0] == "String is too long."

    print("\n✅ test_insert_and_select passed!")

if __name__ == "__main__":
    test_insert_and_select()
    test_long_strings()
    # g++ db.cpp -o db.exe -static-libstdc++ -static-libgcc