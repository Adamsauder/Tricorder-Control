#!/usr/bin/env python3
"""
Quick test for the standalone server
"""

import sys
import os
import time
import threading
from pathlib import Path

# Add server directory to path
sys.path.insert(0, str(Path(__file__).parent))

try:
    from standalone_server import TricorderStandaloneServer
    
    def test_server():
        print("Testing Tricorder Standalone Server...")
        
        # Create server instance
        server = TricorderStandaloneServer()
        
        # Test basic functionality
        print("✓ Server instance created successfully")
        
        # Test logging
        server.log("Test log message")
        print("✓ Logging system working")
        
        # Test stats
        stats = server.get_server_stats()
        print(f"✓ Stats system working: {len(stats)} stat fields")
        
        # Start UDP listener briefly
        print("Testing UDP listener startup...")
        
        def start_and_stop():
            success = server.start_udp_listener()
            return success
        
        # Start UDP in thread and stop quickly
        udp_thread = threading.Thread(target=start_and_stop, daemon=True)
        udp_thread.start()
        
        # Give it time to start
        time.sleep(2)
        
        if server.running:
            print("✓ UDP listener started successfully")
            server.stop()
            print("✓ Server stopped successfully")
        else:
            print("✗ UDP listener failed to start")
            return False
        
        print("\n🎉 All tests passed! Standalone server is working correctly.")
        return True
    
    if __name__ == '__main__':
        success = test_server()
        sys.exit(0 if success else 1)
        
except ImportError as e:
    print(f"✗ Import error: {e}")
    sys.exit(1)
except Exception as e:
    print(f"✗ Test failed: {e}")
    sys.exit(1)
