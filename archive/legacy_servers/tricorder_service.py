#!/usr/bin/env python3
"""
Tricorder Service Wrapper
Runs the standalone server as a Windows service
"""

import sys
import os
import time
import logging
from pathlib import Path

# Add the server directory to the path
sys.path.insert(0, str(Path(__file__).parent))

try:
    import win32serviceutil
    import win32service
    import win32event
    import servicemanager
    HAS_WIN32 = True
except ImportError:
    HAS_WIN32 = False
    print("Warning: pywin32 not available - cannot run as Windows service")

from standalone_server import TricorderStandaloneServer, CONFIG

class TricorderService(win32serviceutil.ServiceFramework if HAS_WIN32 else object):
    _svc_name_ = "TricorderControlServer"
    _svc_display_name_ = "Tricorder Control Server"
    _svc_description_ = "Standalone server for Tricorder Control System v0.1"
    
    def __init__(self, args=None):
        if HAS_WIN32:
            win32serviceutil.ServiceFramework.__init__(self, args)
            self.hWaitStop = win32event.CreateEvent(None, 0, 0, None)
        
        self.server = None
        self.setup_logging()
    
    def setup_logging(self):
        """Setup service logging"""
        log_path = Path(__file__).parent.parent / "logs"
        log_path.mkdir(exist_ok=True)
        
        logging.basicConfig(
            level=logging.INFO,
            format='%(asctime)s - %(levelname)s - %(message)s',
            handlers=[
                logging.FileHandler(log_path / "tricorder_service.log"),
                logging.StreamHandler()
            ]
        )
        
        self.logger = logging.getLogger(__name__)
    
    def SvcStop(self):
        """Stop the service"""
        if HAS_WIN32:
            self.ReportServiceStatus(win32service.SERVICE_STOP_PENDING)
            win32event.SetEvent(self.hWaitStop)
        
        self.logger.info("Service stop requested")
        
        if self.server:
            self.server.stop()
    
    def SvcDoRun(self):
        """Run the service"""
        if HAS_WIN32:
            servicemanager.LogMsg(
                servicemanager.EVENTLOG_INFORMATION_TYPE,
                servicemanager.PYS_SERVICE_STARTED,
                (self._svc_name_, '')
            )
        
        self.logger.info("Tricorder Control Service starting...")
        
        try:
            # Create and configure server
            self.server = TricorderStandaloneServer()
            
            # Override config for service mode
            CONFIG['log_file'] = str(Path(__file__).parent.parent / "logs" / "tricorder_server.log")
            
            self.logger.info("Starting server in service mode...")
            
            # Start server in non-interactive mode
            self.server.start(interactive=False)
            
        except Exception as e:
            self.logger.error(f"Service error: {e}")
            if HAS_WIN32:
                servicemanager.LogErrorMsg(f"Service error: {e}")
        
        self.logger.info("Tricorder Control Service stopped")
        
        if HAS_WIN32:
            servicemanager.LogMsg(
                servicemanager.EVENTLOG_INFORMATION_TYPE,
                servicemanager.PYS_SERVICE_STOPPED,
                (self._svc_name_, '')
            )

def run_as_console():
    """Run as console application when pywin32 is not available"""
    print("Running Tricorder Server as console application...")
    print("(Install pywin32 to run as Windows service)")
    
    try:
        server = TricorderStandaloneServer()
        server.start(interactive=False)
    except KeyboardInterrupt:
        print("\nShutting down...")
    except Exception as e:
        print(f"Error: {e}")

if __name__ == '__main__':
    if HAS_WIN32 and len(sys.argv) == 1:
        # No arguments - try to run as service
        try:
            servicemanager.Initialize()
            servicemanager.PrepareToHostSingle(TricorderService)
            servicemanager.StartServiceCtrlDispatcher()
        except Exception:
            # Fall back to console mode
            run_as_console()
    elif HAS_WIN32:
        # Handle service control arguments
        win32serviceutil.HandleCommandLine(TricorderService)
    else:
        # Run as console application
        run_as_console()
