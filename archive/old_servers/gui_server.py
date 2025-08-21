#!/usr/bin/env python3
"""
Prop Control GUI Server
GUI version of the standalone server with visual interface
Version 0.1 - "Mission Control"
"""

import tkinter as tk
from tkinter import ttk, scrolledtext, messagebox
import threading
import queue
import time
import json
from datetime import datetime
from typing import Dict, List, Optional
import sys
import os
from pathlib import Path

# Add server directory to path
try:
    sys.path.insert(0, str(Path(__file__).parent))
except NameError:
    # Handle case when __file__ is not defined (e.g., when run with exec)
    sys.path.insert(0, os.path.join(os.getcwd(), 'server'))

try:
    from standalone_server import TricorderStandaloneServer, CONFIG
except ImportError as e:
    messagebox.showerror("Import Error", f"Failed to import standalone server: {e}")
    sys.exit(1)

class TricorderGUIServer:
    def __init__(self):
        # Disable signal handlers when using server as GUI backend
        self.server = TricorderStandaloneServer(enable_signal_handlers=False)
        self.root = tk.Tk()
        self.server_thread = None
        self.running = False
        self.message_queue = queue.Queue()
        
        # GUI Variables
        self.device_vars = {}
        self.selected_device = tk.StringVar()
        self.command_action = tk.StringVar(value="ping")
        self.auto_refresh = tk.BooleanVar(value=True)
        
        self.setup_gui()
        self.setup_server_logging()
        
        # Start GUI update loop
        self.root.after(100, self.process_messages)
        
    def setup_gui(self):
        """Setup the main GUI with modern UniFi-inspired styling"""
        self.root.title("Prop Control System - Mission Control v0.1")
        self.root.geometry("1400x900")
        self.root.configure(bg='#0f172a')  # Modern dark slate
        
        # Modern color palette inspired by UniFi Network
        self.colors = {
            'bg_primary': '#0f172a',      # Dark slate (main background)
            'bg_secondary': '#1e293b',    # Medium slate (panels)
            'bg_tertiary': '#334155',     # Light slate (cards)
            'bg_surface': '#475569',      # Surface elements
            'accent_blue': '#0ea5e9',     # Primary blue (UniFi style)
            'accent_green': '#10b981',    # Success green
            'accent_orange': '#f59e0b',   # Warning orange  
            'accent_red': '#ef4444',      # Error red
            'accent_purple': '#8b5cf6',   # Purple accent
            'text_primary': '#f8fafc',    # Pure white text
            'text_secondary': '#cbd5e1',  # Muted text
            'text_muted': '#94a3b8',      # Very muted text
            'border': '#475569',          # Border color
            'border_light': '#64748b'     # Lighter border
        }
        
        # Configure modern ttk styles
        style = ttk.Style()
        style.theme_use('clam')
        
        # Modern frame styles
        style.configure('Modern.TFrame', 
                       background=self.colors['bg_primary'],
                       relief='flat',
                       borderwidth=0)
        
        style.configure('Card.TFrame', 
                       background=self.colors['bg_secondary'],
                       relief='solid',
                       borderwidth=1,
                       bordercolor=self.colors['border'])
        
        style.configure('Panel.TFrame', 
                       background=self.colors['bg_tertiary'],
                       relief='flat',
                       borderwidth=0)
        
        # Modern label styles
        style.configure('Modern.TLabel', 
                       background=self.colors['bg_primary'], 
                       foreground=self.colors['text_primary'],
                       font=('Segoe UI', 10))
        
        style.configure('Header.TLabel', 
                       background=self.colors['bg_primary'], 
                       foreground=self.colors['text_primary'],
                       font=('Segoe UI', 18, 'bold'))
        
        style.configure('Title.TLabel', 
                       background=self.colors['bg_secondary'], 
                       foreground=self.colors['text_primary'],
                       font=('Segoe UI', 14, 'bold'))
        
        style.configure('Subtitle.TLabel', 
                       background=self.colors['bg_primary'], 
                       foreground=self.colors['text_secondary'],
                       font=('Segoe UI', 9))
        
        style.configure('Muted.TLabel', 
                       background=self.colors['bg_secondary'], 
                       foreground=self.colors['text_muted'],
                       font=('Segoe UI', 9))
        
        # Modern button styles
        style.configure('Modern.TButton', 
                       background=self.colors['accent_blue'],
                       foreground='white',
                       font=('Segoe UI', 10, 'bold'),
                       relief='flat',
                       borderwidth=0,
                       padding=(25, 10))
        
        style.map('Modern.TButton',
                 background=[('active', '#0284c7'), ('pressed', '#0369a1')])
        
        style.configure('Success.TButton', 
                       background=self.colors['accent_green'],
                       foreground='white',
                       font=('Segoe UI', 9, 'bold'),
                       relief='flat',
                       borderwidth=0,
                       padding=(20, 8))
        
        style.map('Success.TButton',
                 background=[('active', '#059669'), ('pressed', '#047857')])
        
        style.configure('Warning.TButton', 
                       background=self.colors['accent_orange'],
                       foreground='white',
                       font=('Segoe UI', 9, 'bold'),
                       relief='flat',
                       borderwidth=0,
                       padding=(20, 8))
        
        style.configure('Danger.TButton', 
                       background=self.colors['accent_red'],
                       foreground='white',
                       font=('Segoe UI', 9, 'bold'),
                       relief='flat',
                       borderwidth=0,
                       padding=(20, 8))
        
        # LED Color buttons (circular style)
        style.configure('LED.TButton',
                       font=('Segoe UI', 9, 'bold'),
                       relief='flat',
                       borderwidth=2,
                       padding=(15, 8))
        
        # Modern notebook (tabs)
        style.configure('Modern.TNotebook', 
                       background=self.colors['bg_primary'],
                       borderwidth=0,
                       tabposition='n')
        
        style.configure('Modern.TNotebook.Tab', 
                       background=self.colors['bg_secondary'],
                       foreground=self.colors['text_secondary'],
                       font=('Segoe UI', 11),
                       padding=(30, 15),
                       borderwidth=0)
        
        style.map('Modern.TNotebook.Tab',
                 background=[('selected', self.colors['bg_tertiary']), 
                           ('active', self.colors['bg_surface'])],
                 foreground=[('selected', self.colors['text_primary']),
                           ('active', self.colors['text_primary'])])
        
        # Modern treeview
        style.configure('Modern.Treeview',
                       background=self.colors['bg_secondary'],
                       foreground=self.colors['text_primary'],
                       fieldbackground=self.colors['bg_secondary'],
                       font=('Segoe UI', 10),
                       borderwidth=0,
                       relief='flat',
                       rowheight=30)
        
        style.configure('Modern.Treeview.Heading',
                       background=self.colors['bg_tertiary'],
                       foreground=self.colors['text_primary'],
                       font=('Segoe UI', 11, 'bold'),
                       relief='flat',
                       borderwidth=1,
                       bordercolor=self.colors['border'])
        
        style.map('Modern.Treeview',
                 background=[('selected', self.colors['accent_blue'])],
                 foreground=[('selected', 'white')])
        
        self.create_menu()
        self.create_main_layout()
        
    def create_menu(self):
        """Create modern menu bar"""
        menubar = tk.Menu(self.root, 
                         bg=self.colors['bg_secondary'],
                         fg=self.colors['text_primary'],
                         activebackground=self.colors['accent_blue'],
                         activeforeground='white',
                         font=('Segoe UI', 10))
        self.root.config(menu=menubar)
        
        # Server menu
        server_menu = tk.Menu(menubar, tearoff=0,
                             bg=self.colors['bg_secondary'],
                             fg=self.colors['text_primary'],
                             activebackground=self.colors['accent_blue'],
                             activeforeground='white',
                             font=('Segoe UI', 10))
        menubar.add_cascade(label="Server", menu=server_menu)
        server_menu.add_command(label=">> Start Server", command=self.start_server)
        server_menu.add_command(label="|| Stop Server", command=self.stop_server)
        server_menu.add_separator()
        server_menu.add_command(label="<> Discover Devices", command=self.discover_devices)
        server_menu.add_separator()
        server_menu.add_command(label="X Exit", command=self.on_closing)
        
        # View menu
        view_menu = tk.Menu(menubar, tearoff=0,
                           bg=self.colors['bg_secondary'],
                           fg=self.colors['text_primary'],
                           activebackground=self.colors['accent_blue'],
                           activeforeground='white',
                           font=('Segoe UI', 10))
        menubar.add_cascade(label="View", menu=view_menu)
        view_menu.add_checkbutton(label="[R] Auto Refresh", variable=self.auto_refresh)
        view_menu.add_command(label="[x] Clear Log", command=self.clear_log)
        view_menu.add_command(label="[R] Refresh Devices", command=self.refresh_devices)
        
        # Help menu
        help_menu = tk.Menu(menubar, tearoff=0,
                           bg=self.colors['bg_secondary'],
                           fg=self.colors['text_primary'],
                           activebackground=self.colors['accent_blue'],
                           activeforeground='white',
                           font=('Segoe UI', 10))
        menubar.add_cascade(label="Help", menu=help_menu)
        help_menu.add_command(label="(i) About", command=self.show_about)
        
    def create_main_layout(self):
        """Create modern main layout with UniFi-inspired design"""
        # Main container with modern styling
        main_frame = ttk.Frame(self.root, style='Modern.TFrame')
        main_frame.pack(fill=tk.BOTH, expand=True, padx=20, pady=20)
        
        # Header section
        self.create_modern_header(main_frame)
        
        # Main content area with cards
        content_frame = ttk.Frame(main_frame, style='Modern.TFrame')
        content_frame.pack(fill=tk.BOTH, expand=True, pady=(20, 0))
        
        # Server status card (top)
        self.create_server_status_card(content_frame)
        
        # Main content notebook with modern tabs
        self.create_modern_notebook(content_frame)
        
    def create_modern_header(self, parent):
        """Create modern header with title and quick stats"""
        header_frame = ttk.Frame(parent, style='Modern.TFrame')
        header_frame.pack(fill=tk.X, pady=(0, 20))
        
        # Left side - Title and subtitle
        title_frame = ttk.Frame(header_frame, style='Modern.TFrame')
        title_frame.pack(side=tk.LEFT, fill=tk.X, expand=True)
        
        title_label = ttk.Label(title_frame, 
                               text="Prop Control System", 
                               style='Header.TLabel')
        title_label.pack(anchor=tk.W)
        
        subtitle_label = ttk.Label(title_frame, 
                                  text="Mission Control v0.1 - Device Management Dashboard", 
                                  style='Subtitle.TLabel')
        subtitle_label.pack(anchor=tk.W)
        
        # Right side - Quick stats
        stats_frame = ttk.Frame(header_frame, style='Card.TFrame')
        stats_frame.pack(side=tk.RIGHT, padx=(20, 0))
        
        self.quick_stats_frame = stats_frame
        self.update_quick_stats()
        
    def create_server_status_card(self, parent):
        """Create modern server status card"""
        # Status card container
        status_card = ttk.Frame(parent, style='Card.TFrame')
        status_card.pack(fill=tk.X, pady=(0, 20))
        
        # Card header
        header_frame = ttk.Frame(status_card, style='Card.TFrame')
        header_frame.pack(fill=tk.X, padx=20, pady=(15, 10))
        
        ttk.Label(header_frame, text="Server Status", style='Title.TLabel').pack(side=tk.LEFT)
        
        # Status indicators frame
        indicators_frame = ttk.Frame(header_frame, style='Card.TFrame')
        indicators_frame.pack(side=tk.RIGHT)
        
        # Server status indicator
        self.status_indicator = ttk.Label(indicators_frame, 
                                         text="[*] Stopped", 
                                         foreground=self.colors['accent_red'],
                                         style='Modern.TLabel')
        self.status_indicator.pack(side=tk.RIGHT, padx=(10, 0))
        
        # Card content
        content_frame = ttk.Frame(status_card, style='Card.TFrame')
        content_frame.pack(fill=tk.X, padx=20, pady=(0, 15))
        
        # Left side - Server info
        info_frame = ttk.Frame(content_frame, style='Card.TFrame')
        info_frame.pack(side=tk.LEFT, fill=tk.X, expand=True)
        
        self.server_info_frame = info_frame
        
        # Right side - Control buttons
        controls_frame = ttk.Frame(content_frame, style='Card.TFrame')
        controls_frame.pack(side=tk.RIGHT, padx=(20, 0))
        
        self.start_button = ttk.Button(controls_frame, 
                                      text="Start Server", 
                                      style='Modern.TButton',
                                      command=self.start_server)
        self.start_button.pack(side=tk.LEFT, padx=(0, 10))
        
        self.discover_button = ttk.Button(controls_frame, 
                                         text="Discover Devices", 
                                         style='Success.TButton',
                                         command=self.discover_devices)
        self.discover_button.pack(side=tk.LEFT)
        
        self.update_server_status_display()
        
    def create_modern_notebook(self, parent):
        """Create modern tabbed interface"""
        # Notebook with modern styling
        self.notebook = ttk.Notebook(parent, style='Modern.TNotebook')
        self.notebook.pack(fill=tk.BOTH, expand=True)
        
        # Devices tab
        devices_frame = ttk.Frame(self.notebook, style='Modern.TFrame')
        self.notebook.add(devices_frame, text="[PC] Devices")
        self.create_modern_devices_panel(devices_frame)
        
        # Commands tab
        commands_frame = ttk.Frame(self.notebook, style='Modern.TFrame')
        self.notebook.add(commands_frame, text="[*] Commands")
        self.create_modern_commands_panel(commands_frame)
        
        # Server Log tab
        log_frame = ttk.Frame(self.notebook, style='Modern.TFrame')
        self.notebook.add(log_frame, text="[#] Server Log")
        self.create_modern_log_panel(log_frame)
        
        # Statistics tab
        stats_frame = ttk.Frame(self.notebook, style='Modern.TFrame')
        self.notebook.add(stats_frame, text="[%] Statistics")
        self.create_modern_stats_panel(stats_frame)
        
    def update_quick_stats(self):
        """Update quick stats in header"""
        if hasattr(self, 'quick_stats_frame'):
            # Clear existing stats
            for widget in self.quick_stats_frame.winfo_children():
                widget.destroy()
            
            # Create stats display
            stats_container = ttk.Frame(self.quick_stats_frame, style='Card.TFrame')
            stats_container.pack(padx=15, pady=10)
            
            # Device count
            device_count = len(self.server.devices)
            ttk.Label(stats_container, 
                     text=f"{device_count}", 
                     font=('Segoe UI', 16, 'bold'),
                     foreground=self.colors['accent_blue'],
                     style='Modern.TLabel').pack()
            ttk.Label(stats_container, 
                     text="Devices", 
                     style='Muted.TLabel').pack()
    
    def update_server_status_display(self):
        """Update server status display"""
        if hasattr(self, 'status_indicator'):
            if self.running:
                self.status_indicator.configure(
                    text="[*] Running", 
                    foreground=self.colors['accent_green'])
                self.start_button.configure(text="Stop Server", style='Danger.TButton')
            else:
                self.status_indicator.configure(
                    text="[*] Stopped", 
                    foreground=self.colors['accent_red'])
                self.start_button.configure(text="Start Server", style='Modern.TButton')
        
        # Update server info
        if hasattr(self, 'server_info_frame'):
            for widget in self.server_info_frame.winfo_children():
                widget.destroy()
            
            if self.running:
                uptime = time.time() - self.server.server_start_time
                hours = int(uptime // 3600)
                minutes = int((uptime % 3600) // 60)
                
                info_text = f"Port: 8888 | Uptime: {hours:02d}:{minutes:02d} | Devices: {len(self.server.devices)}"
            else:
                info_text = "Server stopped - Click 'Start Server' to begin"
            
            ttk.Label(self.server_info_frame, 
                     text=info_text, 
                     style='Muted.TLabel').pack(anchor=tk.W)
    
    def create_modern_devices_panel(self, parent):
        """Create modern devices management panel"""
        # Panel with padding
        panel_frame = ttk.Frame(parent, style='Modern.TFrame')
        panel_frame.pack(fill=tk.BOTH, expand=True, padx=20, pady=20)
        
        # Header with title and controls
        header_frame = ttk.Frame(panel_frame, style='Modern.TFrame')
        header_frame.pack(fill=tk.X, pady=(0, 20))
        
        ttk.Label(header_frame, text="Device Management", style='Title.TLabel').pack(side=tk.LEFT)
        
        # Refresh button
        refresh_btn = ttk.Button(header_frame, 
                                text="[R] Refresh", 
                                style='Success.TButton',
                                command=self.refresh_devices)
        refresh_btn.pack(side=tk.RIGHT)
        
        # Main content area
        content_frame = ttk.Frame(panel_frame, style='Card.TFrame')
        content_frame.pack(fill=tk.BOTH, expand=True)
        
        # Devices table container
        table_frame = ttk.Frame(content_frame, style='Card.TFrame')
        table_frame.pack(fill=tk.BOTH, expand=True, padx=20, pady=20)
        
        # Create modern treeview for devices
        columns = ('ID', 'IP Address', 'Firmware', 'Status', 'Last Seen')
        self.device_tree = ttk.Treeview(table_frame, 
                                       columns=columns, 
                                       show='headings',
                                       style='Modern.Treeview',
                                       height=12)
        
        # Configure columns with modern styling
        self.device_tree.heading('ID', text='Device ID', anchor=tk.W)
        self.device_tree.heading('IP Address', text='IP Address', anchor=tk.W)
        self.device_tree.heading('Firmware', text='Firmware', anchor=tk.W)
        self.device_tree.heading('Status', text='Status', anchor=tk.W)
        self.device_tree.heading('Last Seen', text='Last Seen', anchor=tk.W)
        
        # Column widths
        self.device_tree.column('ID', width=150)
        self.device_tree.column('IP Address', width=120)
        self.device_tree.column('Firmware', width=80)
        self.device_tree.column('Status', width=80)
        self.device_tree.column('Last Seen', width=120)
        
        # Scrollbar for table
        scrollbar = ttk.Scrollbar(table_frame, orient=tk.VERTICAL, command=self.device_tree.yview)
        self.device_tree.configure(yscrollcommand=scrollbar.set)
        
        # Pack table and scrollbar
        self.device_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        
        # Device details section
        details_frame = ttk.Frame(content_frame, style='Card.TFrame')
        details_frame.pack(fill=tk.X, padx=20, pady=(0, 20))
        
        ttk.Label(details_frame, text="Device Details", style='Title.TLabel').pack(anchor=tk.W, pady=(0, 10))
        
        # Details text area with modern styling
        self.device_details_text = scrolledtext.ScrolledText(
            details_frame,
            height=8,
            bg=self.colors['bg_secondary'],
            fg=self.colors['text_primary'],
            font=('Consolas', 9),
            insertbackground=self.colors['text_primary'],
            selectbackground=self.colors['accent_blue'],
            wrap=tk.WORD,
            relief='flat',
            borderwidth=0
        )
        self.device_details_text.pack(fill=tk.X)
        
        # Bind selection event
        self.device_tree.bind('<<TreeviewSelect>>', self.on_device_select)
        
    def create_server_panel(self, parent):
        """Create server status and control panel"""
        panel = ttk.LabelFrame(parent, text="Server Status", padding=10)
        panel.pack(fill=tk.X, pady=(0, 5))
        
        # Server info
        info_frame = ttk.Frame(panel)
        info_frame.pack(fill=tk.X)
        
        self.status_label = ttk.Label(info_frame, text="Status: Stopped", font=('Arial', 10, 'bold'))
        self.status_label.pack(side=tk.LEFT)
        
        self.uptime_label = ttk.Label(info_frame, text="Uptime: --")
        self.uptime_label.pack(side=tk.LEFT, padx=(20, 0))
        
        self.devices_count_label = ttk.Label(info_frame, text="Devices: 0")
        self.devices_count_label.pack(side=tk.LEFT, padx=(20, 0))
        
        # Control buttons
        button_frame = ttk.Frame(panel)
        button_frame.pack(fill=tk.X, pady=(10, 0))
        
        self.start_button = ttk.Button(button_frame, text="Start Server", command=self.start_server)
        self.start_button.pack(side=tk.LEFT)
        
        self.stop_button = ttk.Button(button_frame, text="Stop Server", command=self.stop_server, state=tk.DISABLED)
        self.stop_button.pack(side=tk.LEFT, padx=(5, 0))
        
        ttk.Button(button_frame, text="Discover Devices", command=self.discover_devices).pack(side=tk.LEFT, padx=(5, 0))
        
        ttk.Button(button_frame, text="Refresh", command=self.refresh_devices).pack(side=tk.LEFT, padx=(5, 0))
        
    def create_devices_panel(self, parent):
        """Create devices management panel"""
        # Device list
        list_frame = ttk.LabelFrame(parent, text="Discovered Devices", padding=10)
        list_frame.pack(fill=tk.BOTH, expand=True)
        
        # Treeview for devices
        columns = ('Device ID', 'IP Address', 'Firmware', 'Status', 'Last Seen')
        self.device_tree = ttk.Treeview(list_frame, columns=columns, show='headings', height=10)
        
        for col in columns:
            self.device_tree.heading(col, text=col)
            self.device_tree.column(col, width=150)
        
        # Scrollbars
        v_scrollbar = ttk.Scrollbar(list_frame, orient=tk.VERTICAL, command=self.device_tree.yview)
        self.device_tree.configure(yscrollcommand=v_scrollbar.set)
        
        h_scrollbar = ttk.Scrollbar(list_frame, orient=tk.HORIZONTAL, command=self.device_tree.xview)
        self.device_tree.configure(xscrollcommand=h_scrollbar.set)
        
        # Pack treeview and scrollbars
        self.device_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        v_scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        h_scrollbar.pack(side=tk.BOTTOM, fill=tk.X)
        
        # Device details
        details_frame = ttk.LabelFrame(parent, text="Device Details", padding=10)
        details_frame.pack(fill=tk.X, pady=(5, 0))
        
        self.device_details = scrolledtext.ScrolledText(details_frame, height=8, wrap=tk.WORD)
        self.device_details.pack(fill=tk.BOTH, expand=True)
        
        # Bind selection event
        self.device_tree.bind('<<TreeviewSelect>>', self.on_device_select)
        
    def create_commands_panel(self, parent):
        """Create command control panel"""
        # Quick commands
        quick_frame = ttk.LabelFrame(parent, text="Quick Commands", padding=10)
        quick_frame.pack(fill=tk.X)
        
        # Device selection
        device_frame = ttk.Frame(quick_frame)
        device_frame.pack(fill=tk.X, pady=(0, 10))
        
        ttk.Label(device_frame, text="Target Device:").pack(side=tk.LEFT)
        self.device_combo = ttk.Combobox(device_frame, textvariable=self.selected_device, width=20)
        self.device_combo.pack(side=tk.LEFT, padx=(5, 0))
        
        ttk.Button(device_frame, text="All Devices", command=self.select_all_devices).pack(side=tk.LEFT, padx=(5, 0))
        
        # Quick action buttons
        action_frame = ttk.Frame(quick_frame)
        action_frame.pack(fill=tk.X, pady=(0, 10))
        
        ttk.Button(action_frame, text="Ping", command=lambda: self.send_quick_command("ping")).pack(side=tk.LEFT)
        ttk.Button(action_frame, text="Status", command=lambda: self.send_quick_command("status")).pack(side=tk.LEFT, padx=(5, 0))
        ttk.Button(action_frame, text="Boot Screen", command=lambda: self.send_quick_command("display_boot_screen")).pack(side=tk.LEFT, padx=(5, 0))
        ttk.Button(action_frame, text="Stop Video", command=lambda: self.send_quick_command("stop_video")).pack(side=tk.LEFT, padx=(5, 0))
        
        # LED Controls
        led_frame = ttk.LabelFrame(parent, text="LED Controls", padding=10)
        led_frame.pack(fill=tk.X, pady=(5, 0))
        
        # Color buttons
        color_frame = ttk.Frame(led_frame)
        color_frame.pack(fill=tk.X)
        
        colors = [
            ("Red", "#ff0000", (255, 0, 0)),
            ("Green", "#00ff00", (0, 255, 0)),
            ("Blue", "#0000ff", (0, 0, 255)),
            ("Yellow", "#ffff00", (255, 255, 0)),
            ("Cyan", "#00ffff", (0, 255, 255)),
            ("Magenta", "#ff00ff", (255, 0, 255)),
            ("White", "#ffffff", (255, 255, 255)),
            ("Off", "#000000", (0, 0, 0))
        ]
        
        for name, color, rgb in colors:
            btn = tk.Button(color_frame, text=name, bg=color, 
                          fg="white" if sum(rgb) < 400 else "black",
                          command=lambda r=rgb: self.set_led_color(r))
            btn.pack(side=tk.LEFT, padx=2)
        
        # Brightness control
        brightness_frame = ttk.Frame(led_frame)
        brightness_frame.pack(fill=tk.X, pady=(10, 0))
        
        ttk.Label(brightness_frame, text="Brightness:").pack(side=tk.LEFT)
        self.brightness_var = tk.IntVar(value=128)
        brightness_scale = ttk.Scale(brightness_frame, from_=0, to=255, 
                                   variable=self.brightness_var, orient=tk.HORIZONTAL)
        brightness_scale.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(5, 5))
        
        ttk.Button(brightness_frame, text="Set", 
                  command=self.set_led_brightness).pack(side=tk.LEFT)
        
        # Custom command
        custom_frame = ttk.LabelFrame(parent, text="Custom Command", padding=10)
        custom_frame.pack(fill=tk.BOTH, expand=True, pady=(5, 0))
        
        # Action entry
        action_entry_frame = ttk.Frame(custom_frame)
        action_entry_frame.pack(fill=tk.X, pady=(0, 5))
        
        ttk.Label(action_entry_frame, text="Action:").pack(side=tk.LEFT)
        self.action_entry = ttk.Entry(action_entry_frame, width=20)
        self.action_entry.pack(side=tk.LEFT, padx=(5, 0))
        
        # Parameters entry
        params_frame = ttk.Frame(custom_frame)
        params_frame.pack(fill=tk.BOTH, expand=True)
        
        ttk.Label(params_frame, text="Parameters (JSON):").pack(anchor=tk.W)
        self.params_text = scrolledtext.ScrolledText(params_frame, height=4)
        self.params_text.pack(fill=tk.BOTH, expand=True, pady=(5, 5))
        self.params_text.insert(tk.END, '{}')
        
        ttk.Button(custom_frame, text="Send Custom Command", 
                  command=self.send_custom_command).pack()
        
    def create_log_panel(self, parent):
        """Create server log panel"""
        log_frame = ttk.Frame(parent)
        log_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Log display
        self.log_text = scrolledtext.ScrolledText(log_frame, wrap=tk.WORD, 
                                                height=20, font=('Consolas', 9))
        self.log_text.pack(fill=tk.BOTH, expand=True)
        
        # Configure text tags for different log levels
        self.log_text.tag_configure("INFO", foreground="black")
        self.log_text.tag_configure("ERROR", foreground="red")
        self.log_text.tag_configure("WARN", foreground="orange")
        self.log_text.tag_configure("DEBUG", foreground="gray")
        
        # Control buttons
        button_frame = ttk.Frame(log_frame)
        button_frame.pack(fill=tk.X, pady=(5, 0))
        
        ttk.Button(button_frame, text="Clear Log", command=self.clear_log).pack(side=tk.LEFT)
        ttk.Button(button_frame, text="Save Log", command=self.save_log).pack(side=tk.LEFT, padx=(5, 0))
        
    def create_stats_panel(self, parent):
        """Create statistics panel"""
        stats_frame = ttk.Frame(parent)
        stats_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Stats display
        self.stats_text = scrolledtext.ScrolledText(stats_frame, wrap=tk.WORD, 
                                                  height=20, font=('Consolas', 10))
        self.stats_text.pack(fill=tk.BOTH, expand=True)
        
        # Refresh button
        ttk.Button(stats_frame, text="Refresh Statistics", 
                  command=self.refresh_stats).pack(pady=(5, 0))
        
    def setup_server_logging(self):
        """Setup custom logging for the server"""
        # Override the server's log method to send messages to GUI
        original_log = self.server.log
        
        def gui_log(message: str, level: str = "INFO"):
            original_log(message, level)
            self.message_queue.put(('log', level, message))
        
        self.server.log = gui_log
        
    def process_messages(self):
        """Process messages from the server thread"""
        try:
            while True:
                msg_type, *args = self.message_queue.get_nowait()
                
                if msg_type == 'log':
                    level, message = args
                    self.add_log_message(message, level)
                elif msg_type == 'device_update':
                    self.refresh_devices()
                elif msg_type == 'server_status':
                    status = args[0]
                    self.update_server_status(status)
                    
        except queue.Empty:
            pass
        
        # Update GUI periodically
        if self.auto_refresh.get():
            self.update_display()
        
        # Schedule next update
        self.root.after(1000, self.process_messages)
        
    def add_log_message(self, message: str, level: str = "INFO"):
        """Add message to log display"""
        self.log_text.insert(tk.END, message + '\n', level)
        self.log_text.see(tk.END)
        
        # Limit log size
        lines = int(self.log_text.index('end-1c').split('.')[0])
        if lines > 1000:
            self.log_text.delete('1.0', '100.0')
    
    def update_display(self):
        """Update display elements"""
        if self.running and hasattr(self.server, 'get_server_stats'):
            try:
                # Update server status
                stats = self.server.get_server_stats()
                
                # Update UI elements only if they exist
                if hasattr(self, 'uptime_label'):
                    self.uptime_label.config(text=f"Uptime: {stats['uptime']}")
                
                if hasattr(self, 'devices_count_label'):
                    self.devices_count_label.config(text=f"Devices: {stats['devices_online']}")
                    
            except Exception as e:
                # Silently handle missing stats method
                pass
            
    def refresh_devices(self):
        """Refresh device list"""
        # Clear existing items
        for item in self.device_tree.get_children():
            self.device_tree.delete(item)
        
        # Add current devices
        for device_id, device in self.server.devices.items():
            values = (
                device_id,
                device.get('ip_address', 'Unknown'),
                device.get('firmware_version', 'Unknown'),
                device.get('status', 'Unknown'),
                device.get('last_seen', 'Unknown')
            )
            self.device_tree.insert('', tk.END, values=values)
        
        # Update device combo
        device_list = list(self.server.devices.keys())
        self.device_combo['values'] = device_list
        if device_list and not self.selected_device.get():
            self.selected_device.set(device_list[0])
    
    def on_device_select(self, event):
        """Handle device selection"""
        selection = self.device_tree.selection()
        if selection:
            item = self.device_tree.item(selection[0])
            device_id = item['values'][0]
            
            # Show device details
            if device_id in self.server.devices:
                device = self.server.devices[device_id]
                details = json.dumps(device, indent=2)
                self.device_details.delete('1.0', tk.END)
                self.device_details.insert('1.0', details)
                
                # Select in combo
                self.selected_device.set(device_id)
    
    def create_modern_commands_panel(self, parent):
        """Create modern commands panel with LED controls"""
        # Main panel
        panel_frame = ttk.Frame(parent, style='Modern.TFrame')
        panel_frame.pack(fill=tk.BOTH, expand=True, padx=20, pady=20)
        
        # Two-column layout
        left_column = ttk.Frame(panel_frame, style='Card.TFrame')
        left_column.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=(0, 10))
        
        right_column = ttk.Frame(panel_frame, style='Card.TFrame')
        right_column.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True, padx=(10, 0))
        
        # Left column - Device selection and LED controls
        self.create_device_selection_section(left_column)
        self.create_led_controls_section(left_column)
        
        # Right column - Quick commands and custom commands
        self.create_quick_commands_section(right_column)
        self.create_custom_commands_section(right_column)
    
    def create_device_selection_section(self, parent):
        """Create device selection section"""
        # Device selection card
        selection_frame = ttk.Frame(parent, style='Card.TFrame')
        selection_frame.pack(fill=tk.X, padx=20, pady=20)
        
        ttk.Label(selection_frame, text="Target Device", style='Title.TLabel').pack(anchor=tk.W, padx=20, pady=(15, 10))
        
        # Device dropdown
        device_frame = ttk.Frame(selection_frame, style='Card.TFrame')
        device_frame.pack(fill=tk.X, padx=20, pady=(0, 15))
        
        self.selected_device = tk.StringVar(value="All Devices")
        self.device_dropdown = ttk.Combobox(device_frame, 
                                           textvariable=self.selected_device,
                                           values=["All Devices"],
                                           state="readonly",
                                           font=('Segoe UI', 10))
        self.device_dropdown.pack(fill=tk.X)
    
    def create_led_controls_section(self, parent):
        """Create LED controls section with color buttons"""
        # LED controls card
        led_frame = ttk.Frame(parent, style='Card.TFrame')
        led_frame.pack(fill=tk.X, padx=20, pady=(0, 20))
        
        ttk.Label(led_frame, text="LED Controls", style='Title.TLabel').pack(anchor=tk.W, padx=20, pady=(15, 10))
        
        # Color buttons grid
        colors_container = ttk.Frame(led_frame, style='Card.TFrame')
        colors_container.pack(fill=tk.X, padx=20, pady=(0, 15))
        
        # Define LED colors with modern styling
        led_colors = [
            ("Red", "#ef4444", (255, 0, 0)),
            ("Green", "#10b981", (0, 255, 0)), 
            ("Blue", "#3b82f6", (0, 0, 255)),
            ("Yellow", "#f59e0b", (255, 255, 0)),
            ("Cyan", "#06b6d4", (0, 255, 255)),
            ("Magenta", "#d946ef", (255, 0, 255)),
            ("White", "#f8fafc", (255, 255, 255)),
            ("Off", "#374151", (0, 0, 0))
        ]
        
        # Create color buttons in a 4x2 grid
        for i, (name, color, rgb) in enumerate(led_colors):
            row = i // 4
            col = i % 4
            
            btn = tk.Button(colors_container,
                           text=name,
                           bg=color,
                           fg='white' if name != 'White' else 'black',
                           font=('Segoe UI', 9, 'bold'),
                           relief='flat',
                           borderwidth=0,
                           pady=8,
                           command=lambda r=rgb: self.set_led_color_cmd(r))
            btn.grid(row=row, column=col, padx=5, pady=5, sticky='ew')
        
        # Configure grid weights
        for col in range(4):
            colors_container.columnconfigure(col, weight=1)
        
        # Brightness control
        brightness_frame = ttk.Frame(led_frame, style='Card.TFrame')
        brightness_frame.pack(fill=tk.X, padx=20, pady=(0, 15))
        
        ttk.Label(brightness_frame, text="Brightness:", style='Modern.TLabel').pack(side=tk.LEFT)
        
        self.brightness_var = tk.IntVar(value=128)
        brightness_scale = tk.Scale(brightness_frame,
                                   from_=0, to=255,
                                   orient=tk.HORIZONTAL,
                                   variable=self.brightness_var,
                                   bg=self.colors['bg_secondary'],
                                   fg=self.colors['text_primary'],
                                   highlightthickness=0,
                                   troughcolor=self.colors['bg_tertiary'],
                                   activebackground=self.colors['accent_blue'])
        brightness_scale.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(10, 10))
        
        brightness_btn = ttk.Button(brightness_frame,
                                   text="Set",
                                   style='Success.TButton',
                                   command=self.set_brightness_cmd)
        brightness_btn.pack(side=tk.RIGHT)
    
    def create_quick_commands_section(self, parent):
        """Create quick commands section"""
        # Quick commands card
        quick_frame = ttk.Frame(parent, style='Card.TFrame')
        quick_frame.pack(fill=tk.X, padx=20, pady=20)
        
        ttk.Label(quick_frame, text="Quick Commands", style='Title.TLabel').pack(anchor=tk.W, padx=20, pady=(15, 10))
        
        # Commands container
        commands_container = ttk.Frame(quick_frame, style='Card.TFrame')
        commands_container.pack(fill=tk.X, padx=20, pady=(0, 15))
        
        # Quick command buttons
        commands = [
            ("PING", self.send_ping, 'Success.TButton'),
            ("STATUS", self.send_status, 'Modern.TButton'),
            ("BOOT", self.send_boot_screen, 'Warning.TButton'),
            ("STOP", self.send_stop_video, 'Danger.TButton')
        ]
        
        for i, (text, command, style) in enumerate(commands):
            row = i // 2
            col = i % 2
            
            btn = ttk.Button(commands_container,
                            text=text,
                            style=style,
                            command=command)
            btn.grid(row=row, column=col, padx=5, pady=5, sticky='ew')
        
        # Configure grid weights
        commands_container.columnconfigure(0, weight=1)
        commands_container.columnconfigure(1, weight=1)
    
    def create_custom_commands_section(self, parent):
        """Create custom commands section"""
        # Custom commands card
        custom_frame = ttk.Frame(parent, style='Card.TFrame')
        custom_frame.pack(fill=tk.BOTH, expand=True, padx=20, pady=(0, 20))
        
        ttk.Label(custom_frame, text="Custom Commands", style='Title.TLabel').pack(anchor=tk.W, padx=20, pady=(15, 10))
        
        # Action input
        action_frame = ttk.Frame(custom_frame, style='Card.TFrame')
        action_frame.pack(fill=tk.X, padx=20, pady=(0, 10))
        
        ttk.Label(action_frame, text="Action:", style='Modern.TLabel').pack(side=tk.LEFT)
        
        self.custom_action_var = tk.StringVar()
        action_entry = ttk.Entry(action_frame, 
                                textvariable=self.custom_action_var,
                                font=('Segoe UI', 10))
        action_entry.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(10, 0))
        
        # Parameters input
        params_frame = ttk.Frame(custom_frame, style='Card.TFrame')
        params_frame.pack(fill=tk.BOTH, expand=True, padx=20, pady=(0, 10))
        
        ttk.Label(params_frame, text="Parameters (JSON):", style='Modern.TLabel').pack(anchor=tk.W)
        
        self.custom_params_text = scrolledtext.ScrolledText(
            params_frame,
            height=6,
            bg=self.colors['bg_secondary'],
            fg=self.colors['text_primary'],
            font=('Consolas', 9),
            insertbackground=self.colors['text_primary'],
            selectbackground=self.colors['accent_blue'],
            wrap=tk.WORD,
            relief='flat',
            borderwidth=0
        )
        self.custom_params_text.pack(fill=tk.BOTH, expand=True)
        self.custom_params_text.insert('1.0', '{}')
        
        # Send button
        send_frame = ttk.Frame(custom_frame, style='Card.TFrame')
        send_frame.pack(fill=tk.X, padx=20, pady=(0, 15))
        
        send_btn = ttk.Button(send_frame,
                             text="Send Custom Command",
                             style='Modern.TButton',
                             command=self.send_custom_command)
        send_btn.pack(anchor=tk.E)
    
    def create_modern_log_panel(self, parent):
        """Create modern log panel with filtering and search"""
        # Main panel
        panel_frame = ttk.Frame(parent, style='Modern.TFrame')
        panel_frame.pack(fill=tk.BOTH, expand=True, padx=20, pady=20)
        
        # Log controls card
        controls_frame = ttk.Frame(panel_frame, style='Card.TFrame')
        controls_frame.pack(fill=tk.X, padx=20, pady=(20, 10))
        
        ttk.Label(controls_frame, text="Server Logs", style='Title.TLabel').pack(anchor=tk.W, padx=20, pady=(15, 10))
        
        # Controls row
        controls_row = ttk.Frame(controls_frame, style='Card.TFrame')
        controls_row.pack(fill=tk.X, padx=20, pady=(0, 15))
        
        # Log level filter
        ttk.Label(controls_row, text="Level:", style='Modern.TLabel').pack(side=tk.LEFT)
        
        self.log_level_var = tk.StringVar(value="All")
        level_combo = ttk.Combobox(controls_row,
                                  textvariable=self.log_level_var,
                                  values=["All", "DEBUG", "INFO", "WARNING", "ERROR"],
                                  state="readonly",
                                  width=10,
                                  font=('Segoe UI', 9))
        level_combo.pack(side=tk.LEFT, padx=(5, 15))
        
        # Search
        ttk.Label(controls_row, text="Search:", style='Modern.TLabel').pack(side=tk.LEFT)
        
        self.log_search_var = tk.StringVar()
        search_entry = ttk.Entry(controls_row,
                                textvariable=self.log_search_var,
                                font=('Segoe UI', 9),
                                width=20)
        search_entry.pack(side=tk.LEFT, padx=(5, 15))
        
        # Control buttons
        ttk.Button(controls_row,
                  text="Clear",
                  style='Danger.TButton',
                  command=self.clear_log).pack(side=tk.RIGHT, padx=(5, 0))
        
        ttk.Button(controls_row,
                  text="Refresh",
                  style='Success.TButton',
                  command=self.refresh_log).pack(side=tk.RIGHT, padx=(5, 0))
        
        # Log display card
        log_frame = ttk.Frame(panel_frame, style='Card.TFrame')
        log_frame.pack(fill=tk.BOTH, expand=True, padx=20, pady=(0, 20))
        
        # Log text area with modern styling
        self.log_text = scrolledtext.ScrolledText(
            log_frame,
            bg=self.colors['bg_secondary'],
            fg=self.colors['text_primary'],
            font=('Consolas', 9),
            insertbackground=self.colors['text_primary'],
            selectbackground=self.colors['accent_blue'],
            wrap=tk.WORD,
            relief='flat',
            borderwidth=0,
            state=tk.DISABLED
        )
        self.log_text.pack(fill=tk.BOTH, expand=True, padx=20, pady=20)
        
        # Configure text tags for log levels
        self.log_text.tag_configure("ERROR", foreground=self.colors['accent_red'])
        self.log_text.tag_configure("WARNING", foreground=self.colors['accent_orange'])
        self.log_text.tag_configure("INFO", foreground=self.colors['accent_blue'])
        self.log_text.tag_configure("DEBUG", foreground=self.colors['text_secondary'])
    
    def create_modern_stats_panel(self, parent):
        """Create modern statistics panel"""
        # Main panel
        panel_frame = ttk.Frame(parent, style='Modern.TFrame')
        panel_frame.pack(fill=tk.BOTH, expand=True, padx=20, pady=20)
        
        # Stats overview cards
        overview_frame = ttk.Frame(panel_frame, style='Modern.TFrame')
        overview_frame.pack(fill=tk.X, padx=20, pady=20)
        
        # Create stats cards in a grid
        self.create_stats_cards(overview_frame)
        
        # Device activity chart
        activity_frame = ttk.Frame(panel_frame, style='Card.TFrame')
        activity_frame.pack(fill=tk.BOTH, expand=True, padx=20, pady=(0, 20))
        
        ttk.Label(activity_frame, text="Device Activity", style='Title.TLabel').pack(anchor=tk.W, padx=20, pady=(15, 10))
        
        # Activity chart placeholder
        chart_frame = ttk.Frame(activity_frame, style='Card.TFrame')
        chart_frame.pack(fill=tk.BOTH, expand=True, padx=20, pady=(0, 15))
        
        # For now, use a simple text display
        self.activity_text = scrolledtext.ScrolledText(
            chart_frame,
            height=10,
            bg=self.colors['bg_secondary'],
            fg=self.colors['text_primary'],
            font=('Consolas', 9),
            insertbackground=self.colors['text_primary'],
            selectbackground=self.colors['accent_blue'],
            wrap=tk.WORD,
            relief='flat',
            borderwidth=0,
            state=tk.DISABLED
        )
        self.activity_text.pack(fill=tk.BOTH, expand=True)
    
    def create_stats_cards(self, parent):
        """Create statistics overview cards"""
        # Cards container
        cards_frame = ttk.Frame(parent, style='Modern.TFrame')
        cards_frame.pack(fill=tk.X)
        
        # Stats data
        stats = [
            ("Total Devices", "[PC]", "devices_count", self.colors['accent_blue']),
            ("Commands Sent", "[*]", "commands_sent", self.colors['accent_green']),
            ("Uptime", "[T]", "uptime", self.colors['accent_orange']),
            ("Data Transferred", "[%]", "data_transferred", self.colors['accent_purple'])
        ]
        
        # Create cards in a row
        for i, (title, icon, key, color) in enumerate(stats):
            card = ttk.Frame(cards_frame, style='Card.TFrame')
            card.grid(row=0, column=i, padx=10, pady=10, sticky='ew')
            
            # Icon and title
            header_frame = ttk.Frame(card, style='Card.TFrame')
            header_frame.pack(fill=tk.X, padx=15, pady=(15, 5))
            
            icon_label = ttk.Label(header_frame, text=icon, font=('Segoe UI', 16))
            icon_label.pack(side=tk.LEFT)
            
            title_label = ttk.Label(header_frame, text=title, style='Modern.TLabel')
            title_label.pack(side=tk.LEFT, padx=(10, 0))
            
            # Value
            value_label = ttk.Label(card, text="0", style='Title.TLabel')
            value_label.pack(anchor=tk.W, padx=15, pady=(0, 15))
            
            # Store reference for updates
            setattr(self, f"stats_{key}_label", value_label)
        
        # Configure grid weights
        for i in range(len(stats)):
            cards_frame.columnconfigure(i, weight=1)
    
    # Command methods for modern interface
    def set_led_color_cmd(self, rgb):
        """Set LED color command"""
        device_id = self.selected_device.get()
        if device_id == "All Devices":
            device_id = None
        
        command = {
            "action": "set_led_color",
            "parameters": {
                "r": rgb[0],
                "g": rgb[1],
                "b": rgb[2]
            }
        }
        self.send_command_to_device(device_id, command)
    
    def set_brightness_cmd(self):
        """Set brightness command"""
        device_id = self.selected_device.get()
        if device_id == "All Devices":
            device_id = None
        
        command = {
            "action": "set_led_brightness",
            "parameters": {
                "brightness": self.brightness_var.get()
            }
        }
        self.send_command_to_device(device_id, command)
    
    def send_ping(self):
        """Send ping command"""
        device_id = self.selected_device.get()
        if device_id == "All Devices":
            device_id = None
        
        command = {"action": "ping"}
        self.send_command_to_device(device_id, command)
    
    def send_status(self):
        """Send status request"""
        device_id = self.selected_device.get()
        if device_id == "All Devices":
            device_id = None
        
        command = {"action": "status"}
        self.send_command_to_device(device_id, command)
    
    def send_boot_screen(self):
        """Send boot screen command"""
        device_id = self.selected_device.get()
        if device_id == "All Devices":
            device_id = None
        
        command = {"action": "boot_screen"}
        self.send_command_to_device(device_id, command)
    
    def send_stop_video(self):
        """Send stop video command"""
        device_id = self.selected_device.get()
        if device_id == "All Devices":
            device_id = None
        
        command = {"action": "stop_video"}
        self.send_command_to_device(device_id, command)
    
    def send_custom_command(self):
        """Send custom command"""
        device_id = self.selected_device.get()
        if device_id == "All Devices":
            device_id = None
        
        action = self.custom_action_var.get().strip()
        if not action:
            messagebox.showerror("Error", "Action cannot be empty")
            return
        
        try:
            params_text = self.custom_params_text.get('1.0', tk.END).strip()
            if params_text:
                params = json.loads(params_text)
            else:
                params = {}
            
            command = {"action": action, "parameters": params}
            self.send_command_to_device(device_id, command)
            
        except json.JSONDecodeError as e:
            messagebox.showerror("Error", f"Invalid JSON parameters: {e}")
    
    def clear_log(self):
        """Clear log display"""
        self.log_text.config(state=tk.NORMAL)
        self.log_text.delete('1.0', tk.END)
        self.log_text.config(state=tk.DISABLED)
    
    def refresh_log(self):
        """Refresh log display"""
        # This would typically fetch new log entries
        self.update_log_display()
    
    def send_command_to_device(self, device_id, command):
        """Send command to device(s)"""
        try:
            # Extract action and parameters from command dict
            action = command.get('action', '')
            parameters = command.get('parameters', {})
            
            # Remove action and parameters from command dict to avoid duplication
            command_params = {k: v for k, v in command.items() if k not in ['action', 'parameters']}
            # Merge any additional parameters
            if command_params:
                parameters.update(command_params)
            
            if device_id is None:
                # Send to all devices
                for dev_id in self.server.devices:
                    self.server.send_command(dev_id, action, parameters)
                self.add_log_entry("INFO", f"Sent command to all devices: {action}")
            else:
                self.server.send_command(device_id, action, parameters)
                self.add_log_entry("INFO", f"Sent command to {device_id}: {action}")
        except Exception as e:
            self.add_log_entry("ERROR", f"Failed to send command: {e}")
            messagebox.showerror("Error", f"Failed to send command: {e}")
    
    def add_log_entry(self, level, message):
        """Add entry to log display"""
        if hasattr(self, 'log_text'):
            timestamp = datetime.now().strftime("%H:%M:%S")
            entry = f"[{timestamp}] {level}: {message}\n"
            
            self.log_text.config(state=tk.NORMAL)
            self.log_text.insert(tk.END, entry, level)
            self.log_text.see(tk.END)
            self.log_text.config(state=tk.DISABLED)
    
    def update_log_display(self):
        """Update log display with recent entries"""
        # This is a placeholder - in a real implementation, you'd fetch from a log file or buffer
        pass
    
    def start_server(self):
        """Start the server"""
        if not self.running:
            self.running = True
            self.server_thread = threading.Thread(target=self._run_server, daemon=True)
            self.server_thread.start()
            
            if hasattr(self, 'start_button'):
                self.start_button.config(state=tk.DISABLED)
            
            self.add_log_message("Starting Tricorder GUI Server...")
    
    def _run_server(self):
        """Run server in background thread"""
        try:
            self.server.start(interactive=False)
        except Exception as e:
            self.message_queue.put(('log', 'ERROR', f"Server error: {e}"))
        finally:
            self.running = False
            self.message_queue.put(('server_status', 'stopped'))
    
    def stop_server(self):
        """Stop the server"""
        if self.running:
            self.server.stop()
            self.running = False
            
            if hasattr(self, 'start_button'):
                self.start_button.config(state=tk.NORMAL)
            
            self.add_log_message("Server stopped")
    
    def update_server_status(self, status):
        """Update server status display"""
        if status == 'stopped':
            if hasattr(self, 'start_button'):
                self.start_button.config(state=tk.NORMAL)
        elif status == 'running':
            if hasattr(self, 'start_button'):
                self.start_button.config(state=tk.DISABLED)
    
    def discover_devices(self):
        """Trigger device discovery"""
        if self.running:
            threading.Thread(target=self.server.discover_devices, daemon=True).start()
            self.add_log_message("Starting device discovery...")
        else:
            messagebox.showwarning("Server Not Running", "Please start the server first")
    
    def select_all_devices(self):
        """Select all devices for broadcast"""
        self.selected_device.set("ALL")
    
    def send_quick_command(self, action):
        """Send a quick command"""
        if not self.running:
            messagebox.showwarning("Server Not Running", "Please start the server first")
            return
        
        device_id = self.selected_device.get()
        if not device_id:
            messagebox.showwarning("No Device Selected", "Please select a device first")
            return
        
        if device_id == "ALL":
            self.server.broadcast_command(action)
            self.add_log_message(f"Broadcast command: {action}")
        else:
            self.server.send_command(device_id, action)
            self.add_log_message(f"Sent command to {device_id}: {action}")
    
    def set_led_color(self, rgb):
        """Set LED color"""
        if not self.running:
            messagebox.showwarning("Server Not Running", "Please start the server first")
            return
        
        device_id = self.selected_device.get()
        if not device_id:
            messagebox.showwarning("No Device Selected", "Please select a device first")
            return
        
        parameters = {"r": rgb[0], "g": rgb[1], "b": rgb[2]}
        
        if device_id == "ALL":
            self.server.broadcast_command("set_led_color", parameters)
            self.add_log_message(f"Broadcast LED color: RGB{rgb}")
        else:
            self.server.send_command(device_id, "set_led_color", parameters)
            self.add_log_message(f"Set LED color on {device_id}: RGB{rgb}")
    
    def set_led_brightness(self):
        """Set LED brightness"""
        if not self.running:
            messagebox.showwarning("Server Not Running", "Please start the server first")
            return
        
        device_id = self.selected_device.get()
        if not device_id:
            messagebox.showwarning("No Device Selected", "Please select a device first")
            return
        
        brightness = self.brightness_var.get()
        parameters = {"brightness": brightness}
        
        if device_id == "ALL":
            self.server.broadcast_command("set_led_brightness", parameters)
            self.add_log_message(f"Broadcast LED brightness: {brightness}")
        else:
            self.server.send_command(device_id, "set_led_brightness", parameters)
            self.add_log_message(f"Set LED brightness on {device_id}: {brightness}")
    
    def send_custom_command(self):
        """Send custom command"""
        if not self.running:
            messagebox.showwarning("Server Not Running", "Please start the server first")
            return
        
        device_id = self.selected_device.get()
        if not device_id:
            messagebox.showwarning("No Device Selected", "Please select a device first")
            return
        
        action = self.action_entry.get().strip()
        if not action:
            messagebox.showwarning("No Action", "Please enter an action")
            return
        
        try:
            params_text = self.params_text.get('1.0', tk.END).strip()
            parameters = json.loads(params_text) if params_text else {}
        except json.JSONDecodeError as e:
            messagebox.showerror("Invalid JSON", f"Parameters JSON is invalid: {e}")
            return
        
        if device_id == "ALL":
            self.server.broadcast_command(action, parameters)
            self.add_log_message(f"Broadcast custom command: {action}")
        else:
            self.server.send_command(device_id, action, parameters)
            self.add_log_message(f"Sent custom command to {device_id}: {action}")
    
    def refresh_stats(self):
        """Refresh statistics display"""
        if self.running:
            stats = self.server.get_server_stats()
            
            stats_text = "TRICORDER SERVER STATISTICS\n"
            stats_text += "=" * 50 + "\n\n"
            
            for key, value in stats.items():
                formatted_key = key.replace('_', ' ').title()
                stats_text += f"{formatted_key:.<30} {value}\n"
            
            stats_text += "\n" + "=" * 50 + "\n"
            stats_text += f"Last Updated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n"
            
            self.stats_text.delete('1.0', tk.END)
            self.stats_text.insert('1.0', stats_text)
        else:
            self.stats_text.delete('1.0', tk.END)
            self.stats_text.insert('1.0', "Server not running - no statistics available")
    
    def clear_log(self):
        """Clear the log display"""
        self.log_text.delete('1.0', tk.END)
    
    def save_log(self):
        """Save log to file"""
        from tkinter import filedialog
        
        filename = filedialog.asksaveasfilename(
            defaultextension=".log",
            filetypes=[("Log files", "*.log"), ("Text files", "*.txt"), ("All files", "*.*")]
        )
        
        if filename:
            try:
                with open(filename, 'w') as f:
                    f.write(self.log_text.get('1.0', tk.END))
                messagebox.showinfo("Log Saved", f"Log saved to {filename}")
            except Exception as e:
                messagebox.showerror("Save Error", f"Failed to save log: {e}")
    
    def show_about(self):
        """Show about dialog"""
        about_text = """Prop Control GUI Server v0.1
"Mission Control"

A graphical interface for the Prop Control System.

Features:
• Visual device management
• Real-time server monitoring
• Interactive command sending
• LED color controls
• Statistics and logging

Built with Python tkinter
Part of the Prop Control System v0.1"""
        
        messagebox.showinfo("About", about_text)
    
    def on_closing(self):
        """Handle application closing"""
        if self.running:
            if messagebox.askokcancel("Quit", "Server is running. Stop server and quit?"):
                self.stop_server()
                time.sleep(1)  # Give server time to stop
                self.root.destroy()
        else:
            self.root.destroy()
    
    def run(self):
        """Run the GUI application"""
        self.root.protocol("WM_DELETE_WINDOW", self.on_closing)
        
        # Add initial log message
        self.add_log_message("Tricorder GUI Server v0.1 - Mission Control")
        self.add_log_message("Ready to start server. Click 'Start Server' to begin.")
        
        # Start the GUI
        self.root.mainloop()

def main():
    """Main entry point"""
    import argparse
    
    parser = argparse.ArgumentParser(description='Tricorder GUI Server v0.1')
    parser.add_argument('--port', type=int, default=8888,
                       help='UDP port to listen on (default: 8888)')
    parser.add_argument('--log-file', type=str, default='tricorder_gui_server.log',
                       help='Log file path (default: tricorder_gui_server.log)')
    
    args = parser.parse_args()
    
    # Update config with command line arguments
    CONFIG['udp_port'] = args.port
    CONFIG['log_file'] = args.log_file
    
    print("Starting Tricorder GUI Server...")
    print(f"UDP Port: {CONFIG['udp_port']}")
    print(f"Log File: {CONFIG['log_file']}")
    
    try:
        app = TricorderGUIServer()
        app.run()
    except Exception as e:
        messagebox.showerror("Startup Error", f"Failed to start GUI server: {e}")
        return 1
    
    return 0

if __name__ == '__main__':
    sys.exit(main())
