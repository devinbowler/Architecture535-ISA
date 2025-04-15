import sys
import os
import time
import subprocess
import requests
from PyQt6.QtWidgets import (
    QApplication, QWidget, QVBoxLayout, QHBoxLayout, QLabel, QPushButton, QSizePolicy, QSpacerItem,
    QTableWidget, QTabWidget, QSpinBox, QCheckBox, QComboBox, QGridLayout, QLineEdit, QHeaderView,
    QFileDialog, QMessageBox, QTableWidgetItem
)
from PyQt6.QtCore import Qt, QThread, pyqtSignal
from PyQt6.QtGui import QPalette, QColor
from watchdog.observers import Observer
from watchdog.events import FileSystemEventHandler

SCRIPT_PATH = os.path.abspath(__file__)

# Run request in a thread to prevent freezing
class load_instructionsThread(QThread):
    finished = pyqtSignal(dict)
    error = pyqtSignal(str)

    def __init__(self, api_url, instructions):
        super().__init__()
        self.api_url = api_url
        self.instructions = instructions

    def run(self):
        import requests
        try:
            response = requests.post(
                f"{self.api_url}/load_instructions",
                json={"instructions": self.instructions},
                timeout=10 
            )
            result = response.json()
            self.finished.emit(result)
        except Exception as e:
            self.error.emit(str(e))

class execute_instructionsThread(QThread):
    finished = pyqtSignal(dict)
    error = pyqtSignal(str)

    def __init__(self, api_url):
        super().__init__()
        self.api_url = api_url

    def run(self):
        print("[DEBUG] execute_instructionsThread running")
        try:
            # Increase the timeout value
            response = requests.post(f"{self.api_url}/execute_instructions", timeout=30)
            result = response.json()
            print("[DEBUG] Full API response:", result)
            if "error" in result:
                self.error.emit(result["error"])
            else:
                self.finished.emit(result)
        except requests.exceptions.Timeout:
            print("[DEBUG] Request timed out")
            self.error.emit("Request timed out")
        except Exception as e:
            print("[DEBUG] API call failed:", str(e))
            self.error.emit(str(e))

class step_instructionThread(QThread):
    finished = pyqtSignal(dict)
    error = pyqtSignal(str)

    def __init__(self, api_url):
        super().__init__()
        self.api_url = api_url

    def run(self):
        print("[DEBUG] step_instructionThread running")
        try:
            response = requests.post(f"{self.api_url}/step_instruction", timeout=10)
            result = response.json()
            print("[DEBUG] API step response:", result)
            if "error" in result:
                self.error.emit(result["error"])
            else:
                self.finished.emit(result)
        except Exception as e:
            print("[DEBUG] API step call failed:", str(e))
            self.error.emit(str(e))

class resetThread(QThread):
    finished = pyqtSignal(dict)
    error = pyqtSignal(str)

    def __init__(self, api_url):
        super().__init__()
        self.api_url = api_url

    def run(self):
        try:
            response = requests.post(f"{self.api_url}/reset", timeout=10)
            result = response.json()
            if "error" in result:
                self.error.emit(result["error"])
            else:
                self.finished.emit(result)
        except Exception as e:
            self.error.emit(str(e))

class FileChangeHandler(FileSystemEventHandler):
    """ Watches for file changes and restarts the application """
    def __init__(self, process):
        self.process = process

    def on_modified(self, event):
        if event.src_path == SCRIPT_PATH:  # Restart only if main script changes
            print("\nFile changed! Restarting...\n")
            self.process.terminate()  # Kill old process
            time.sleep(1)  # Short delay to avoid race conditions
            self.process = subprocess.Popen([sys.executable, SCRIPT_PATH])  # Restart app

def start_application():
    """ Runs the PyQt6 application and monitors for file changes """
    process = subprocess.Popen([sys.executable, SCRIPT_PATH])  # Start app
    observer = Observer()
    handler = FileChangeHandler(process)
    observer.schedule(handler, path=os.path.dirname(SCRIPT_PATH), recursive=False)
    observer.start()

    try:
        while True:
            time.sleep(1)  # Keep running
    except KeyboardInterrupt:
        observer.stop()
        process.terminate()

    observer.join()

class ISASimulatorUI(QWidget):
    def __init__(self):
        super().__init__()
        self.api_url = "http://127.0.0.1:5000"
        self.setWindowTitle("ARCH-16: Instruction Set Architecture")
        self.setGeometry(100, 100, 1200, 800)
        self.apply_dark_mode()
        
        main_layout = QHBoxLayout()
        
        # Data Section
        self.data_section = self.create_data_section()
        main_layout.addWidget(self.data_section, 2)
        
        # Instruction Section
        self.instruction_section = self.create_instruction_section()
        main_layout.addWidget(self.instruction_section, 3)
        
        # Configuration Section
        self.config_section = self.create_config_section()
        main_layout.addWidget(self.config_section, 1)
        
        self.setLayout(main_layout)

    def apply_dark_mode(self):
        palette = QPalette()
        palette.setColor(QPalette.ColorRole.Window, QColor(25, 25, 25))         # Very dark background
        palette.setColor(QPalette.ColorRole.WindowText, QColor(220, 220, 220))   # Very light text
        palette.setColor(QPalette.ColorRole.Base, QColor(18, 18, 18))           # Even darker base
        palette.setColor(QPalette.ColorRole.AlternateBase, QColor(30, 30, 30))   # Slightly lighter alt base
        palette.setColor(QPalette.ColorRole.ToolTipBase, QColor(25, 25, 25))     # Dark tooltip
        palette.setColor(QPalette.ColorRole.ToolTipText, QColor(220, 220, 220))  # Light tooltip text
        palette.setColor(QPalette.ColorRole.Text, QColor(220, 220, 220))         # Light text
        palette.setColor(QPalette.ColorRole.Button, QColor(30, 30, 30))          # Dark button
        palette.setColor(QPalette.ColorRole.ButtonText, QColor(220, 220, 220))   # Light button text
        palette.setColor(QPalette.ColorRole.BrightText, QColor(255, 128, 128))   # Bright error text
        palette.setColor(QPalette.ColorRole.Link, QColor(42, 130, 218))          # Blue links
        
        self.setPalette(palette)
    
    def create_data_section(self):
        widget = QWidget()
        layout = QVBoxLayout()
        
        tab_widget = QTabWidget()
        self.register_table = self.create_table(16, ["QR - Integer Registers"])
        self.memory_table = self.create_table(1000, ["Value"])
        self.cache_table = self.create_table(16, ["Index", "Offset", "Valid", "Tag", "Data[0]", "Data[1]", "Data[2]", "Data[3]"])
        
        tab_widget.addTab(self.register_table, "Registers")
        tab_widget.addTab(self.memory_table, "Memory")
        tab_widget.addTab(self.cache_table, "Cache")
        
        layout.addWidget(tab_widget)
        widget.setLayout(layout)
        return widget
    
    def create_instruction_section(self):
        widget = QWidget()
        layout = QVBoxLayout()
        
        # Add cycle counter at the top with larger font
        self.cycle_counter = QLabel("Cycle: 0")
        self.cycle_counter.setStyleSheet("font-size: 18px; font-weight: bold; margin: 10px; color: #4488cc;")
        self.cycle_counter.setAlignment(Qt.AlignmentFlag.AlignCenter)
        layout.addWidget(self.cycle_counter)
        
        tab_widget = QTabWidget()
        self.instruction_table = self.create_table(16, ["Memory Address", "Text Instruction", "Hex Instruction"])
        self.code_table = self.create_table(16, ["Code"])
        
        # Improved pipeline table with all 5 stages - show more details and values
        self.pipeline_table = self.create_table(5, ["Stage", "Status", "Instruction", "PC"])
        
        # Initialize with stage names and empty statuses
        stages = ["Fetch", "Decode", "Execute", "Memory", "Writeback"]
        for i, stage in enumerate(stages):
            self.pipeline_table.setItem(i, 0, QTableWidgetItem(stage))
            status_item = QTableWidgetItem("Empty")
            status_item.setBackground(QColor(20, 20, 20))  # Very dark gray for empty
            status_item.setForeground(QColor(150, 150, 150))  # Mid-gray text
            self.pipeline_table.setItem(i, 1, status_item)
            
            # Initialize instruction and values columns
            instr_item = QTableWidgetItem("-")
            instr_item.setBackground(QColor(20, 20, 20))
            instr_item.setForeground(QColor(150, 150, 150))
            self.pipeline_table.setItem(i, 2, instr_item)
            
            pc_item = QTableWidgetItem("-")
            pc_item.setBackground(QColor(20, 20, 20))
            pc_item.setForeground(QColor(150, 150, 150))
            self.pipeline_table.setItem(i, 3, pc_item)
        
        tab_widget.addTab(self.instruction_table, "Instructions")
        tab_widget.addTab(self.code_table, "Code")
        tab_widget.addTab(self.pipeline_table, "Pipeline")
        
        layout.addWidget(tab_widget)
        
        widget.setLayout(layout)
        return widget
    
    def create_config_section(self):
        widget = QWidget()
        layout = QVBoxLayout()

        layout.addSpacerItem(QSpacerItem(24, 24, QSizePolicy.Policy.Minimum, QSizePolicy.Policy.Fixed))
        
        # Config Settings
        config_layout = QGridLayout()
        
        config_layout.addWidget(QLabel("Add Breakpoint"), 0, 0)
        self.breakpoint_input = QLineEdit()
        config_layout.addWidget(self.breakpoint_input, 0, 1)

        config_layout.addWidget(QLabel("Cache Enabled"), 1, 0)
        self.cache_enabled = QCheckBox()
        config_layout.addWidget(self.cache_enabled, 1, 1)
        
        config_layout.addWidget(QLabel("Cache Type"), 2, 0)
        self.cache_type = QComboBox()
        self.cache_type.addItems(["Direct-Mapped", "Fully Associative", "Set Associative"])
        config_layout.addWidget(self.cache_type, 2, 1)
        
        config_layout.addWidget(QLabel("DRAM Delay"), 3, 0)
        self.dram_delay = QSpinBox()
        self.dram_delay.setMaximum(1000)
        config_layout.addWidget(self.dram_delay, 3, 1)
        
        config_layout.addWidget(QLabel("Cache Delay"), 4, 0)
        self.cache_delay = QSpinBox()
        self.cache_delay.setMaximum(1000)
        config_layout.addWidget(self.cache_delay, 4, 1)
        
        layout.addLayout(config_layout)
        
        # Set Config Button
        layout.addWidget(QPushButton("Set Configuration"))

        # Add spacer to push execution buttons to the bottom
        layout.addSpacerItem(QSpacerItem(20, 40, QSizePolicy.Policy.Minimum, QSizePolicy.Policy.Expanding))
        
        # Execution Controls - Grouped together
        button_layout = QVBoxLayout()
        button_layout.setSpacing(5)

        load_button = QPushButton("Load Instructions")
        load_button.clicked.connect(self.load_instructions)
        
        clear_button = QPushButton("Clear Instructions")
        clear_button.clicked.connect(self.reset_simulator)
        
        step_button = QPushButton("Step 1 Cycle")
        step_button.clicked.connect(self.step_instruction)
        
        run_button = QPushButton("Run Instructions")
        run_button.clicked.connect(self.execute_instructions)

        button_layout.addWidget(load_button)
        button_layout.addWidget(clear_button)
        button_layout.addWidget(step_button)
        button_layout.addWidget(run_button)

        layout.addLayout(button_layout)  # Add compact button layout

        widget.setLayout(layout)
        return widget

    def create_table(self, rows, headers):
        table = QTableWidget()
        table.setRowCount(rows)
        table.setColumnCount(len(headers))
        table.setHorizontalHeaderLabels(headers)

        # Set vertical header labels for register table (0-15)
        if headers == ["QR - Integer Registers"]:
            table.setVerticalHeaderLabels([f"R{i}" for i in range(16)])
            table.setRowCount(16)  # Ensure exactly 16 registers

        # Enable resizing for columns and rows
        table.horizontalHeader().setSectionResizeMode(QHeaderView.ResizeMode.Stretch)  # Columns stretch
        table.verticalHeader().setSectionResizeMode(QHeaderView.ResizeMode.Stretch)    # Rows stretch

        table.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Expanding)  # Make table expand fully
        return table


    def handle_instruction_error(self, result):
        print("RESULT FROM API:", result)

        if "error" in result:
            QMessageBox.information(self, "Error", result["error"])
            return


    # Functions for UI functionality.
    def load_instructions(self):
        file_path, _ = QFileDialog.getOpenFileName(self, "Select Instruction File", "", "Text Files (*.txt)")

        if file_path:  # If a file was selected
            with open(file_path, "r") as file:
                instructions = [line.strip() for line in file.readlines()]

            self.thread = load_instructionsThread(self.api_url, instructions)
            self.thread.finished.connect(self.handle_instruction_result)
            self.thread.error.connect(self.handle_instruction_error)
            self.thread.start()

    def execute_instructions(self):
        # Check cache settings
        cache_enabled = self.cache_enabled.isChecked()
        cache_type = self.cache_type.currentText()
        
        print(f"[UI] Executing with cache enabled: {cache_enabled}, type: {cache_type}")
        
        # Start thread to call API
        self.execute_thread = execute_instructionsThread(self.api_url)
        self.execute_thread.finished.connect(self.handle_execution_result)
        self.execute_thread.error.connect(self.handle_instruction_error)
        self.execute_thread.start()

    def handle_execution_result(self, result):
        # print("[DEBUG] handle_execution_result called with result:", result)
        
        # Check for errors
        if "error" in result:
            QMessageBox.information(self, "Error", result["error"])
            return

        # Update register table with values from API response
        registers = result.get("registers", [])
        print(f"[DEBUG] Received register updates: {registers}")
        
        # Ensure register table has enough rows
        if len(registers) > self.register_table.rowCount():
            self.register_table.setRowCount(len(registers))
            
        for reg, val in registers:
            try:
                reg = int(reg)
                val = int(val)
                binary_val = format(val & 0xFFFF, "016b")
                print(f"[DEBUG] Setting register[{reg}] to {binary_val} (decimal: {val})")
                item = QTableWidgetItem(binary_val)
                self.register_table.setItem(reg, 0, item)
                # Force the table to update
                self.register_table.viewport().update()
            except Exception as e:
                print(f"[UI ERROR] Failed to set register[{reg}]: {e}")

        # Update memory table
        memory = result.get("memory", [])
        for addr, val in memory:
            try:
                addr = int(addr)
                val = int(val)
                binary_val = format(val & 0xFFFF, "016b")

                # Ensure enough rows
                if addr >= self.memory_table.rowCount():
                    self.memory_table.setRowCount(addr + 1)

                self.memory_table.setItem(addr, 0, QTableWidgetItem(binary_val))
            except Exception as e:
                print(f"[UI ERROR] Failed to set memory[{addr}]: {e}")
                
        # Update cache table
        cache = result.get("cache", [])
        cache_data = result.get("cache_data", [])
        print(f"[DEBUG] Received cache updates: {len(cache)} entries")
        print(f"[DEBUG] Received cache data updates: {len(cache_data)} entries")
        
        # Process cache information: cache contains (index, offset, valid, tag)
        # Create a dictionary to organize cache entries
        cache_entries = {}
        
        # First collect all cache metadata
        for index, offset, valid, tag in cache:
            key = f"{index}:{offset}"
            cache_entries[key] = {
                "index": index,
                "offset": offset,
                "valid": valid,
                "tag": tag,
                "data": [0, 0, 0, 0]  # Initialize with zeros
            }
        
        # Then fill in the cache data values
        for index, offset, data_idx, data_val in cache_data:
            key = f"{index}:{offset}"
            if key in cache_entries and 0 <= data_idx < 4:
                cache_entries[key]["data"][data_idx] = data_val
        
        # Now populate the cache table
        self.cache_table.setRowCount(len(cache_entries))
        
        for i, (key, entry) in enumerate(cache_entries.items()):
            try:
                self.cache_table.setItem(i, 0, QTableWidgetItem(str(entry["index"])))
                self.cache_table.setItem(i, 1, QTableWidgetItem(str(entry["offset"])))
                self.cache_table.setItem(i, 2, QTableWidgetItem("Valid" if entry["valid"] else "Invalid"))
                self.cache_table.setItem(i, 3, QTableWidgetItem(str(entry["tag"])))
                
                # Add the data values
                for j in range(4):
                    self.cache_table.setItem(i, 4 + j, QTableWidgetItem(str(entry["data"][j])))
                
                # print(f"[DEBUG] Updated cache line {i}: {entry}")
            except Exception as e:
                print(f"[UI ERROR] Failed to update cache line {i}: {e}")

        # Force UI update
        self.register_table.update()
        self.memory_table.update()
        self.cache_table.update()
        self.update()

        # Check if 'message' key exists
        if "message" in result:
            QMessageBox.information(self, "Execution Complete", result["message"])
        else:
            QMessageBox.information(self, "Execution Complete", "Execution finished without a message.")

    def handle_instruction_result(self, result):
        # Error print
        if "error" in result:
            QMessageBox.information(self, "Error", result["error"])
            return

        binary = result.get("binary", [])
        raw = result.get("raw", [])
        memory = result.get("memory", [])

        # Update code and instruction tables
        self.code_table.setRowCount(len(raw))
        self.instruction_table.setRowCount(len(raw))
        for i, (asm, bin_val) in enumerate(zip(raw, binary)):
            self.code_table.setItem(i, 0, QTableWidgetItem(asm))
            self.instruction_table.setItem(i, 0, QTableWidgetItem(str(i)))
            self.instruction_table.setItem(i, 1, QTableWidgetItem(asm))
            self.instruction_table.setItem(i, 2, QTableWidgetItem(bin_val))

        # Initialize memory table with zeros for all 1000 addresses
        # Set row headers to be the memory addresses
        address_labels = [str(i) for i in range(1000)]
        self.memory_table.setVerticalHeaderLabels(address_labels)
        
        # Set all cells to zero first
        for i in range(1000):
            self.memory_table.setItem(i, 0, QTableWidgetItem("0000000000000000"))

        # Update memory with loaded instructions
        for addr, val in memory:
            try:
                addr = int(addr)
                if addr >= 1000:
                    continue  # Skip addresses beyond our table size
                    
                val = int(val)
                binary_val = format(val & 0xFFFF, "016b")
                print(f"[UI] Setting memory[{addr}] = {binary_val}")
                self.memory_table.setItem(addr, 0, QTableWidgetItem(binary_val))
            except Exception as e:
                print(f"[UI ERROR] Failed to set memory[{addr}]: {e}")

        QMessageBox.information(self, "file loaded", result["message"])

    def step_instruction(self):
        """Execute a single pipeline cycle"""
        print("[DEBUG] Starting step_instruction")
        
        # Start thread to call API
        self.step_thread = step_instructionThread(self.api_url)
        self.step_thread.finished.connect(self.handle_step_result)
        self.step_thread.error.connect(self.handle_instruction_error)
        self.step_thread.start()
        print("[DEBUG] step_instructionThread started")

    def handle_step_result(self, result):
        """Handle the result from a step instruction request"""
        # print("[DEBUG] handle_step_result called with result:", result)
        
        # Check for errors
        if "error" in result:
            QMessageBox.information(self, "Error", result["error"])
            return
            
        # Update the pipeline visualization first
        pipeline_state = result.get("pipeline", [])
        print(f"[DEBUG] Received pipeline state: {pipeline_state}")
        
        # Map stage names to row indices
        stage_to_row = {
            "FETCH": 0,
            "DECODE": 1,
            "EXECUTE": 2,
            "MEMORY": 3,
            "WRITEBACK": 4
        }
        
        # Dictionary to track which stages have been updated
        # We'll mark unmentioned stages as bubbles
        updated_stages = {i: False for i in range(5)}
        
        # Update pipeline table
        for stage, instruction, pc in pipeline_state:
            if stage in stage_to_row:
                row = stage_to_row[stage]
                updated_stages[row] = True
                
                # Determine status and color based on instruction content
                if instruction.lower() == "bubble" or instruction.lower() == "nop":
                    # This is a bubble or NOP
                    status_item = QTableWidgetItem("NOP" if instruction.lower() == "nop" else "Bubble")
                    status_item.setBackground(QColor(40, 15, 15))  # Very dark red for bubble/NOP
                    status_item.setForeground(QColor(180, 180, 180))  # Light gray text
                    self.pipeline_table.setItem(row, 1, status_item)
                    
                    # Set instruction and PC with red background too
                    instr_item = QTableWidgetItem("-")
                    instr_item.setBackground(QColor(35, 10, 10))  # Even darker red
                    instr_item.setForeground(QColor(180, 180, 180))  # Light gray text
                    
                    pc_item = QTableWidgetItem("-")
                    pc_item.setBackground(QColor(35, 10, 10))  # Even darker red
                    pc_item.setForeground(QColor(180, 180, 180))  # Light gray text
                    
                    self.pipeline_table.setItem(row, 2, instr_item)
                    self.pipeline_table.setItem(row, 3, pc_item)
                else:
                    # This is a valid instruction
                    status_item = QTableWidgetItem("Valid")
                    status_item.setBackground(QColor(15, 40, 15))  # Very dark green for valid
                    self.pipeline_table.setItem(row, 1, status_item)
                    
                    # Set instruction and PC with green background
                    instr_item = QTableWidgetItem(instruction)
                    instr_item.setBackground(QColor(10, 35, 10))  # Even darker green
                    instr_item.setForeground(QColor(180, 180, 180))  # Light gray text
                    
                    pc_item = QTableWidgetItem(str(pc))
                    pc_item.setBackground(QColor(10, 35, 10))  # Even darker green
                    pc_item.setForeground(QColor(180, 180, 180))  # Light gray text
                    
                    self.pipeline_table.setItem(row, 2, instr_item)
                    self.pipeline_table.setItem(row, 3, pc_item)
                
                print(f"[DEBUG] Updated pipeline stage {stage}: {instruction}, PC={pc}")
        
        # For any stage not explicitly mentioned, mark as bubble
        for row, updated in updated_stages.items():
            if not updated:
                # This stage wasn't mentioned in the update, assume it's a bubble
                status_item = QTableWidgetItem("Bubble")
                status_item.setBackground(QColor(40, 15, 15))  # Very dark red for bubble
                status_item.setForeground(QColor(180, 180, 180))  # Light gray text
                self.pipeline_table.setItem(row, 1, status_item)
                
                # Also update instruction and PC as empty with red background
                instr_item = QTableWidgetItem("-")
                instr_item.setBackground(QColor(35, 10, 10))  # Even darker red
                instr_item.setForeground(QColor(180, 180, 180))  # Light gray text
                
                pc_item = QTableWidgetItem("-")
                pc_item.setBackground(QColor(35, 10, 10))  # Even darker red
                pc_item.setForeground(QColor(180, 180, 180))  # Light gray text
                
                self.pipeline_table.setItem(row, 2, instr_item)
                self.pipeline_table.setItem(row, 3, pc_item)
                
        # Update cycle counter
        cycle = result.get("cycle", 0)
        if cycle > 0:
            self.setWindowTitle(f"ARCH-16: Instruction Set Architecture - Cycle {cycle}")
            # Update the cycle counter label
            self.cycle_counter.setText(f"Cycle: {cycle}")
            # Use color to make the cycle number more visible
            self.cycle_counter.setStyleSheet(f"font-size: 18px; font-weight: bold; margin: 10px; color: #0066cc;")
            
        # Now update all other components using the existing handlers
        self.handle_execution_result(result)
        
        # No need for a popup message after each step
        # Just update the UI silently

    def reset_simulator(self):
        """Reset the entire simulator state"""
        print("[DEBUG] Starting simulator reset")
        
        # Start thread to call API
        self.reset_thread = resetThread(self.api_url)
        self.reset_thread.finished.connect(self.handle_reset_result)
        self.reset_thread.error.connect(self.handle_instruction_error)
        self.reset_thread.start()

    def handle_reset_result(self, result):
        """Handle the result from a reset request"""
        print("[DEBUG] handle_reset_result called with result:", result)
        
        # Reset cycle counter
        self.cycle_counter.setText("Cycle: 0")
        self.setWindowTitle("ARCH-16: Instruction Set Architecture")
        
        # Clear instruction and code tables
        self.clear_table(self.instruction_table)
        self.clear_table(self.code_table)
        
        # Update all other components using existing handler
        self.handle_execution_result(result)

    def clear_table(self, table):
        """Helper to clear a table's contents"""
        for row in range(table.rowCount()):
            for col in range(table.columnCount()):
                table.setItem(row, col, QTableWidgetItem(""))




if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "--watch":
        start_application()
    else:
        app = QApplication(sys.argv)
        window = ISASimulatorUI()
        window.show()
        sys.exit(app.exec())

