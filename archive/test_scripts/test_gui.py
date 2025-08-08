#!/usr/bin/env python3
"""
Quick test for the GUI server components
"""

import sys
import os
from pathlib import Path

# Add server directory to path
sys.path.insert(0, str(Path(__file__).parent))

def test_gui_imports():
    """Test that all required modules can be imported"""
    try:
        import tkinter as tk
        print("✓ tkinter available")
        
        from tkinter import ttk, scrolledtext, messagebox
        print("✓ tkinter components available")
        
        # Test that we can create a root window (without showing it)
        root = tk.Tk()
        root.withdraw()  # Hide the window
        print("✓ tkinter window creation works")
        root.destroy()
        
        return True
    except ImportError as e:
        print(f"✗ Import error: {e}")
        return False
    except Exception as e:
        print(f"✗ GUI test error: {e}")
        return False

def test_server_integration():
    """Test that GUI server can import standalone server"""
    try:
        from standalone_server import TricorderStandaloneServer, CONFIG
        print("✓ Standalone server import successful")
        
        # Test creating server instance
        server = TricorderStandaloneServer()
        print("✓ Server instance created")
        
        # Test basic functionality
        stats = server.get_server_stats()
        print(f"✓ Server stats working: {len(stats)} fields")
        
        return True
    except ImportError as e:
        print(f"✗ Server import error: {e}")
        return False
    except Exception as e:
        print(f"✗ Server integration error: {e}")
        return False

def main():
    print("Testing Tricorder GUI Server Components...")
    print("=" * 50)
    
    gui_ok = test_gui_imports()
    server_ok = test_server_integration()
    
    print("=" * 50)
    
    if gui_ok and server_ok:
        print("🎉 All tests passed! GUI server should work correctly.")
        print("\nTo start the GUI server:")
        print("  Windows: start_gui_server.bat")
        print("  PowerShell: .\\start_gui_server.ps1")
        print("  Direct: python server\\gui_server.py")
        return 0
    else:
        print("❌ Some tests failed. GUI server may not work properly.")
        if not gui_ok:
            print("  - GUI components not available")
        if not server_ok:
            print("  - Server integration failed")
        return 1

if __name__ == '__main__':
    sys.exit(main())
