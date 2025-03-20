import sys
import os
import time
import subprocess
import requests
from PyQt6.QtWidgets import (
    QApplication, QWidget, QVBoxLayout, QHBoxLayout, QLabel, QPushButton, QSizePolicy, QSpacerItem,
    QTableWidget, QTabWidget, QSpinBox, QCheckBox, QComboBox, QGridLayout, QLineEdit, QHeaderView,
    QFileDialog, QMessageBox
)
from PyQt6.QtCore import Qt
from PyQt6.QtGui import QPalette, QColor
from watchdog.observers import Observer
from watchdog.events import FileSystemEventHandler

SCRIPT_PATH = os.path.abspath(__file__)

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
        self.memory_table = self.create_table(50000, ["Value"])
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
        self.pipeline_table = self.create_table(5, ["Cycle", "Stage", "Instruction"])
        
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

        # Enable resizing for columns and rows
        table.horizontalHeader().setSectionResizeMode(QHeaderView.ResizeMode.Stretch)  # Columns stretch
        table.verticalHeader().setSectionResizeMode(QHeaderView.ResizeMode.Stretch)    # Rows stretch

        table.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Expanding)  # Make table expand fully
        return table


    # Functions for UI functionality.
    def load_instructions(self):
        file_dialog = QFileDialog
        file_path, _ = file_dialog.getOpenFileName(self, "Select Instruction File", "", "Text Files (*.txt)")

        if file_path:  # If a file was selected
            with open(file_path, "r") as file:
                file_contents = file.read()
            
            response = requests.post(f"{self.api_url}/load_instructions", json={"file_contents": file_contents})
            QMessageBox.information(self, "File Loaded", response.json().get("message"))


if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "--watch":
        start_application()
    else:
        app = QApplication(sys.argv)
        window = ISASimulatorUI()
        window.show()
        sys.exit(app.exec())
