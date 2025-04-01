from flask import Flask, request, jsonify
import subprocess
import threading

app = Flask(__name__)

# Launch C simulator executable
simulator_process = subprocess.Popen(
    ["../simulator/simulator"],
    stdin=subprocess.PIPE,
    stdout=subprocess.PIPE,
    stderr=subprocess.PIPE,
    text=True,
    bufsize=1
)

def read_output():
    output = []
    print("[DEBUG] Starting to read simulator output")
    while True:
        line = simulator_process.stdout.readline()
        if not line:
            print("[DEBUG] No more output from simulator")
            break
        line = line.strip()
        print(f"[C OUTPUT]: {line}")
        output.append(line)

        if line == "[END]":
            print("[DEBUG] Found [END] marker")
            break

    print(f"[DEBUG] Collected output: {output}")
    return "\n".join(output)



def send_command(command):
    """Send a command to the simulator and return output."""
    print(f"[DEBUG] Sending command: {command}")
    simulator_process.stdin.write(command + "\n")
    simulator_process.stdin.flush()
    output = read_output()
    print(f"[DEBUG] Command output: {output}")
    return output

@app.route("/load_instructions", methods=["POST"])
def loadInstructions():
    data = request.json
    instructions = data.get("instructions")

    if not instructions:
        return jsonify({"error": "No file contents provided."}), 400

    binary_lines = []
    raw_lines = []
    memory_content = []

    for index, instruction in enumerate(instructions):
        raw_lines.append(instruction)
        response = send_command(f"write {instruction}")

        for r in response.splitlines():
            if r.startswith("[BIN]"):
                val = int(r[5:])
                binary = format(val, "016b")
                binary_lines.append(binary)
            elif r.startswith("[MEM]"):
                addr, val = r[5:].split(":")
                memory_content.append((int(addr), int(val)))

    return jsonify({
        "message": "Instructions loaded.",
        "binary": binary_lines,
        "raw": raw_lines,
        "memory": memory_content
    })

@app.route("/execute_instructions", methods=["POST"])
def executeInstructions():
    print("[DEBUG] Starting execute_instructions")
    memory_content = []
    register_contents = []

    response = send_command("start")
    print(f"[DEBUG] Got response: {response}")

    for output in response.splitlines():
        print(f"[DEBUG] Processing line: {output}")
        if output.startswith("[MEM]"):
            addr, val = output[5:].split(":")
            memory_content.append((int(addr), int(val)))
        elif output.startswith("[REG]"):
            # Remove the [REG] prefix and split the rest
            reg_val = output[5:].split(":")
            register_contents.append((int(reg_val[0]), int(reg_val[1])))
            print(f"[DEBUG] Found register update: {reg_val[0]} = {reg_val[1]}")

    print(f"[DEBUG] Final register contents: {register_contents}")
    return jsonify({
        "message": "Execution Finished.",
        "memory": memory_content,
        "registers": register_contents
    })



if __name__ == "__main__":
    app.run(port=5000, debug=True)

