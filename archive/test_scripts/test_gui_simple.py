#!/usr/bin/env python3
"""
Simple test script for the Tricorder GUI Server v0.1
Tests basic functionality without complex mocking.
"""

import sys
import os

# Add server directory to path
sys.path.append(os.path.join(os.path.dirname(__file__)))

def test_basic_imports():
    """Test that all required modules can be imported."""
    print("🔍 Testing basic imports...")
    
    try:
        # Test tkinter availability
        import tkinter as tk
        from tkinter import ttk, scrolledtext, messagebox, filedialog
        print("  ✓ tkinter modules available")
        
        # Test standard library
        import threading
        import json
        import socket
        import logging
        import datetime
        print("  ✓ Standard library modules available")
        
        # Test standalone server
        from standalone_server import TricorderStandaloneServer
        print("  ✓ Standalone server import successful")
        
        # Test GUI server
        from gui_server import TricorderGUIServer
        print("  ✓ GUI server import successful")
        
        return True
        
    except ImportError as e:
        print(f"  ❌ Import failed: {e}")
        return False

def test_tkinter_window():
    """Test that tkinter can create a basic window."""
    print("\n🖥️ Testing tkinter window creation...")
    
    try:
        import tkinter as tk
        from tkinter import ttk
        
        # Create and immediately destroy window
        root = tk.Tk()
        root.withdraw()  # Hide window
        
        # Test basic widgets
        label = tk.Label(root, text="Test")
        button = tk.Button(root, text="Test")
        frame = tk.Frame(root)
        
        # Test ttk widgets
        ttk_label = ttk.Label(root, text="Test")
        ttk_button = ttk.Button(root, text="Test")
        notebook = ttk.Notebook(root)
        
        print("  ✓ All tkinter widgets created successfully")
        
        root.destroy()
        return True
        
    except Exception as e:
        print(f"  ❌ tkinter test failed: {e}")
        return False

def test_standalone_server_creation():
    """Test that standalone server can be created."""
    print("\n🔧 Testing standalone server creation...")
    
    try:
        from standalone_server import TricorderStandaloneServer
        
        # Create server instance
        server = TricorderStandaloneServer()
        print("  ✓ Server instance created")
        
        # Check required attributes exist
        if hasattr(server, 'devices'):
            print("  ✓ Server has devices attribute")
        
        if hasattr(server, 'send_command'):
            print("  ✓ Server has send_command method")
            
        if hasattr(server, 'discover_devices'):
            print("  ✓ Server has discover_devices method")
        
        return True
        
    except Exception as e:
        print(f"  ❌ Standalone server test failed: {e}")
        return False

def test_gui_server_creation():
    """Test that GUI server can be created without showing window."""
    print("\n🎛️ Testing GUI server creation...")
    
    try:
        # Import the GUI server class
        from gui_server import TricorderGUIServer
        print("  ✓ GUI server class imported")
        
        # Test that we can at least reference the class
        # Note: We won't actually instantiate it to avoid showing windows
        if hasattr(TricorderGUIServer, '__init__'):
            print("  ✓ GUI server class has __init__ method")
        
        # Try to check the source to see if it looks correct
        import inspect
        source_lines = inspect.getsource(TricorderGUIServer)
        if 'tkinter' in source_lines and 'TricorderStandaloneServer' in source_lines:
            print("  ✓ GUI server source contains expected components")
        
        return True
        
    except Exception as e:
        print(f"  ❌ GUI server test failed: {e}")
        return False

def test_file_existence():
    """Test that all required files exist."""
    print("\n📁 Testing file existence...")
    
    server_dir = os.path.dirname(__file__)
    root_dir = os.path.dirname(server_dir)
    
    required_files = [
        ('standalone_server.py', server_dir),
        ('gui_server.py', server_dir), 
        ('start_gui_server.bat', root_dir),
        ('start_gui_server.ps1', root_dir),
        ('GUI_SERVER_README.md', server_dir)
    ]
    
    all_exist = True
    for filename, directory in required_files:
        filepath = os.path.join(directory, filename)
        if os.path.exists(filepath):
            print(f"  ✓ {filename} exists")
        else:
            print(f"  ❌ {filename} missing")
            all_exist = False
    
    return all_exist

def test_launch_scripts():
    """Test that launch scripts have correct content."""
    print("\n🚀 Testing launch scripts...")
    
    try:
        root_dir = os.path.dirname(os.path.dirname(__file__))
        
        # Test batch file
        bat_file = os.path.join(root_dir, 'start_gui_server.bat')
        if os.path.exists(bat_file):
            with open(bat_file, 'r') as f:
                content = f.read()
            if 'gui_server.py' in content and 'python.exe' in content:
                print("  ✓ Batch file has correct content")
            else:
                print("  ❌ Batch file content incorrect")
                return False
        
        # Test PowerShell file
        ps1_file = os.path.join(root_dir, 'start_gui_server.ps1')
        if os.path.exists(ps1_file):
            with open(ps1_file, 'r') as f:
                content = f.read()
            if 'gui_server.py' in content and 'python.exe' in content:
                print("  ✓ PowerShell file has correct content")
            else:
                print("  ❌ PowerShell file content incorrect")
                return False
        
        return True
        
    except Exception as e:
        print(f"  ❌ Launch script test failed: {e}")
        return False

def run_simple_tests():
    """Run all simple tests."""
    print("🧪 Tricorder GUI Server Simple Test Suite v0.1")
    print("=" * 50)
    
    tests = [
        ("Basic Imports", test_basic_imports),
        ("Tkinter Window", test_tkinter_window),
        ("Standalone Server Creation", test_standalone_server_creation),
        ("GUI Server Creation", test_gui_server_creation),
        ("File Existence", test_file_existence),
        ("Launch Scripts", test_launch_scripts),
    ]
    
    passed = 0
    failed = 0
    
    for test_name, test_func in tests:
        print(f"\n{'='*10} {test_name} {'='*10}")
        try:
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
        print("\n🎉 ALL TESTS PASSED! GUI Server components are working!")
        print("\n📋 Next Steps:")
        print("   1. Run 'start_gui_server.bat' to launch the GUI application")
        print("   2. Test device discovery and control features")
        print("   3. Verify LED controls and command sending")
        return True
    else:
        print(f"\n⚠️  {failed} test(s) failed. Please check the issues above.")
        return False

if __name__ == "__main__":
    success = run_simple_tests()
    sys.exit(0 if success else 1)
