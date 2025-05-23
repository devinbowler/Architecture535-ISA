from flask import Flask, request, jsonify
import subprocess
import threading
from globals import (
    USER_DRAM_DELAY, USER_CACHE_DELAY,
    CACHE_ENABLED,  PIPELINE_ENABLED, CACHE_MODE
)

app = Flask(__name__)

# Launch C simulator executable
simulator_process = subprocess.Popen(
    ["../simulator/build/simulator"],
    stdin=subprocess.PIPE,
    stdout=subprocess.PIPE,
    stderr=subprocess.PIPE,
    text=True,
    bufsize=1
)

def read_output():
    output = []

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

    print(f"[DEBUG] Collected output: {len(output)} lines")
    return "\n".join(output)



def send_command(command):
    """Send a command to the simulator and return output."""
    print(f"[DEBUG] Sending command: {command}")
    simulator_process.stdin.write(command + "\n")
    simulator_process.stdin.flush()
    output = read_output()
    print(f"[DEBUG] Command output: {output}")
    return output

@app.route("/set_configuration", methods=["POST"])
def set_configuration():
    data = request.get_json(force=True)
    try:
        # Extract configuration values
        cache_enabled = bool(data.get("cache_enabled", True))
        pipeline_enabled = bool(data.get("pipeline_enabled", True))
        dram_delay = int(data.get("dram_delay", USER_DRAM_DELAY.value))
        cache_delay = int(data.get("cache_delay", USER_CACHE_DELAY.value))
        
        # Get Cache Mode
        cache_type = data.get("cache_type", "Direct-Mapped")
        cache_mode = 1  # Default to direct-mapped
        if cache_type == "Set Associative":
            cache_mode = 2
        
        # Set global values
        CACHE_ENABLED.value = 1 if cache_enabled else 0
        PIPELINE_ENABLED.value = 1 if pipeline_enabled else 0
        USER_DRAM_DELAY.value = dram_delay
        USER_CACHE_DELAY.value = cache_delay
        CACHE_MODE.value = cache_mode

        # Send explicit config command to C program
        config_cmd = f"config pipe={1 if pipeline_enabled else 0} cache={1 if cache_enabled else 0} dram={dram_delay} cache_delay={cache_delay} cache_mode={cache_mode}"
        print(f"[DEBUG] Sending config command: {config_cmd}")
        send_command(config_cmd)

        return jsonify({
            "message": "Configuration updated",
            "cache_enabled": cache_enabled,
            "pipeline_enabled": pipeline_enabled,
            "dram_delay": dram_delay,
            "cache_delay": cache_delay,
            "cache_type": cache_type,
            "cache_mode": cache_mode
        })
    except (ValueError, TypeError) as e:
        return jsonify({"error": f"bad config value: {e}"}), 400

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
    cache_contents = []
    cache_data_contents = []
    pipeline_state = []
    cycle_count = 0
    fetch_status = None
    pc_val = None

    response = send_command("start")
    print(f"[DEBUG] Got response: {response}")

    for output in response.splitlines():
        if output.startswith("[MEM]"):
            addr, val = output[5:].split(":")
            memory_content.append((int(addr), int(val)))
        elif output.startswith("[REG]"):
            # Remove the [REG] prefix and split the rest
            reg_val = output[5:].split(":")
            register_contents.append((int(reg_val[0]), int(reg_val[1])))
            # print(f"[DEBUG] Found register update: {reg_val[0]} = {reg_val[1]}")
        elif output.startswith("[CACHE]"):
            # Format: [CACHE]index:offset:valid:data
            # Example: [CACHE]2:0:1:42
            parts = output[7:].split(":")
            if len(parts) == 4:
                index, offset, valid, data = parts
                cache_contents.append((int(index), int(offset), int(valid) == 1, int(data)))
                # print(f"[DEBUG] Found cache update: index={index}, offset={offset}, valid={valid}, data={data}")
        elif output.startswith("[CACHE_DATA]"):
            # Format: [CACHE_DATA]index:offset:data_index:data_value
            # Example: [CACHE_DATA]2:0:1:42
            parts = output[12:].split(":")
            if len(parts) == 4:
                index, offset, data_index, data_value = parts
                cache_data_contents.append((int(index), int(offset), int(data_index), int(data_value)))
                # print(f"[DEBUG] Found cache data: index={index}, offset={offset}, data_index={data_index}, value={data_value}")
        elif output.startswith("[PIPELINE]"):
            parts = output[10:].split(":")
            if len(parts) >= 3:
                stage = parts[0]
                instruction = parts[1]
                pc = int(parts[2])
                pc_val = pc
                pipeline_state.append((stage, instruction, pc))
                print(f"[DEBUG] Found pipeline state: stage={stage}, instruction={instruction}, pc={pc}")
        elif output.startswith("[CYCLE]"):
            cycle_count = int(output[7:])
            print(f"[DEBUG] Current cycle: {cycle_count}")
        elif output.startswith("[FETCH_STATUS]"):
            fetch_status = output
            print(f"[DEBUG] Found fetch status: {output}")

    print(f"[DEBUG] Final register contents: {register_contents}")
    # print(f"[DEBUG] Final memory contents (showing first 10): {memory_content[:10]}")
    # print(f"[DEBUG] Final cache contents: {cache_contents}")
    # print(f"[DEBUG] Final cache data contents: {cache_data_contents}")
    
    return jsonify({
        "message": "Execution Finished.",
        "memory": memory_content,
        "registers": register_contents,
        "cache": cache_contents,
        "cache_data": cache_data_contents,
        "pipeline": pipeline_state,
        "cycle": cycle_count,
        "fetch_status": fetch_status,
        "pc": pc_val
    })


@app.route("/step_instruction", methods=["POST"])
def stepInstruction():
    print("[DEBUG] Starting step_instruction")
    memory_content = []
    register_contents = []
    cache_contents = []
    cache_data_contents = []
    pipeline_state = []
    cycle_count = 0
    fetch_status = None

    response = send_command("step")
    print(f"[DEBUG] Got response: {response}")

    for output in response.splitlines():
        if output.startswith("[MEM]"):
            addr, val = output[5:].split(":")
            memory_content.append((int(addr), int(val)))
        elif output.startswith("[REG]"):
            # Remove the [REG] prefix and split the rest
            reg_val = output[5:].split(":")
            register_contents.append((int(reg_val[0]), int(reg_val[1])))
            # print(f"[DEBUG] Found register update: {reg_val[0]} = {reg_val[1]}")
        elif output.startswith("[CACHE]"):
            # Format: [CACHE]index:offset:valid:data
            parts = output[7:].split(":")
            if len(parts) == 4:
                index, offset, valid, data = parts
                cache_contents.append((int(index), int(offset), int(valid) == 1, int(data)))
                # print(f"[DEBUG] Found cache update: index={index}, offset={offset}, valid={valid}, data={data}")
        elif output.startswith("[CACHE_DATA]"):
            # Format: [CACHE_DATA]index:offset:data_index:data_value
            parts = output[12:].split(":")
            if len(parts) == 4:
                index, offset, data_index, data_value = parts
                cache_data_contents.append((int(index), int(offset), int(data_index), int(data_value)))
                # print(f"[DEBUG] Found cache data: index={index}, offset={offset}, data_index={data_index}, value={data_value}")
        elif output.startswith("[PIPELINE]"):
            # Format: [PIPELINE]stage:instruction:pc
            # Example: [PIPELINE]FETCH:ADD R5, R4, R3:0
            parts = output[10:].split(":")
            if len(parts) >= 3:
                stage = parts[0]
                instruction = parts[1]
                pc = int(parts[2])
                pipeline_state.append((stage, instruction, pc))
                print(f"[DEBUG] Found pipeline state: stage={stage}, instruction={instruction}, pc={pc}")
        elif output.startswith("[CYCLE]"):
            # Extract the cycle count
            cycle_count = int(output[7:])
            print(f"[DEBUG] Current cycle: {cycle_count}")
        elif output.startswith("[FETCH_STATUS]"):
            # Capture fetch status for UI
            fetch_status = output
            print(f"[DEBUG] Found fetch status: {output}")

    print(f"[DEBUG] Final pipeline state: {pipeline_state}")
    
    return jsonify({
        "message": "Step Completed",
        "memory": memory_content,
        "registers": register_contents,
        "cache": cache_contents,
        "cache_data": cache_data_contents,
        "pipeline": pipeline_state,
        "cycle": cycle_count,  # Include actual cycle count
        "fetch_status": fetch_status  # Include fetch status for UI
    })

@app.route("/reset", methods=["POST"])
def resetSimulator():
    print("[DEBUG] Starting simulator reset")
    
    # Send reset command to simulator
    response = send_command("reset")
    print(f"[DEBUG] Got reset response: {response}")
    
    # Initialize empty state
    memory_content = [(i, 0) for i in range(1000)]  # All memory locations set to 0
    register_contents = [(i, 1) if i == 1 else (i, 0) for i in range(16)]
    cache_contents = []  # Empty cache
    cache_data_contents = []  # Empty cache data
    pipeline_state = [  # Empty pipeline stages
        ("FETCH", "Bubble", 0),
        ("DECODE", "Bubble", 0),
        ("EXECUTE", "Bubble", 0),
        ("MEMORY", "Bubble", 0),
        ("WRITEBACK", "Bubble", 0)
    ]
    
    return jsonify({
        "message": "Simulator Reset Complete",
        "memory": memory_content,
        "registers": register_contents,
        "cache": cache_contents,
        "cache_data": cache_data_contents,
        "pipeline": pipeline_state,
        "cycle": 0
    })

if __name__ == "__main__":
    app.run(port=5000, debug=True)

