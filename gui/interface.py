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
        palette.setColor(QPalette.ColorRole.Window, QColor(25, 25, 25))
        palette.setColor(QPalette.ColorRole.WindowText, QColor(220, 220, 220))
        palette.setColor(QPalette.ColorRole.Base, QColor(18, 18, 18))
        palette.setColor(QPalette.ColorRole.AlternateBase, QColor(30, 30, 30))
        palette.setColor(QPalette.ColorRole.ToolTipBase, QColor(25, 25, 25))
        palette.setColor(QPalette.ColorRole.ToolTipText, QColor(220, 220, 220))
        palette.setColor(QPalette.ColorRole.Text, QColor(220, 220, 220))
        palette.setColor(QPalette.ColorRole.Button, QColor(30, 30, 30))
        palette.setColor(QPalette.ColorRole.ButtonText, QColor(220, 220, 220))
        palette.setColor(QPalette.ColorRole.BrightText, QColor(255, 128, 128))
        palette.setColor(QPalette.ColorRole.Link, QColor(42, 130, 218))
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
        
        tab_widget = QTabWidget()
        self.instruction_table = self.create_table(16, ["Memory Address", "Text Instruction", "Hex Instruction"])
        self.pipeline_table = self.create_table(5, ["Stage", "Status", "Instruction", "PC"])
        
        stages = ["Fetch", "Decode", "Execute", "Memory", "Writeback"]
        for i, stage in enumerate(stages):
            self.pipeline_table.setItem(i, 0, QTableWidgetItem(stage))
            status_item = QTableWidgetItem("Empty")
            status_item.setBackground(QColor(20, 20, 20))
            status_item.setForeground(QColor(150, 150, 150))
            self.pipeline_table.setItem(i, 1, status_item)
            instr_item = QTableWidgetItem("-")
            instr_item.setBackground(QColor(20, 20, 20))
            instr_item.setForeground(QColor(150, 150, 150))
            self.pipeline_table.setItem(i, 2, instr_item)
            pc_item = QTableWidgetItem("-")
            pc_item.setBackground(QColor(20, 20, 20))
            pc_item.setForeground(QColor(150, 150, 150))
            self.pipeline_table.setItem(i, 3, pc_item)
        
        tab_widget.addTab(self.instruction_table, "Instructions")
        tab_widget.addTab(self.pipeline_table, "Pipeline")
        layout.addWidget(tab_widget)
        widget.setLayout(layout)
        return widget
    
    def create_config_section(self):
        widget = QWidget()
        layout = QVBoxLayout()

        layout.addSpacerItem(QSpacerItem(24, 24, QSizePolicy.Policy.Minimum, QSizePolicy.Policy.Fixed))
        
        config_layout = QGridLayout()
        config_layout.addWidget(QLabel("Cache Enabled"), 1, 0)
        self.cache_enabled = QCheckBox()
        self.cache_enabled.setChecked(True)
        config_layout.addWidget(self.cache_enabled, 1, 1)
        config_layout.addWidget(QLabel("Pipeline Enabled"), 2, 0)
        self.pipeline_enabled = QCheckBox()
        self.pipeline_enabled.setChecked(True)
        config_layout.addWidget(self.pipeline_enabled, 2, 1)
        config_layout.addWidget(QLabel("Cache Type"), 3, 0)
        self.cache_type = QComboBox()
        self.cache_type.addItems(["Direct-Mapped", "Fully Associative", "Set Associative"])
        config_layout.addWidget(self.cache_type, 3, 1)
        config_layout.addWidget(QLabel("DRAM Delay"), 4, 0)
        self.dram_delay = QSpinBox()
        self.dram_delay.setMaximum(1000)
        config_layout.addWidget(self.dram_delay, 4, 1)
        config_layout.addWidget(QLabel("Cache Delay"), 5, 0)
        self.cache_delay = QSpinBox()
        self.cache_delay.setMaximum(1000)
        config_layout.addWidget(self.cache_delay, 5, 1)
        layout.addLayout(config_layout)

        set_cfg = QPushButton("Set Configuration")
        set_cfg.clicked.connect(self.set_configuration)
        layout.addWidget(set_cfg)

        status_frame = QWidget()
        status_layout = QVBoxLayout()
        status_frame.setLayout(status_layout)
        status_layout.addWidget(QLabel("<b>Simulator Status</b>"))
        self.cycle_display = QLabel("Cycle: 0")
        self.pc_display    = QLabel("PC: 0")
        for w in (self.cycle_display, self.pc_display):
            w.setAlignment(Qt.AlignmentFlag.AlignLeft)
            status_layout.addWidget(w)
        layout.addWidget(status_frame)

        layout.addSpacerItem(QSpacerItem(20, 40, QSizePolicy.Policy.Minimum, QSizePolicy.Policy.Expanding))
        
        button_layout = QVBoxLayout()
        button_layout.setSpacing(5)

        load_button = QPushButton("Load Instructions")
        load_button.clicked.connect(self.load_instructions)
        clear_button = QPushButton("Reset ARCH-16")
        clear_button.clicked.connect(self.reset_simulator)
        step_button = QPushButton("Step 1 Cycle")
        step_button.clicked.connect(self.step_instruction)
        run_button = QPushButton("Run Instructions")
        run_button.clicked.connect(self.execute_instructions)

        button_layout.addWidget(load_button)
        button_layout.addWidget(clear_button)
        button_layout.addWidget(step_button)
        button_layout.addWidget(run_button)
        layout.addLayout(button_layout)

        widget.setLayout(layout)
        return widget
    
    def create_table(self, rows, headers):
        table = QTableWidget()
        table.setRowCount(rows)
        table.setColumnCount(len(headers))
        table.setHorizontalHeaderLabels(headers)
        if headers == ["QR - Integer Registers"]:
            table.setVerticalHeaderLabels([f"R{i}" for i in range(16)])
            table.setRowCount(16)
        table.horizontalHeader().setSectionResizeMode(QHeaderView.ResizeMode.Stretch)
        table.verticalHeader().setSectionResizeMode(QHeaderView.ResizeMode.Stretch)
        table.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Expanding)
        return table
    
    def handle_instruction_error(self, result):
        if "error" in result:
            QMessageBox.information(self, "Error", result["error"])
            return
    
    def load_instructions(self):
        file_path, _ = QFileDialog.getOpenFileName(self, "Select Instruction File", "", "Text Files (*.txt)")
        if file_path:
            with open(file_path, "r") as file:
                instructions = [line.strip() for line in file.readlines()]
            self.thread = load_instructionsThread(self.api_url, instructions)
            self.thread.finished.connect(self.handle_instruction_result)
            self.thread.error.connect(self.handle_instruction_error)
            self.thread.start()
    
    def execute_instructions(self):
        cache_enabled = self.cache_enabled.isChecked()
        cache_type = self.cache_type.currentText()
        print(f"[UI] Executing with cache enabled: {cache_enabled}, type: {cache_type}")
        self.execute_thread = execute_instructionsThread(self.api_url)
        self.execute_thread.finished.connect(self.handle_execution_result)
        self.execute_thread.error.connect(self.handle_instruction_error)
        self.execute_thread.start()
    
    def handle_execution_result(self, result):
        if "error" in result:
            QMessageBox.information(self, "Error", result["error"])
            return
        registers = result.get("registers", [])
        if len(registers) > self.register_table.rowCount():
            self.register_table.setRowCount(len(registers))
        for reg, val in registers:
            binary_val = format(val & 0xFFFF, "016b")
            item = QTableWidgetItem(binary_val)
            self.register_table.setItem(reg, 0, item)
        memory = result.get("memory", [])
        for addr, val in memory:
            if addr >= self.memory_table.rowCount():
                self.memory_table.setRowCount(addr + 1)
            binary_val = format(val & 0xFFFF, "016b")
            self.memory_table.setItem(addr, 0, QTableWidgetItem(binary_val))
        cache = result.get("cache", [])
        cache_data = result.get("cache_data", [])
        cache_entries = {}
        for index, offset, valid, tag in cache:
            key = f"{index}:{offset}"
            cache_entries[key] = {"index": index, "offset": offset, "valid": valid, "tag": tag, "data": [0,0,0,0]}
        for index, offset, data_idx, data_val in cache_data:
            key = f"{index}:{offset}"
            if key in cache_entries and 0 <= data_idx < 4:
                cache_entries[key]["data"][data_idx] = data_val
        self.cache_table.setRowCount(len(cache_entries))
        for i, entry in enumerate(cache_entries.values()):
            self.cache_table.setItem(i, 0, QTableWidgetItem(str(entry["index"])))
            self.cache_table.setItem(i, 1, QTableWidgetItem(str(entry["offset"])))
            self.cache_table.setItem(i, 2, QTableWidgetItem("Valid" if entry["valid"] else "Invalid"))
            self.cache_table.setItem(i, 3, QTableWidgetItem(str(entry["tag"])))
            for j in range(4):
                self.cache_table.setItem(i, 4+j, QTableWidgetItem(str(entry["data"][j])))
        self.register_table.update()
        self.memory_table.update()
        self.cache_table.update()
        self.update()
        if "message" in result:
            QMessageBox.information(self, "Execution Complete", result["message"])
        else:
            QMessageBox.information(self, "Execution Complete", "Execution finished without a message.")
    
    def handle_instruction_result(self, result):
        if "error" in result:
            QMessageBox.information(self, "Error", result["error"])
            return
        binary = result.get("binary", [])
        raw = result.get("raw", [])
        memory = result.get("memory", [])
        self.instruction_table.setRowCount(len(raw))
        for i, (asm, bin_val) in enumerate(zip(raw, binary)):
            self.instruction_table.setItem(i, 0, QTableWidgetItem(str(i)))
            self.instruction_table.setItem(i, 1, QTableWidgetItem(asm))
            self.instruction_table.setItem(i, 2, QTableWidgetItem(bin_val))
        address_labels = [str(i) for i in range(1000)]
        self.memory_table.setVerticalHeaderLabels(address_labels)
        for i in range(1000):
            self.memory_table.setItem(i, 0, QTableWidgetItem("0000000000000000"))
        for addr, val in memory:
            if addr < 1000:
                binary_val = format(val & 0xFFFF, "016b")
                self.memory_table.setItem(addr, 0, QTableWidgetItem(binary_val))
        QMessageBox.information(self, "file loaded", result["message"])
    
    def step_instruction(self):
        self.step_thread = step_instructionThread(self.api_url)
        self.step_thread.finished.connect(self.handle_step_result)
        self.step_thread.error.connect(self.handle_instruction_error)
        self.step_thread.start()
    
    def handle_step_result(self, result):
        """Handle the result from a step instruction request"""
        # Check for errors
        if "error" in result:
            QMessageBox.information(self, "Error", result["error"])
            return

        # Update pipeline visualization
        pipeline_state = result.get("pipeline", [])
        stage_to_row = {
            "FETCH": 0,
            "DECODE": 1,
            "EXECUTE": 2,
            "MEMORY": 3,
            "WRITEBACK": 4
        }
        updated = {i: False for i in range(5)}

        for stage, instr, pc in pipeline_state:
            row = stage_to_row.get(stage)
            if row is None:
                continue
            updated[row] = True
            is_nop = instr.lower() in ("nop", "bubble")
            color = QColor(40, 15, 15) if is_nop else QColor(15, 40, 15)

            # Status cell
            status_item = QTableWidgetItem("Bubble" if is_nop else "Valid")
            status_item.setBackground(color)
            status_item.setForeground(QColor(180, 180, 180))
            self.pipeline_table.setItem(row, 1, status_item)

            # Instruction cell
            instr_item = QTableWidgetItem("-" if is_nop else instr)
            instr_item.setBackground(color.darker())
            instr_item.setForeground(QColor(180, 180, 180))
            self.pipeline_table.setItem(row, 2, instr_item)

            # PC cell
            pc_item = QTableWidgetItem("-" if is_nop else str(pc))
            pc_item.setBackground(color.darker())
            pc_item.setForeground(QColor(180, 180, 180))
            self.pipeline_table.setItem(row, 3, pc_item)

        # Fill in any untouched rows as bubbles
        for r, seen in updated.items():
            if not seen:
                status = QTableWidgetItem("Bubble")
                status.setBackground(QColor(40, 15, 15))
                status.setForeground(QColor(180, 180, 180))
                self.pipeline_table.setItem(r, 1, status)

                dash = QTableWidgetItem("-")
                dash.setBackground(QColor(35, 10, 10))
                dash.setForeground(QColor(180, 180, 180))
                self.pipeline_table.setItem(r, 2, dash)
                self.pipeline_table.setItem(r, 3, dash.clone())

        # Update cycle count display and window title
        cycle = result.get("cycle", 0)
        self.cycle_display.setText(f"Cycle: {cycle}")
        self.setWindowTitle(f"ARCH-16: Instruction Set Architecture - Cycle {cycle}")

        for reg, val in result.get("registers", []):
            if reg == 15:
                self.pc_display.setText(f"PC: {val}")
                break

        branch = result.get("branch_taken", False)

        # And finally, update registers/memory/cache
        self.handle_execution_result(result)

    
    def set_configuration(self):
        payload = {
            "cache_enabled": self.cache_enabled.isChecked(),
            "pipeline_enabled": self.pipeline_enabled.isChecked(),
            "dram_delay": self.dram_delay.value(),
            "cache_delay": self.cache_delay.value(),
        }
        try:
            r = requests.post(f"{self.api_url}/set_configuration", json=payload, timeout=5)
            r.raise_for_status()
            QMessageBox.information(self, "Config applied", r.json().get("message","OK"))
        except Exception as e:
            QMessageBox.information(self, "Error", f"set-configuration failed: {e}")
    
    def reset_simulator(self):
        self.reset_thread = resetThread(self.api_url)
        self.reset_thread.finished.connect(self.handle_reset_result)
        self.reset_thread.error.connect(self.handle_instruction_error)
        self.reset_thread.start()
    
    def handle_reset_result(self, result):
        """Handle the result from a reset request"""
        print("[DEBUG] handle_reset_result called with result:", result)

        # Reset cycle count display and window title
        self.cycle_display.setText("Cycle: 0")
        self.setWindowTitle("ARCH-16: Instruction Set Architecture")

        # Reset PC and branch displays
        self.pc_display.setText("PC: 0")

        # Clear the instruction table
        self.clear_table(self.instruction_table)

        # Clear and reset the pipeline table to all “Bubble” rows
        for row in range(self.pipeline_table.rowCount()):
            # Status column → “Bubble”
            status = QTableWidgetItem("Bubble")
            status.setBackground(QColor(40, 15, 15))
            status.setForeground(QColor(180, 180, 180))
            self.pipeline_table.setItem(row, 1, status)

            # Instruction column → “-”
            instr = QTableWidgetItem("-")
            instr.setBackground(QColor(35, 10, 10))
            instr.setForeground(QColor(180, 180, 180))
            self.pipeline_table.setItem(row, 2, instr)

            # PC column → “-”
            pc = QTableWidgetItem("-")
            pc.setBackground(QColor(35, 10, 10))
            pc.setForeground(QColor(180, 180, 180))
            self.pipeline_table.setItem(row, 3, pc)

        # Finally, repopulate registers, memory and cache with the reset state
        self.handle_execution_result(result)


    
    def clear_table(self, table):
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