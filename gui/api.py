from flask import Flask, request, jsonify
import subprocess
import os

app = Flask(__name__)

# Start the C simulator in the background with unbuffered output
simulator_process = subprocess.Popen(
    ["../simulator/simulator.exe"],
    stdin=subprocess.PIPE,
    stdout=subprocess.PIPE,
    stderr=subprocess.PIPE,
    text=True,
    bufsize=1,  # Line-buffered
    universal_newlines=True  # Ensures newline handling is correct
)

def read_output():
    """Continuously read available output from the C program."""
    output = []
    while True:
        line = simulator_process.stdout.readline().strip()  # Read one line at a time
        if not line:
            break
        print(f"[C OUTPUT]: {line}")  # Print output in Flask logs
        output.append(line)
    return "\n".join(output)

def send_command(command):
    """Send a command to the C simulator and return all output."""
    simulator_process.stdin.write(command + "\n")
    simulator_process.stdin.flush()
    return read_output()  # Read output immediately

@app.route("/load_instructions", methods=["POST"])
def loadInstructions():
    """Loads instruction contents from UI into the simulator."""
    data = request.json
    instructions = data.get("instructions")

    print(instructions)

    if not instructions:
        return jsonify({"error": "No file contents provided."}), 400
    
    results = []
    for instruction in instructions:
        response = send_command(f"write {instruction}")
        results.append(response)

    return jsonify({"message": response})

if __name__ == "__main__":
    app.run(port=5000, debug=True)
