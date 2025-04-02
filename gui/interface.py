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
        palette.setColor(QPalette.ColorRole.Window, QColor(45, 45, 45))
        palette.setColor(QPalette.ColorRole.WindowText, QColor(200, 200, 200))
        palette.setColor(QPalette.ColorRole.Base, QColor(30, 30, 30))
        palette.setColor(QPalette.ColorRole.AlternateBase, QColor(45, 45, 45))
        palette.setColor(QPalette.ColorRole.ToolTipBase, QColor(255, 255, 255))
        palette.setColor(QPalette.ColorRole.ToolTipText, QColor(0, 0, 0))
        palette.setColor(QPalette.ColorRole.Text, QColor(200, 200, 200))
        palette.setColor(QPalette.ColorRole.Button, QColor(45, 45, 45))
        palette.setColor(QPalette.ColorRole.ButtonText, QColor(200, 200, 200))
        palette.setColor(QPalette.ColorRole.BrightText, QColor(255, 0, 0))
        palette.setColor(QPalette.ColorRole.Link, QColor(42, 130, 218))
        
        self.setPalette(palette)
    
    def create_data_section(self):
        widget = QWidget()
        layout = QVBoxLayout()
        
        tab_widget = QTabWidget()
        self.register_table = self.create_table(16, ["QR - Integer Registers"])
        self.memory_table = self.create_table(1000, ["Value"])
        self.cache_table = self.create_table(16, ["Index", "Offset", "Valid", "Data"])
        
        tab_widget.addTab(self.register_table, "Registers")
        tab_widget.addTab(self.memory_table, "Memory")
        tab_widget.addTab(self.cache_table, "Cache")
        
        layout.addWidget(tab_widget)
        widget.setLayout(layout)
        return widget
    
    def create_instruction_section(self):
        widget = QWidget()
        layout = QVBoxLayout()
        
        tab_widget = QTabWidget()
        self.instruction_table = self.create_table(16, ["Memory Address", "Text Instruction", "Hex Instruction"])
        self.code_table = self.create_table(16, ["Code"])
        self.pipeline_table = self.create_table(5, ["Fetch", "Decode", "Execute", "Memory", "Writeback"])
        
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
        step_button = QPushButton("Step 1 Cycle")
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
        print("[DEBUG] Starting execute_instructions")
        self.thread = execute_instructionsThread(self.api_url)
        self.thread.finished.connect(self.handle_execution_result)
        self.thread.error.connect(self.handle_instruction_error)
        self.thread.start()
        print("[DEBUG] execute_instructionsThread started")

    def handle_execution_result(self, result):
        print("[DEBUG] handle_execution_result called with result:", result)
        
        # Check for errors
        if "error" in result:
            QMessageBox.information(self, "Error", result["error"])
            return

        # Update memory table - first clear and resize
        memory = result.get("memory", [])
        print(f"[DEBUG] Received {len(memory)} memory updates")
        
        # Set row headers to be the memory addresses for all 1000 memory locations
        address_labels = [str(i) for i in range(1000)]
        self.memory_table.setVerticalHeaderLabels(address_labels)
        
        # Clear all cells first with zeros
        for i in range(1000):
            self.memory_table.setItem(i, 0, QTableWidgetItem("0000000000000000"))
        
        # Now update with actual values from the execution
        for addr, val in memory:
            try:
                addr = int(addr)
                if addr >= 1000:
                    continue  # Skip addresses beyond our table size
                    
                val = int(val)
                binary_val = format(val & 0xFFFF, "016b")
                
                print(f"[DEBUG] Setting memory[{addr}] to {binary_val} (decimal: {val})")
                item = QTableWidgetItem(binary_val)
                self.memory_table.setItem(addr, 0, item)
            except Exception as e:
                print(f"[UI ERROR] Failed to set memory[{addr}]: {e}")

        # Update register table with values from API response
        registers = result.get("registers", [])
        print(f"[DEBUG] Received register updates: {registers}")
            
        for reg, val in registers:
            try:
                reg = int(reg)
                if reg >= 16:  # Only update the first 16 registers
                    continue
                    
                val = int(val)
                binary_val = format(val & 0xFFFF, "016b")
                print(f"[DEBUG] Setting register[{reg}] to {binary_val} (decimal: {val})")
                item = QTableWidgetItem(binary_val)
                self.register_table.setItem(reg, 0, item)
            except Exception as e:
                print(f"[UI ERROR] Failed to set register[{reg}]: {e}")

        # Force UI update
        self.memory_table.viewport().update()
        self.memory_table.update()
        self.register_table.viewport().update()
        self.register_table.update()
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




if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "--watch":
        start_application()
    else:
        app = QApplication(sys.argv)
        window = ISASimulatorUI()
        window.show()
        sys.exit(app.exec())

