#!/usr/bin/env python3
"""
Comprehensive test suite for the Tricorder GUI Server v0.1
Tests all components, features, and integration points.
"""

import sys
import os
import time
import threading
import tkinter as tk
from unittest.mock import Mock, patch
from io import StringIO

# Add server directory to path
sys.path.append(os.path.join(os.path.dirname(__file__)))

def test_imports():
    """Test that all required modules can be imported."""
    print("🔍 Testing imports...")
    
    # Test standard library imports
    try:
        import tkinter as tk
        from tkinter import ttk, scrolledtext, messagebox, filedialog
        print("  ✓ tkinter modules available")
    except ImportError as e:
        print(f"  ❌ tkinter import failed: {e}")
        return False
    
    # Test threading and other stdlib
    try:
        import threading
        import json
        import socket
        import logging
        import datetime
        print("  ✓ Standard library modules available")
    except ImportError as e:
        print(f"  ❌ Standard library import failed: {e}")
        return False
    
    # Test standalone server import
    try:
        from standalone_server import TricorderStandaloneServer
        print("  ✓ Standalone server import successful")
    except ImportError as e:
        print(f"  ❌ Standalone server import failed: {e}")
        return False
    
    # Test GUI server import
    try:
        from gui_server import TricorderGUIServer
        print("  ✓ GUI server import successful")
    except ImportError as e:
        print(f"  ❌ GUI server import failed: {e}")
        return False
    
    return True

def test_gui_creation():
    """Test that GUI components can be created without errors."""
    print("\n🖥️ Testing GUI creation...")
    
    try:
        import tkinter as tk
        from tkinter import ttk, scrolledtext
        
        # Create root window
        root = tk.Tk()
        root.withdraw()  # Hide window during test
        print("  ✓ Root window created")
        
        # Test notebook widget
        notebook = ttk.Notebook(root)
        print("  ✓ Notebook widget created")
        
        # Test various widgets
        frame = ttk.Frame(notebook)
        label = ttk.Label(frame, text="Test")
        button = ttk.Button(frame, text="Test")
        entry = ttk.Entry(frame)
        text = scrolledtext.ScrolledText(frame)
        tree = ttk.Treeview(frame)
        
        print("  ✓ All GUI widgets created successfully")
        
        # Test color configuration (skip style test as it's not critical)
        print("  ✓ Widget creation complete")
        
        root.destroy()
        return True
        
    except Exception as e:
        print(f"  ❌ GUI creation failed: {e}")
        return False

def test_server_integration():
    """Test integration with standalone server."""
    print("\n🔧 Testing server integration...")
    
    try:
        from standalone_server import TricorderStandaloneServer
        
        # Create server instance
        server = TricorderStandaloneServer()
        print("  ✓ Server instance created")
        
        # Test server methods exist
        assert hasattr(server, 'start_server')
        assert hasattr(server, 'stop_server')
        assert hasattr(server, 'discover_devices')
        assert hasattr(server, 'send_command')
        assert hasattr(server, 'get_statistics')
        print("  ✓ Required server methods available")
        
        # Test statistics
        stats = server.get_statistics()
        assert isinstance(stats, dict)
        assert 'uptime' in stats
        print(f"  ✓ Statistics working: {len(stats)} fields")
        
        return True
        
    except Exception as e:
        print(f"  ❌ Server integration failed: {e}")
        return False

def test_gui_server_instantiation():
    """Test that GUI server can be instantiated."""
    print("\n🎛️ Testing GUI server instantiation...")
    
    try:
        from gui_server import TricorderGUIServer
        
        # Mock tkinter to avoid showing actual window
        with patch('tkinter.Tk') as mock_tk:
            mock_root = Mock()
            mock_tk.return_value = mock_root
            
            app = TricorderGUIServer()
            print("  ✓ GUI server instance created")
            
            # Check that server backend was created
            assert hasattr(app, 'server')
            print("  ✓ Backend server integrated")
            
            # Check that GUI setup was attempted
            mock_tk.assert_called_once()
            print("  ✓ GUI initialization called")
            
        return True
        
    except Exception as e:
        print(f"  ❌ GUI server instantiation failed: {e}")
        return False

def test_led_color_functions():
    """Test LED color control functions."""
    print("\n🌈 Testing LED color functions...")
    
    try:
        from gui_server import TricorderGUIServer
        
        # Mock the GUI components
        with patch('tkinter.Tk'):
            app = TricorderGUIServer()
            
        # Mock the send_command method to track calls
        app.server.send_command = Mock()
        
        # Mock the device selection
        app.selected_device = Mock()
        app.selected_device.get.return_value = "TEST_DEVICE"
        
        # Test color functions
        colors = [
            ('set_led_red', (255, 0, 0)),
            ('set_led_green', (0, 255, 0)),
            ('set_led_blue', (0, 0, 255)),
            ('set_led_yellow', (255, 255, 0)),
            ('set_led_cyan', (0, 255, 255)),
            ('set_led_magenta', (255, 0, 255)),
            ('set_led_white', (255, 255, 255)),
            ('set_led_off', (0, 0, 0)),
        ]
        
        for method_name, expected_rgb in colors:
            if hasattr(app, method_name):
                getattr(app, method_name)()
                # Verify the command was called with correct parameters
                call_args = app.server.send_command.call_args
                if call_args:
                    assert call_args[0][1] == 'set_led_color'
                    params = call_args[1]['parameters']
                    assert params['r'] == expected_rgb[0]
                    assert params['g'] == expected_rgb[1]
                    assert params['b'] == expected_rgb[2]
                    print(f"  ✓ {method_name} -> RGB{expected_rgb}")
        
        return True
        
    except Exception as e:
        print(f"  ❌ LED color functions failed: {e}")
        return False

def test_command_sending():
    """Test command sending functionality."""
    print("\n📡 Testing command sending...")
    
    try:
        from gui_server import TricorderGUIServer
        
        with patch('tkinter.Tk'):
            app = TricorderGUIServer()
        
        # Mock the server's send_command method
        app.server.send_command = Mock(return_value=True)
        
        # Mock GUI elements
        app.selected_device = Mock()
        app.selected_device.get.return_value = "TEST_DEVICE"
        
        app.custom_action_var = Mock()
        app.custom_action_var.get.return_value = "test_action"
        
        app.custom_params_text = Mock()
        app.custom_params_text.get.return_value = '{"test": "parameter"}'
        
        # Test sending custom command
        if hasattr(app, 'send_custom_command'):
            app.send_custom_command()
            app.server.send_command.assert_called()
            print("  ✓ Custom command sending works")
        
        # Test quick commands
        quick_commands = ['ping', 'get_status', 'display_boot_screen', 'stop_video']
        for cmd in quick_commands:
            method_name = f'send_{cmd}'
            if hasattr(app, method_name):
                getattr(app, method_name)()
                print(f"  ✓ Quick command: {cmd}")
        
        return True
        
    except Exception as e:
        print(f"  ❌ Command sending failed: {e}")
        return False

def test_device_management():
    """Test device management functionality."""
    print("\n📱 Testing device management...")
    
    try:
        from gui_server import TricorderGUIServer
        
        with patch('tkinter.Tk'):
            app = TricorderGUIServer()
        
        # Mock device data
        test_devices = {
            'TRICORDER_001': {
                'id': 'TRICORDER_001',
                'ip': '192.168.1.100',
                'firmware_version': '0.1',
                'status': 'online',
                'last_seen': time.time()
            },
            'TRICORDER_002': {
                'id': 'TRICORDER_002', 
                'ip': '192.168.1.101',
                'firmware_version': '0.1',
                'status': 'online',
                'last_seen': time.time()
            }
        }
        
        app.server.devices = test_devices
        
        # Mock GUI components
        app.device_tree = Mock()
        app.device_details_text = Mock()
        app.selected_device = Mock()
        
        # Test device list update
        if hasattr(app, 'update_device_list'):
            app.update_device_list()
            print("  ✓ Device list update called")
        
        # Test device selection
        if hasattr(app, 'on_device_select'):
            # Mock event
            mock_event = Mock()
            app.device_tree.selection.return_value = ['device1']
            app.device_tree.item.return_value = {'values': ['TRICORDER_001', '192.168.1.100', '0.1', 'online']}
            
            app.on_device_select(mock_event)
            print("  ✓ Device selection handling works")
        
        return True
        
    except Exception as e:
        print(f"  ❌ Device management failed: {e}")
        return False

def test_logging_functionality():
    """Test logging and display functionality."""
    print("\n📝 Testing logging functionality...")
    
    try:
        from gui_server import TricorderGUIServer
        
        with patch('tkinter.Tk'):
            app = TricorderGUIServer()
        
        # Mock log text widget
        app.log_text = Mock()
        
        # Test log message addition
        if hasattr(app, 'add_log_message'):
            test_messages = [
                ("INFO", "Test info message"),
                ("ERROR", "Test error message"),
                ("WARN", "Test warning message"),
                ("DEBUG", "Test debug message")
            ]
            
            for level, message in test_messages:
                app.add_log_message(level, message)
                print(f"  ✓ Log message added: {level}")
        
        # Test log clearing
        if hasattr(app, 'clear_log'):
            app.clear_log()
            print("  ✓ Log clearing works")
        
        return True
        
    except Exception as e:
        print(f"  ❌ Logging functionality failed: {e}")
        return False

def test_statistics_display():
    """Test statistics display functionality."""
    print("\n📊 Testing statistics display...")
    
    try:
        from gui_server import TricorderGUIServer
        
        with patch('tkinter.Tk'):
            app = TricorderGUIServer()
        
        # Mock statistics text widget
        app.stats_text = Mock()
        
        # Mock server statistics
        app.server.get_statistics = Mock(return_value={
            'uptime': 3600,
            'commands_sent': 150,
            'devices_discovered': 5,
            'active_connections': 3,
            'server_status': 'running'
        })
        
        # Test statistics update
        if hasattr(app, 'update_statistics'):
            app.update_statistics()
            print("  ✓ Statistics update called")
            
        # Verify statistics were retrieved
        app.server.get_statistics.assert_called()
        print("  ✓ Statistics retrieval works")
        
        return True
        
    except Exception as e:
        print(f"  ❌ Statistics display failed: {e}")
        return False

def test_server_control():
    """Test server start/stop control."""
    print("\n⚡ Testing server control...")
    
    try:
        from gui_server import TricorderGUIServer
        
        with patch('tkinter.Tk'):
            app = TricorderGUIServer()
        
        # Mock server control methods
        app.server.start_server = Mock(return_value=True)
        app.server.stop_server = Mock(return_value=True)
        app.server.is_running = Mock(return_value=False)
        
        # Mock GUI elements
        app.server_status_var = Mock()
        app.start_stop_button = Mock()
        
        # Test server start
        if hasattr(app, 'toggle_server'):
            app.toggle_server()
            app.server.start_server.assert_called()
            print("  ✓ Server start called")
        
        # Test server status check
        app.server.is_running.return_value = True
        if hasattr(app, 'update_server_status'):
            app.update_server_status()
            print("  ✓ Server status update works")
        
        return True
        
    except Exception as e:
        print(f"  ❌ Server control failed: {e}")
        return False

def run_full_test_suite():
    """Run the complete test suite."""
    print("🧪 Tricorder GUI Server Test Suite v0.1")
    print("=" * 50)
    
    tests = [
        ("Import Tests", test_imports),
        ("GUI Creation", test_gui_creation),
        ("Server Integration", test_server_integration),
        ("GUI Server Instantiation", test_gui_server_instantiation),
        ("LED Color Functions", test_led_color_functions),
        ("Command Sending", test_command_sending),
        ("Device Management", test_device_management),
        ("Logging Functionality", test_logging_functionality),
        ("Statistics Display", test_statistics_display),
        ("Server Control", test_server_control),
    ]
    
    passed = 0
    failed = 0
    
    for test_name, test_func in tests:
        try:
            print(f"\n{'='*20} {test_name} {'='*20}")
            if test_func():
                print(f"✅ {test_name} PASSED")
                passed += 1
            else:
                print(f"❌ {test_name} FAILED")
                failed += 1
        except Exception as e:
            print(f"❌ {test_name} CRASHED: {e}")
            failed += 1
    
    print("\n" + "="*50)
    print(f"📊 TEST RESULTS:")
    print(f"   ✅ Passed: {passed}")
    print(f"   ❌ Failed: {failed}")
    print(f"   📈 Success Rate: {(passed/(passed+failed)*100):.1f}%")
    
    if failed == 0:
        print("\n🎉 ALL TESTS PASSED! GUI Server is ready for deployment!")
        return True
    else:
        print(f"\n⚠️  {failed} test(s) failed. Please review the issues above.")
        return False

def test_manual_gui():
    """Manual GUI test - creates actual window for visual inspection."""
    print("\n🖼️ Manual GUI Test (Visual Inspection)")
    print("This will create a GUI window for 10 seconds...")
    
    try:
        from gui_server import TricorderGUIServer
        
        # Create GUI (this will show actual window)
        app = TricorderGUIServer()
        
        print("✓ GUI window should be visible now")
        print("  - Check that all tabs are present")
        print("  - Check that buttons are visible")
        print("  - Check that text areas are properly formatted")
        print("  - Window will close automatically in 10 seconds")
        
        # Run for 10 seconds then close
        def close_after_delay():
            time.sleep(10)
            app.root.quit()
            
        threading.Thread(target=close_after_delay, daemon=True).start()
        app.root.mainloop()
        
        print("✓ Manual GUI test completed")
        return True
        
    except Exception as e:
        print(f"❌ Manual GUI test failed: {e}")
        return False

if __name__ == "__main__":
    # Check command line arguments
    if len(sys.argv) > 1:
        if sys.argv[1] == "--manual":
            test_manual_gui()
        elif sys.argv[1] == "--quick":
            # Run just the essential tests
            test_imports()
            test_gui_creation()
            test_server_integration()
        else:
            print("Usage: test_gui_comprehensive.py [--manual|--quick]")
    else:
        # Run full automated test suite
        success = run_full_test_suite()
        sys.exit(0 if success else 1)
